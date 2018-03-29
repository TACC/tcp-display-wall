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

#include <common/work/DWwork.h>

namespace ospray {
  namespace dw {

    namespace display {

      struct SetTile : public dw::SetTile
      {
        SetTile() = default;
        SetTile(ospray::ObjectHandle &handle,
                const uint64 &size,
                const byte_t *msg);
        void sendToWorker(size_t worker, void *msg, size_t size);
        void run() override;
        void runOnMaster() override;
      };

      struct CreateFrameBuffer : public mpi::work::CreateFrameBuffer
      {
        CreateFrameBuffer() = default;
        CreateFrameBuffer(ObjectHandle handle,
                          vec2i dimensions,
                          OSPFrameBufferFormat format,
                          uint32 channels);
        void run() override;
        void runOnMaster() override;
      };

      struct RenderFrame : public mpi::work::RenderFrame
      {
        RenderFrame() = default;
        RenderFrame(OSPFrameBuffer fb, OSPRenderer renderer, uint32 channels);
        void run() override;
        void runOnMaster() override;
      };

      void registerOSPWorkItems(mpi::work::WorkTypeRegistry &registry);
    }  // namespace display

  }  // namespace dw
}  // namespace ospray