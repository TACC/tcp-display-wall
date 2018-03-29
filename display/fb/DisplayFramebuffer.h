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
#pragma once

#include <mpi/common/Messaging.h>
#include <ospray/fb/LocalFB.h>
#include "ospcommon/tasking/parallel_for.h"

#include <condition_variable>
#include <mutex>
#include <set>

namespace ospray {
  namespace dw {
    namespace display {

      //
      //    OSP_FB_NONE,    //!< framebuffer will not be mapped by application
      //    OSP_FB_RGBA8,   //!< one dword per pixel: rgb+alpha, each one byte
      //    OSP_FB_SRGBA,   //!< one dword per pixel: rgb (in sRGB space) +
      //    alpha, each one byte OSP_FB_RGBA32F, //!< one float4 per pixel:
      //    rgb+alpha, each one float

      template <OSPFrameBufferFormat>
      constexpr size_t sizeOfType()
      {
        return 0;
      }

      template <>
      constexpr size_t sizeOfType<OSP_FB_NONE>()
      {
        return 0;
      }
      template <>
      constexpr size_t sizeOfType<OSP_FB_RGBA8>()
      {
        return sizeof(uint32);
      }

      template <>
      constexpr size_t sizeOfType<OSP_FB_SRGBA>()
      {
        return sizeof(uint32);
      }

      template <>
      constexpr size_t sizeOfType<OSP_FB_RGBA32F>()
      {
        return sizeof(vec4f);
      }

      struct TileData
      {
        OSPFrameBufferFormat type;
        vec2i coords;

        TileData(const OSPFrameBufferFormat &type = OSP_FB_NONE,
                 const vec2i &coords              = vec2i(0))
            : type(type), coords(coords){};
      };

      template <OSPFrameBufferFormat FBType>
      struct TilePixels : public TileData
      {
        static constexpr size_t bufsize{TILE_SIZE * TILE_SIZE *
                                        sizeOfType<FBType>()};
        byte_t finaltile[bufsize];

        TilePixels() : TileData(FBType){};
        TilePixels(const vec2i &coords, const byte_t *buffer)
            : TileData(FBType, coords)
        {
          std::memcpy(finaltile, buffer, bufsize);
        }
      };

      struct DisplayFramebuffer : mpi::messaging::MessageHandler,
                                  ospray::FrameBuffer
      {
        DisplayFramebuffer(ObjectHandle &handle,
                           const vec2i &size,
                           ColorBufferFormat colorBufferFormat,
                           bool hasDepthBuffer,
                           bool hasAccumBuffer,
                           bool hasVarianceBuffer,
                           const vec2i &pos,
                           const vec2f &ratio,
                           const vec2i &completeScreen);
        ~DisplayFramebuffer() override;
        void incoming(const std::shared_ptr<maml::Message> &message) override;
        bool isFrameReady();
        bool setNumTilesDone(const vec2i &tilesDone);
        void waitUntilFrameDone();
        const void *mapDepthBuffer() override;
        const void *mapColorBuffer() override;
        void unmap(const void *mappedMem) override;
        void setTile(Tile &tile) override;
        void clear(const uint32 fbChannelFlags) override;
        int32 accumID(const vec2i &tile) override;
        float tileError(const vec2i &tile) override;
        void beginFrame() override;
        float endFrame(const float errorThreshold) override;
        int getTotalTiles() const;

        template <OSPFrameBufferFormat FBType>
        inline void accum(TilePixels<FBType> *tile)
        {
          throw std::runtime_error("Unknown type");
        }

        void createTiles();

        std::set<vec2i> diff();

       protected:
        std::atomic<size_t> numTilesDone{0};
        std::mutex done;
        std::condition_variable condition_done;
        void *colorBuffer = nullptr;
        bool frameActive{false};
        vec2f ratio;
        vec2i pos;
        vec2i completeScreen;

        std::set<vec2i> tilesRequired;

        std::mutex tilesDone_mutex;

        std::set<vec2i> tilesMissing;
      };

      template <>
      inline void DisplayFramebuffer::accum(
          ospray::dw::display::TilePixels<OSP_FB_NONE> *tile)
      {
        if (!colorBufferFormat & OSP_FB_NONE)
          throw std::runtime_error("Incompatible buffer formats");
        setNumTilesDone(1);
      }

      template <>
      inline void DisplayFramebuffer::accum(
          ospray::dw::display::TilePixels<OSP_FB_RGBA8> *tile)
      {
        if (!colorBufferFormat & OSP_FB_RGBA8)
          throw std::runtime_error("Incompatible buffer formats");
        uint32 *color  = (uint32 *)colorBuffer;
        uint32 *pixels = (uint32 *)tile->finaltile;

        int count = 0;
        tasking::parallel_for(TILE_SIZE, [&](const int iy) {
          int y = ((tile->coords.y + iy) - pos.y) * ratio.y;
          if (y < 0 || y >= size.y)
            return;
          for (int ix = 0; ix < TILE_SIZE; ix++) {
            int x = ((tile->coords.x + ix) - pos.x) * ratio.x;
            if (x < 0 || x >= size.x)
              continue;
            color[y * size.x + x] = pixels[iy * TILE_SIZE + ix];
          }
        });

        setNumTilesDone(tile->coords);
      }

      template <>
      inline void DisplayFramebuffer::accum(
          ospray::dw::display::TilePixels<OSP_FB_RGBA32F> *tile)
      {
        if (!colorBufferFormat & OSP_FB_RGBA32F)
          throw std::runtime_error("Incompatible buffer formats");
        vec4f *color  = (vec4f *)colorBuffer;
        vec4f *pixels = (vec4f *)tile->finaltile;
        tasking::parallel_for(TILE_SIZE, [&](const int iy) {
          int y = ((tile->coords.y + iy) - pos.y) * ratio.y;
          if (y < 0 || y >= size.y)
            return;
          for (int ix = 0; ix < TILE_SIZE; ix++) {
            int x = ((tile->coords.x + ix) - pos.x) * ratio.x;
            if (x < 0 || x >= size.x)
              continue;
            color[y * size.x + x] = pixels[iy * TILE_SIZE + ix];
          }
        });
        setNumTilesDone(tile->coords);
      }

    }  // namespace display
  }    // namespace dw
}  // namespace ospray