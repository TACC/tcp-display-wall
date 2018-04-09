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

#include "ospcommon/networking/DataStreaming.h"
#include "ospcommon/networking/Fabric.h"
#include "ospcommon/networking/Socket.h"

#include "MPICommon.h"
#include <mutex>
#include <condition_variable>
#include <thread>

namespace mpicommon {

  struct msg_send {
    uint_fast64_t sz32;
    uint_fast64_t bytesWritten;
    byte_t *outCompressed;
  };

  struct TCPFabric : public networking::Fabric
  {
    TCPFabric(std::string hostname, int port, bool server = false);

    virtual ~TCPFabric();

    /*! send exact number of bytes - the fabric can do that through
      multiple smaller messages, but all bytes have to be
      delivered */
    virtual void send(void *mem, size_t size) override;

    /*! receive some block of data - whatever the sender has sent -
      and give us size and pointer to this data */
    virtual size_t read(void *&mem) override;

    virtual bool isServer()
    {
      return server;
    }

    bool active = {true};
    std::mutex _protect_buffer;
    std::vector<msg_send> outgoing;
    std::condition_variable cv;
    ospcommon::socket_t connection;
    std::thread sendThread;

   private:
    // wait for Bcast with non-blocking test, and barrier
    // void waitForBcast(MPI_Request &);
    std::vector<byte_t> buffer;
    std::string hostname;
    bool server;
    int port;





  };




}  // namespace mpicommon
