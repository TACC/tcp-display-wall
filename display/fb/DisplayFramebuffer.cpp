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
#include "DisplayFramebuffer.h"
#include <ospray/fb/LocalFB.h>
#include <thread>

template <typename T>
bool inline inRange(const T &x, const T &lower, const T &upper)
{
  return (x >= lower && x < upper);
}

template <typename T>
T inline minInRange(const T &x, const T &lower, const T &upper)
{
  return std::min(std::max(x, lower), upper);
}

bool inline tileBelongsTo(const ospcommon::vec2i &tpos,
                          const ospcommon::vec2i &tsize,
                          const ospcommon::vec2i &dpos,
                          const ospcommon::vec2i &dsize)
{
  bool test = inRange<int>(tpos.x, dpos.x, dpos.x + dsize.x) ||
              inRange(dpos.x, tpos.x, tpos.x + tsize.x);
  test &= inRange<int>(tpos.y, dpos.y, dpos.y + dsize.y) ||
          inRange(dpos.y, tpos.y, tpos.y + tsize.y);
  return test;
}

ospray::dw::display::DisplayFramebuffer::DisplayFramebuffer(
    ObjectHandle &handle,
    const ospcommon::vec2i &size,
    ColorBufferFormat colorBufferFormat,
    bool hasDepthBuffer,
    bool hasAccumBuffer,
    bool hasVarianceBuffer,
    const vec2i &pos,
    const vec2f &ratio,
    const vec2i &completeScreen)
    : mpi::messaging::MessageHandler(handle),
      ospray::FrameBuffer(size,
                          colorBufferFormat,
                          hasDepthBuffer,
                          hasAccumBuffer,
                          hasVarianceBuffer),
      pos(pos),
      ratio(ratio),
      completeScreen(completeScreen)
{
  Assert(size.x > 0);
  Assert(size.y > 0);

  switch (colorBufferFormat) {
  case OSP_FB_NONE:
    colorBuffer = nullptr;
    break;
  case OSP_FB_RGBA8:
  case OSP_FB_SRGBA:
    colorBuffer = (uint32 *)alignedMalloc(sizeof(uint32) * size.x * size.y);
    break;
  case OSP_FB_RGBA32F:
    colorBuffer = (vec4f *)alignedMalloc(sizeof(vec4f) * size.x * size.y);
    break;
  }

  int aa = 0;
  for (int y = 0; y < completeScreen.y; y += TILE_SIZE) {
    for (int x = 0; x < completeScreen.x; x += TILE_SIZE) {
      vec2i p(x, y);

      if (mpicommon::IamTheMaster())
        tilesRequired.insert(p);
      else if (tileBelongsTo(p, vec2i(TILE_SIZE), pos, size))
        tilesRequired.insert(p);
    }
  }
}

ospray::dw::display::DisplayFramebuffer::~DisplayFramebuffer()
{
  alignedFree(colorBuffer);
}

bool ospray::dw::display::DisplayFramebuffer::isFrameReady()
{
  std::lock_guard<std::mutex> ll(tilesDone_mutex);
  return (getTotalTiles() == tilesDone.size());
}

bool ospray::dw::display::DisplayFramebuffer::setNumTilesDone(
    const vec2i &tileDone)
{
  std::lock_guard<std::mutex> lock(tilesDone_mutex);
  tilesDone.insert(tileDone);
  bool done = (getTotalTiles() == tilesDone.size());
  if (done)
    condition_done.notify_all();
  return done;
}

void ospray::dw::display::DisplayFramebuffer::waitUntilFrameDone()
{
  std::unique_lock<std::mutex> lock(done);
  condition_done.wait(lock, [&] { return isFrameReady(); });
}

void ospray::dw::display::DisplayFramebuffer::incoming(
    const std::shared_ptr<maml::Message> &message)
{
  while (!frameActive)
    ;
  auto tile = (TileData *)message->data;

  switch (tile->type) {
  case OSP_FB_RGBA8:
    accum((TilePixels<OSP_FB_RGBA8> *)tile);
    break;
  case OSP_FB_RGBA32F:
    accum((TilePixels<OSP_FB_RGBA32F> *)tile);
    break;
  case OSP_FB_NONE:
    accum((TilePixels<OSP_FB_NONE> *)tile);
    break;
  }
}

void ospray::dw::display::DisplayFramebuffer::beginFrame()
{
  mpi::messaging::enableAsyncMessaging();
  std::lock_guard<std::mutex> lock(tilesDone_mutex);
  tilesDone.clear();
  frameActive = true;
  condition_done.notify_all();
}

float ospray::dw::display::DisplayFramebuffer::endFrame(
    const float errorThreshold)
{
  mpi::messaging::disableAsyncMessaging();
  frameActive = false;
  std::lock_guard<std::mutex> lock(tilesDone_mutex);
  tilesDone.clear();
}
const void *ospray::dw::display::DisplayFramebuffer::mapDepthBuffer()
{
  return nullptr;
}
const void *ospray::dw::display::DisplayFramebuffer::mapColorBuffer()
{
  this->refInc();
  return (const void *)colorBuffer;
}
void ospray::dw::display::DisplayFramebuffer::unmap(const void *mappedMem)
{
  if (!(mappedMem == colorBuffer)) {
    throw std::runtime_error(
        "ERROR: unmapping a pointer not created by "
        "OSPRay!");
  }
  this->refDec();
}
void ospray::dw::display::DisplayFramebuffer::setTile(ospray::Tile &tile) {}
void ospray::dw::display::DisplayFramebuffer::clear(
    const ospray::uint32 fbChannelFlags)
{
  frameID = -1;  // we increment at the start of the frame
}
ospray::int32 ospray::dw::display::DisplayFramebuffer::accumID(
    const ospcommon::vec2i &tile)
{
  return 0;
}
float ospray::dw::display::DisplayFramebuffer::tileError(
    const ospcommon::vec2i &tile)
{
  return 0;
}
int ospray::dw::display::DisplayFramebuffer::getTotalTiles() const
{
  return tilesRequired.size();
}

std::set<ospcommon::vec2i> ospray::dw::display::DisplayFramebuffer::diff()
{
  std::set<ospcommon::vec2i> d;
  for (auto p : tilesRequired) {
    if (tilesDone.find(p) == tilesDone.end())
      d.insert(p);
  }

  return d;
}