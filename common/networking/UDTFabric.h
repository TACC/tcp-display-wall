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

/** @file UDPFabric.h
 *  @brief Description of UDPFabric.h
 *
 *  Description of UDPFabric.h
 *
 *  @author Joao Barbosa
 *  @date  4/9/18
 *  @bug No known bugs.
 */


#pragma once

/**
 *  @class UDPFabric
 *  @brief Description of UDPFabric
 *
 *  <detail description>
 *   
 *  @note <notes>
 *
 **/

#include "ospcommon/networking/Fabric.h"
#include "MPICommon.h"
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */

namespace mpicommon {

struct UDTFabric : public networking::Fabric {

  UDTFabric(std::string hostname, int port, bool server = false);

  virtual ~UDTFabric();

  /*! send exact number of bytes - the fabric can do that through
    multiple smaller messages, but all bytes have to be
    delivered */
  virtual void send(void *mem, size_t size) override;

  /*! receive some block of data - whatever the sender has sent -
    and give us size and pointer to this data */
  virtual size_t read(void *&mem) override;

protected:
  std::vector<byte_t> buffer;
  std::string hostname;
  bool server;
  int port;

  int connection;

};
}




