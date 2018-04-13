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
#pragma once

#include <common/work/DWwork.h>
#include <mpi/common/OSPWork.h>

namespace ospray {
  namespace dw {
    namespace farm {

      void registerOSPWorkItems(mpi::work::WorkTypeRegistry &registry);

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

    }  // namespace farm
  }    // namespace dw
}  // namespace ospray
