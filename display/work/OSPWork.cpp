/* =======================================================================================
   This file is released as part of TCP Display Wall module for TCP Bridged
   Display Wall module for OSPray

   https://github.com/TACC/tcp-display-wall

   Copyright 2017-2018 Texas Advanced Computing Center, The University of Texas
   at Austin All rights reserved.

   Licensed under the BSD 3-Clause License, (the "License"); you may not use
   this file except in compliance with the License. A copy of the License is
   included with this software in the file LICENSE. If your copy does not
   contain the License, you may obtain a copy of the License at:

       http://opensource.org/licenses/BSD-3-Clause

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
   WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
   License for the specific language governing permissions and limitations under
   limitations under the License.

   TCP Bridged Display Wall funded in part by an Intel Visualization Center of
   Excellence award
   =======================================================================================
   @author Joao Barbosa <jbarbosa@tacc.utexas.edu>
 */
#include "OSPWork.h"
#include <display/Device.h>
#include <display/fb/DisplayFramebuffer.h>
#include <display/glDisplay/glDisplay.h>
#include <mpi/fb/DistributedFrameBuffer.h>
#include <future>

#include <ospcommon/tasking/parallel_foreach.h>
#include <ospcommon/tasking/schedule.h>

//#define DW_MEASURE_RENDER 1

ospray::dw::display::SetTile::SetTile(ospray::ObjectHandle &handle,
                                      const ospray::uint64 &size,
                                      const ospcommon::byte_t *msg)
    : ospray::dw::SetTile(handle, size, msg)
{
}

void ospray::dw::display::SetTile::run() {}

void ospray::dw::display::SetTile::sendToWorker(size_t worker,
                                                void *msg,
                                                size_t size)
{
  std::shared_ptr<maml::Message> msgsend =
      std::make_shared<maml::Message>(msg, size);
  mpi::messaging::sendTo(
      mpicommon::globalRankFromWorkerRank(worker), fbHandle, msgsend);
}

void ospray::dw::display::SetTile::runOnMaster()
{
  Buffer _buffer;
  _buffer.decode(data);

  auto device =
      std::dynamic_pointer_cast<display::Device>(api::Device::current);
  auto dfb = dynamic_cast<dw::display::DisplayFramebuffer *>(fbHandle.lookup());

  for (auto &tdata : _buffer._buffer_data) {
    auto *msg = (ospray::MasterTileMessage *)tdata.get();
    if (msg->command & MASTER_WRITE_TILE_I8) {
      dfb->accum<OSP_FB_RGBA8>(msg->coords,
                               ((MasterTileMessage_RGBA_I8 *)msg)->color);
      display::TilePixels<OSP_FB_RGBA8> tile(
          msg->coords, (byte_t *)((MasterTileMessage_RGBA_I8 *)msg)->color);
      const auto &ranks = device->wc->getRanks(tile.coords);
      std::shared_ptr<maml::Message> msgsend =
          std::make_shared<maml::Message>(&tile, sizeof(tile));

      for (auto &w : ranks) {
        mpi::messaging::sendTo(
            mpicommon::globalRankFromWorkerRank(w), fbHandle, msgsend);
      }
    } else if (msg->command & MASTER_WRITE_TILE_F32) {
      dfb->accum<OSP_FB_RGBA32F>(
          msg->coords,
          (unsigned int *)((MasterTileMessage_RGBA_F32 *)msg)->color);

      display::TilePixels<OSP_FB_RGBA32F> tile(
          msg->coords, (byte_t *)((MasterTileMessage_RGBA_F32 *)msg)->color);
      const auto &ranks = device->wc->getRanks(tile.coords);
      std::shared_ptr<maml::Message> msgsend =
          std::make_shared<maml::Message>(&tile, sizeof(tile));

      for (auto &w : ranks) {
        mpi::messaging::sendTo(
            mpicommon::globalRankFromWorkerRank(w), fbHandle, msgsend);
      }
    } else {
      throw std::runtime_error("Got an unexpected message");
    }
  };
}

ospray::dw::display::CreateFrameBuffer::CreateFrameBuffer(
    ospray::ObjectHandle handle,
    ospcommon::vec2i dimensions,
    OSPFrameBufferFormat format,
    ospray::uint32 channels)
    : mpi::work::CreateFrameBuffer(handle, dimensions, format, channels)
{
}

void ospray::dw::display::CreateFrameBuffer::run()
{
  assert(dimensions.x > 0);
  assert(dimensions.y > 0);
  auto wc =
      std::dynamic_pointer_cast<display::Device>(api::Device::current)->wc;
  FrameBuffer *fb;
  if (mpicommon::IamTheMaster()) {
    fb = new DisplayFramebuffer(
        handle,
        dimensions,
        format,
        channels,
        vec2i(0),
#ifndef __APPLE__
        vec2f(float(dimensions.x) / wc->completeScreeen.x,
              float(dimensions.y) / wc->completeScreeen.y),
#else
        vec2f(1.f),
#endif
        wc->completeScreeen);
  } else {
    fb = new DisplayFramebuffer(handle,
                                wc->localScreen,
                                format,
                                channels,
                                wc->localPosition,
                                vec2f(1.f),
                                wc->completeScreeen);
  }
  handle.assign(fb);
}

void ospray::dw::display::CreateFrameBuffer::runOnMaster()
{
  run();
}

void ospray::dw::display::registerOSPWorkItems(
    mpi::work::WorkTypeRegistry &registry)
{
  mpi::work::registerOSPWorkItems(registry);
  // Register common work
  mpi::work::registerWorkUnit<dw::display::SetTile>(registry);
  // Create a different buffer (instead of writing the buffer just forward)
  mpi::work::registerWorkUnit<dw::display::CreateFrameBuffer>(registry);
  // Local Definitions
  mpi::work::registerWorkUnit<dw::display::RenderFrame>(registry);
}

ospray::dw::display::RenderFrame::RenderFrame(OSPFrameBuffer fb,
                                              OSPRenderer renderer,
                                              ospray::uint32 channels)
    : mpi::work::RenderFrame(fb, renderer, channels)
{
}

void ospray::dw::display::RenderFrame::run()
{
  auto *dfb = dynamic_cast<display::DisplayFramebuffer *>(fbHandle.lookup());
  dfb->beginFrame();
  dfb->waitUntilFrameDone();
  dfb->endFrame(inf);
  byte_t *color = (byte_t *)dfb->mapBuffer(OSP_FB_COLOR);
  mpicommon::worker.barrier();
  dw::glDisplay::loadFrame(color, dfb->size);
  mpicommon::world.barrier();
  if (color)
    dfb->unmap(color);
}

void ospray::dw::display::RenderFrame::runOnMaster()
{
#ifdef DW_MEASURE_RENDER
  auto start = std::chrono::steady_clock::now();
#endif

  auto device =
      std::dynamic_pointer_cast<dw::display::Device>(api::Device::current);
  assert(device);
  auto *dfb = dynamic_cast<display::DisplayFramebuffer *>(fbHandle.lookup());
  dfb->beginFrame();

  auto read    = 0;
  auto process = 0;

  while (!dfb->isFrameReady()) {
#ifdef DW_MEASURE_RENDER
    auto wait_start = std::chrono::steady_clock::now();
#endif

    auto work = device->readWork();

#ifdef DW_MEASURE_RENDER
    auto wait_end = std::chrono::steady_clock::now();
#endif

    work->runOnMaster();

#ifdef DW_MEASURE_RENDER
    auto process_end = std::chrono::steady_clock::now();
    read += std::chrono::duration_cast<std::chrono::milliseconds>(wait_end -
                                                                  wait_start)
                .count();
    process += std::chrono::duration_cast<std::chrono::milliseconds>(
                   process_end - wait_end)
                   .count();
#endif
  }

#ifdef DW_MEASURE_RENDER
  auto end_frame = std::chrono::steady_clock::now();
#endif

  dfb->endFrame(inf);
  mpicommon::world.barrier();

#ifdef DW_MEASURE_RENDER
  auto end = std::chrono::steady_clock::now();
  auto spf =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_frame - start);
  auto fps = 1.f / (spf.count() * 0.001);
  std::cout << "FPS : " << fps << " (" << spf.count() << "ms, " << read
            << "ms, " << process << "ms )\n";
#endif
}
