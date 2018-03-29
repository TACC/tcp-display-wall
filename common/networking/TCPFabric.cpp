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

#include "TCPFabric.h"
#include <snappy.h>

namespace mpicommon {

  TCPFabric::TCPFabric(std::string hostname, int port, bool server)
      : hostname(hostname), port(port), server(server)
  {
    if (server) {
      ospcommon::socket_t server_socket = ospcommon::bind(port);
      connection                        = ospcommon::listen(server_socket);
    } else {
      connection = ospcommon::connect(hostname.c_str(), port);
    }
  }

  TCPFabric::~TCPFabric()
  {
    ospcommon::close(connection);
  }
#if 0
  size_t TCPFabric::read(void *&mem)
  {
    uint32_t sz32 = 0;
    // Get the size of the bcast being sent to us. Only this part must be
    // non-blocking, after getting the size we know everyone will enter the
    // blocking barrier and the blocking bcast where the buffer is sent out.
    ospcommon::read(connection, &sz32, sizeof(uint32_t));
    // TODO: Maybe at some point we should dump the buffer if it gets really
    // large
    buffer.resize(sz32);
    mem = buffer.data();
    ospcommon::read(connection, mem, sz32);
    return sz32;
  }

  void TCPFabric::send(void *mem, size_t size)
  {
    assert(size < (1LL << 30));
    uint32_t sz32 = size;
    // Send the size of the bcast we're sending. Only this part must be
    // non-blocking, after getting the size we know everyone will enter the
    // blocking barrier and the blocking bcast where the buffer is sent out.
    ospcommon::write(connection, &sz32, sizeof(uint32_t));
    // Now do non-blocking test to see when this bcast is satisfied to avoid
    // locking out the send/recv threads
    ospcommon::write(connection, mem, sz32);
    ospcommon::flush(connection);
  }
#else
size_t TCPFabric::read(void *&mem)
{
  uint32_t sz32 = 0;
  // Get the size of the bcast being sent to us. Only this part must be
  // non-blocking, after getting the size we know everyone will enter the
  // blocking barrier and the blocking bcast where the buffer is sent out.
  ospcommon::read(connection, &sz32, sizeof(uint32_t));
  // TODO: Maybe at some point we should dump the buffer if it gets really
  // large
  char* compressed = new char[sz32];
  ospcommon::read(connection, compressed, sz32);
  size_t usize;
  snappy::GetUncompressedLength(compressed,sz32,&usize);
  buffer.resize(usize);
  mem = buffer.data();
  snappy::RawUncompress(compressed,sz32,(char*)mem);
  delete [] compressed;
  return usize;
}

void TCPFabric::send(void *mem, size_t size)
{
  assert(size < (1LL << 30));
  uint32_t sz32 = size;
  char* compressedbuf = new char[snappy::MaxCompressedLength(sz32)];
  size_t compressed_size;
  snappy::RawCompress((char*)mem,sz32,compressedbuf,&compressed_size);
  // Send the size of the bcast we're sending. Only this part must be
  // non-blocking, after getting the size we know everyone will enter the
  // blocking barrier and the blocking bcast where the buffer is sent out.
  ospcommon::write(connection, &compressed_size, sizeof(uint32_t));
  // Now do non-blocking test to see when this bcast is satisfied to avoid
  // locking out the send/recv threads
  ospcommon::write(connection, compressedbuf, compressed_size);
  ospcommon::flush(connection);

  delete [] compressedbuf;
}
#endif
}  // namespace mpicommon