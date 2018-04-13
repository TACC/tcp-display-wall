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
#ifndef OSPRAY_DISPLAY_DEVICE_H
#define OSPRAY_DISPLAY_DEVICE_H

#include <common/networking/TCPFabric.h>
#include <display/glDisplay/WallConfig.h>
#include <mpi/MPIOffloadDevice.h>
#include <mpi/common/OSPWork.h>

namespace ospray {
  namespace dw {
    namespace display {
      struct Device : public ospray::mpi::MPIOffloadDevice
      {
       public:
        Device() = default;
        ~Device() override;
        void commit() override;
        float renderFrame(OSPFrameBuffer _sc,
                          OSPRenderer _renderer,
                          const uint32 fbChannelFlags) override;
        OSPFrameBuffer frameBufferCreate(const vec2i &size,
                                         const OSPFrameBufferFormat mode,
                                         const uint32 channels) override;

        std::unique_ptr<mpi::work::Work> readWork();

        wallconfig *wc;

       protected:
        void initializeDevice() override;
        void processWork(mpi::work::Work &work,
                         bool flushWriteStream = false) override;
        std::unique_ptr<networking::Fabric> tcpFabric{nullptr};
        std::unique_ptr<networking::ReadStream> tcpreadStream{nullptr};
        std::unique_ptr<networking::WriteStream> tcpwriteStream{nullptr};
        bool tcp_initialized{false};

        ObjectHandle wHandle;
      };
    }  // namespace display
  }    // namespace dw
}  // namespace ospray

#endif  // OSPRAY_DISPLAY_DEVICE_H
