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

#include <OSPConfig.h>

#include "UDTFabric.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


#include <udt4/api.h>
#include <udt4/ccc.h>


constexpr uint_fast64_t msg_packet_size = TILE_SIZE * TILE_SIZE * 4;
constexpr uint_fast64_t udp_packet_size = 4096;

#if defined(DW_USE_SNAPPY)
#include <snappy.h>
#elif defined(DW_USE_DENSITY)
#include <density_api.h>
#include "OSPConfig.h"


static uint64_t rcount = 0;
static uint64_t scount = 0;

#else

#endif

class CUDPBlast: public CCC
{
public:
  CUDPBlast()
  {
    m_dPktSndPeriod = 1000000;
    m_dCWndSize = 83333.0;
  }

public:
  void setRate(double mbps)
  {
    m_dPktSndPeriod = (m_iMSS * 8.0) / mbps;
  }
};


namespace ospcommon {
  namespace udt {

    void read(UDTSOCKET comm, void *mem, const size_t &size)
    {
      char* buf  = (char *)mem;
      size_t bsize = size;
      int ss;
      while (bsize > 0) {
        if (UDT::ERROR == (ss = UDT::recv(comm, buf, bsize, 0))) {
          throw std::runtime_error(UDT::getlasterror().getErrorMessage());
        }
        bsize -= ss;
        buf += ss;
      }
    }


    void write(UDTSOCKET comm, void *mem, const size_t &size)
    {
      char* buf  = (char *)mem;
      size_t bsize = size;
      int ss;
      while (bsize > 0) {
        if (UDT::ERROR == (ss = UDT::send(comm, buf, bsize, 0))) {
          throw std::runtime_error(UDT::getlasterror().getErrorMessage());
        }

        bsize -= ss;
        buf += ss;
      }
    }

  }  // namespace udt
}  // namespace ospcommon

namespace mpicommon {
struct msg_send {
  uint_fast64_t sz32;
  uint_fast64_t bytesWritten;
  byte_t *outCompressed;
};
}

mpicommon::UDTFabric::UDTFabric(std::string hostname, int port, bool server)
    : hostname(hostname), port(port), server(server)
{
  if (server) {
    UDTSOCKET serv = UDT::socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in my_addr;
    my_addr.sin_family      = AF_INET;
    my_addr.sin_port        = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(my_addr.sin_zero), '\0', 8);

    if (UDT::ERROR == UDT::bind(serv, (sockaddr *)&my_addr, sizeof(my_addr))) {
      throw std::runtime_error(UDT::getlasterror().getErrorMessage());
    }

    UDT::listen(serv, 10);

    int namelen;
    sockaddr_in their_addr;

    connection = UDT::accept(serv, (sockaddr *)&their_addr, &namelen);
  } else {
    /*! perform DNS lookup */
    struct hostent *server = ::gethostbyname(hostname.c_str());
    if (server == nullptr)
      THROW_RUNTIME_ERROR("server " + std::string(hostname.c_str()) +
                          " not found");

    /*! perform connection */
    struct sockaddr_in serv_addr;
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = (unsigned short)htons(port);
    memcpy((char *)&serv_addr.sin_addr.s_addr,
           (char *)server->h_addr,
           server->h_length);

    connection = UDT::socket(AF_INET, SOCK_STREAM, 0);

    memset(&(serv_addr.sin_zero), '\0', 8);

    if (UDT::ERROR ==
        UDT::connect(connection, (sockaddr *)&serv_addr, sizeof(serv_addr))) {
      throw std::runtime_error(UDT::getlasterror().getErrorMessage());
    }
  }


  UDT::setsockopt(connection, 0, UDT_CC, new CCCFactory<CUDPBlast> , sizeof(CCCFactory<CUDPBlast> ));

  CUDPBlast* cchandle = NULL;
  int temp;
  UDT::getsockopt(connection, 0, UDT_CC, &cchandle, &temp);
  if (NULL != cchandle)
    cchandle->setRate(1000);

  UDT::setsockopt(connection, 0, UDT_MSS, &udp_packet_size, sizeof(uint_fast64_t));
  UDT::setsockopt(connection, 0, UDT_SNDBUF, &msg_packet_size, sizeof(uint_fast64_t));
  UDT::setsockopt(connection, 0, UDT_RCVBUF, &msg_packet_size, sizeof(uint_fast64_t));
  UDT::setsockopt(connection, 0, UDT_FC, &msg_packet_size, sizeof(uint_fast64_t));
}

mpicommon::UDTFabric::~UDTFabric() {}
#if defined(DW_USE_SNAPPY)

size_t mpicommon::UDTFabric::read(void *&mem)
  {
    uint32_t sz32 = 0;
    // Get the size of the bcast being sent to us. Only this part must be
    // non-blocking, after getting the size we know everyone will enter the
    // blocking barrier and the blocking bcast where the buffer is sent out.
    ospcommon::udt::read(connection, &sz32, sizeof(uint32_t));
    // TODO: Maybe at some point we should dump the buffer if it gets really
    // large
    char *compressed_data_block = new char[sz32];
    ospcommon::udt::read(connection, compressed_data_block, sz32);
    size_t compressed_data_size;
    snappy::GetUncompressedLength(
        compressed_data_block, sz32, &compressed_data_size);
    buffer.resize(compressed_data_size);
    mem = buffer.data();
    snappy::RawUncompress(compressed_data_block, sz32, (char *)mem);
    delete[] compressed_data_block;
    return compressed_data_size;
  }

  void mpicommon::UDTFabric::send(void *mem, size_t size)
  {
    assert(size < (1LL << 30));
    uint32_t sz32               = size;
    char *compressed_data_block = new char[snappy::MaxCompressedLength(sz32)];
    size_t compressed_data_size;
    snappy::RawCompress(
        (char *)mem, sz32, compressed_data_block, &compressed_data_size);
    // Send the size of the bcast we're sending. Only this part must be
    // non-blocking, after getting the size we know everyone will enter the
    // blocking barrier and the blocking bcast where the buffer is sent out.
    ospcommon::udt::write(connection, &compressed_data_size, sizeof(uint32_t));
    // Now do non-blocking test to see when this bcast is satisfied to avoid
    // locking out the send/recv threads
    ospcommon::udt::write(connection, compressed_data_block, compressed_data_size);
    //ospcommon::flush(connection);

    delete[] compressed_data_block;
  }
#elif defined(DW_USE_DENSITY)
size_t mpicommon::UDTFabric::read(void *&mem)
{
  uint_fast64_t sz32          = 0, size_flag;
  uint_fast64_t compress_size = 0;
  //  // Get the size of the bcast being sent to us. Only this part must be
  //  // non-blocking, after getting the size we know everyone will enter the
  //  // blocking barrier and the blocking bcast where the buffer is sent out.
  //  ospcommon::read(connection, &sz32, sizeof(uint32_t));
  //  // TODO: Maybe at some point we should dump the buffer if it gets really
  //  // large
  //  buffer.resize(sz32);
  //  mem = buffer.data();
  //  ospcommon::read(connection, mem, sz32);
#ifdef DENSITY_MEASURE_TIMES
  std::chrono::high_resolution_clock::time_point tstart_read =
        std::chrono::high_resolution_clock::now();
#endif

  ospcommon::udt::read(connection, &sz32, sizeof(uint_fast64_t));
  ospcommon::udt::read(connection, &compress_size, sizeof(uint_fast64_t));
  byte_t *outCompressed = new byte_t[compress_size];
  ospcommon::udt::read(connection, outCompressed, compress_size);

#ifdef DENSITY_MEASURE_TIMES
  std::chrono::high_resolution_clock::time_point tfinish_read =
        std::chrono::high_resolution_clock::now();
#endif

  uint_fast64_t decompress_safe_size = density_decompress_safe_size(sz32);
  buffer.resize(decompress_safe_size);
  mem = buffer.data();

  density_processing_result result;
  result = density_decompress(
    outCompressed, compress_size, (uint8_t *)mem, decompress_safe_size);

  if (result.state != DENSITY_STATE_OK) {
    printf("[Decompression] Decompression %llu bytes to %llu bytes %i\n",
           result.bytesRead,
           result.bytesWritten,
           result.state);
    throw std::runtime_error("Error uncompressing data");
  }

#ifdef DENSITY_MEASURE_TIMES
  if (decompress_safe_size >= msg_packet_size) {
      std::chrono::high_resolution_clock::time_point tfinish_compression =
          std::chrono::high_resolution_clock::now();

      auto decompression_time =
          std::chrono::duration_cast<std::chrono::microseconds>(
              tfinish_compression - tfinish_read)
              .count();
      auto decompression_time_milliseconds =
          std::chrono::duration_cast<std::chrono::milliseconds>(tfinish_compression -
                                                           tfinish_read)
              .count();
      auto read_time = std::chrono::duration_cast<std::chrono::microseconds>(
                           tfinish_read - tstart_read)
                           .count();
      auto read_time_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   tfinish_read - tstart_read)
                                   .count();
      std::cout << "[ " << rcount++ << " ] Decompressiom ratio : "
                << (float(result.bytesWritten) / compress_size) << " "
                << result.bytesWritten << " /  " << compress_size << " ";
      std::cout << "Time decompression : " << decompression_time << "us ("
                << decompression_time_milliseconds << "ms) read: " << read_time
                << "us (" << read_time_milliseconds << "ms)" << std::endl;
    }
#endif

  delete[] outCompressed;
  return result.bytesWritten;
}

void mpicommon::UDTFabric::send(void *mem, size_t size)
{
  assert(size < (1LL << 30));

  msg_send msg;

  msg.sz32               = size;
  uint_fast64_t compress_safe_size = density_compress_safe_size(msg.sz32);

  DENSITY_ALGORITHM compression = DENSITY_ALGORITHM_CHEETAH;

#ifdef DENSITY_MEASURE_TIMES
  std::chrono::high_resolution_clock::time_point tstart_compression =
        std::chrono::high_resolution_clock::now();
#endif

  msg.outCompressed = new byte_t[compress_safe_size];
  density_processing_result result;
  result = density_compress(
    (uint8_t *)mem, msg.sz32, msg.outCompressed, compress_safe_size, compression);

  msg.bytesWritten = result.bytesWritten;

  if (result.state != DENSITY_STATE_OK) {
    printf("[Compression] Compression %llu bytes to %llu bytes\n",
           result.bytesRead,
           result.bytesWritten);
    msg.outCompressed;
    throw std::runtime_error("Error compressing data");
  }

#ifdef DENSITY_MEASURE_TIMES
  std::chrono::high_resolution_clock::time_point tfinish_compression =
        std::chrono::high_resolution_clock::now();
#endif

  ospcommon::udt::write(connection, &msg.sz32, sizeof(uint_fast64_t));
  ospcommon::udt::write(connection, &result.bytesWritten, sizeof(uint_fast64_t));
  ospcommon::udt::write(connection, msg.outCompressed, result.bytesWritten);
//  ospcommon::udt::flush(connection);

#ifdef DENSITY_MEASURE_TIMES
  if (compress_safe_size >= msg_packet_size) {
      std::chrono::high_resolution_clock::time_point tfinish_send =
          std::chrono::high_resolution_clock::now();

      auto compression_time =
          std::chrono::duration_cast<std::chrono::microseconds>(
              tfinish_compression - tstart_compression)
              .count();
      auto compression_time_milliseconds =
          std::chrono::duration_cast<std::chrono::milliseconds>(tfinish_compression -
                                                           tstart_compression)
              .count();
      auto send_time = std::chrono::duration_cast<std::chrono::microseconds>(
                           tfinish_send - tfinish_compression)
                           .count();
      auto send_time_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   tfinish_send - tfinish_compression)
                                   .count();
      std::cout << "[ " << scount++ << " ] Compression ratio : "
                << (float(msg.sz32) / result.bytesWritten) << " " << msg.sz32 << " /  "
                << result.bytesWritten << " ";
      std::cout << "Time compression : " << compression_time << "us ("
                << compression_time_milliseconds << "ms) send: " << send_time
                << "us (" << send_time_milliseconds << "ms)" << std::endl;
    }
#endif

  //delete[] outCompressed;
}
#else
size_t mpicommon::UDTFabric::read(void *&mem)
{
  uint32_t sz32 = 0;
  // Get the size of the bcast being sent to us. Only this part must be
  // non-blocking, after getting the size we know everyone will enter the
  // blocking barrier and the blocking bcast where the buffer is sent out.
  ospcommon::udt::read(connection, &sz32, sizeof(uint32_t));
  // TODO: Maybe at some point we should dump the buffer if it gets really
  // large
  buffer.resize(sz32);
  mem = buffer.data();
  ospcommon::udt::read(connection, mem, sz32);
  return sz32;
}

void mpicommon::UDTFabric::send(void *mem, size_t size)
{
  assert(size < (1LL << 30));
  uint32_t sz32 = size;
  // Send the size of the bcast we're sending. Only this part must be
  // non-blocking, after getting the size we know everyone will enter the
  // blocking barrier and the blocking bcast where the buffer is sent out.
  ospcommon::udt::write(connection, &sz32, sizeof(uint32_t));
  // Now do non-blocking test to see when this bcast is satisfied to avoid
  // locking out the send/recv threads
  ospcommon::udt::write(connection, mem, sz32);
  //  ospcommon::flush(socketfd);
}
#endif