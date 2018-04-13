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
#ifndef OSPRAY_FARM_DEVICE_H
#define OSPRAY_FARM_DEVICE_H

#include <common/networking/TCPFabric.h>
#include <mpi/MPIOffloadDevice.h>

namespace ospray {
  namespace dw {
    namespace farm {

      struct Device : public ospray::mpi::MPIOffloadDevice
      {
        Device() = default;
        ~Device() override;
        void commit() override;
        void sendWorkDisplayWall(mpi::work::Work &work,
                                 bool flushWriteStream = false);
        mpi::work::WorkTypeRegistry &getWorkRegistry();

       protected:
        void initializeDevice() override;
        std::unique_ptr<networking::Fabric> tcpFabric{nullptr};
        std::unique_ptr<networking::ReadStream> tcpreadStream{nullptr};
        std::unique_ptr<networking::WriteStream> tcpwriteStream{nullptr};
        bool tcp_initialized{false};
      };

    }  // namespace farm
  }    // namespace dw
}  // namespace ospray

#endif  // OSPRAY_FARM_DEVICE_H
