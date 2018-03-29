/* =======================================================================================
   This file is released as part of TCP Display Wall module for TCP Bridged
   Display Wall module for OSPray tacc.github.io/display-wall

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
   =======================================================================================
   @author Joao Barbosa <jbarbosa@tacc.utexas.edu>
 */
#include "OSPWork.h"
#include <display/fb/DisplayFramebuffer.h>
#include <display/glDisplay/glDisplay.h>
#include <modules/dw/display/Device.h>
#include <mpi/fb/DistributedFrameBuffer.h>

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
  auto device =
      std::dynamic_pointer_cast<display::Device>(api::Device::current);
  auto dfb = dynamic_cast<dw::display::DisplayFramebuffer *>(fbHandle.lookup());
  auto *msg = (ospray::TileMessage *)data;
  if (msg->command & MASTER_WRITE_TILE_I8) {
    auto MT8 = (MasterTileMessage_RGBA_I8 *)msg;
    display::TilePixels<OSP_FB_RGBA8> tile(MT8->coords, (byte_t *)MT8->color);
    dfb->accum(&tile);
    for (auto w : device->wc->getRanks(tile.coords))
      sendToWorker(w, &tile, sizeof(tile));
  } else if (msg->command & MASTER_WRITE_TILE_F32) {
    auto MT32 = (MasterTileMessage_RGBA_F32 *)msg;
    display::TilePixels<OSP_FB_RGBA32F> tile(MT32->coords,
                                             (byte_t *)MT32->color);
    dfb->accum(&tile);
    for (auto w : device->wc->getRanks(tile.coords))
      sendToWorker(w, &tile, sizeof(tile));
  } else {
    throw std::runtime_error("Got an unexpected message");
  }
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
  const bool hasDepthBuffer    = channels & OSP_FB_DEPTH;
  const bool hasAccumBuffer    = channels & OSP_FB_ACCUM;
  const bool hasVarianceBuffer = channels & OSP_FB_VARIANCE;

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
        hasDepthBuffer,
        hasAccumBuffer,
        hasVarianceBuffer,
        vec2i(0),
        vec2f(float(dimensions.x) / wc->completeScreeen.x,
              float(dimensions.y) / wc->completeScreeen.y),
        wc->completeScreeen);
  } else {
    fb = new DisplayFramebuffer(handle,
                                wc->localScreen,
                                format,
                                hasDepthBuffer,
                                hasAccumBuffer,
                                hasVarianceBuffer,
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
  byte_t *color = (byte_t *)dfb->mapColorBuffer();
  mpicommon::worker.barrier();
  dw::glDisplay::loadFrame(color, dfb->size);
  mpicommon::world.barrier();
  dfb->unmap(color);
}

void ospray::dw::display::RenderFrame::runOnMaster()
{
  auto device =
      std::dynamic_pointer_cast<dw::display::Device>(api::Device::current);
  assert(device);
  auto *dfb = dynamic_cast<display::DisplayFramebuffer *>(fbHandle.lookup());
  dfb->beginFrame();

  while (!dfb->isFrameReady()) {
    auto work = device->readWork();
    auto tag  = typeIdOf(work);
    if (tag != typeIdOf<dw::display::SetTile>())
      throw std::runtime_error("Somthing went wrong it can only be a tile");
    work->runOnMaster();
  }
  dfb->endFrame(inf);
  mpicommon::world.barrier();
}
