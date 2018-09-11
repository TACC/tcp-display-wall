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

#include "TCPFabric.h"

#include <sys/ioctl.h>

#include <chrono>

#if defined(DW_USE_SNAPPY)
#include <snappy.h>
#elif defined(DW_USE_DENSITY)
#include <density_api.h>
#include "OSPConfig.h"
constexpr uint_fast64_t lion_packet_size = TILE_SIZE * TILE_SIZE * 4;

static uint64_t rcount = 0;
static uint64_t scount = 0;

#else

#endif

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
#if defined(DW_USE_SNAPPY)

  size_t TCPFabric::read(void *&mem)
  {
    uint32_t sz32 = 0;
    // Get the size of the bcast being sent to us. Only this part must be
    // non-blocking, after getting the size we know everyone will enter the
    // blocking barrier and the blocking bcast where the buffer is sent out.
    ospcommon::read(connection, &sz32, sizeof(uint32_t));
    // TODO: Maybe at some point we should dump the buffer if it gets really
    // large
    char *compressed_data_block = new char[sz32];
    ospcommon::read(connection, compressed_data_block, sz32);
    size_t compressed_data_size;
    snappy::GetUncompressedLength(
        compressed_data_block, sz32, &compressed_data_size);
    buffer.resize(compressed_data_size);
    mem = buffer.data();
    snappy::RawUncompress(compressed_data_block, sz32, (char *)mem);
    delete[] compressed_data_block;
    return compressed_data_size;
  }

  void TCPFabric::send(const void *mem, size_t size)
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
    ospcommon::write(connection, &compressed_data_size, sizeof(uint32_t));
    // Now do non-blocking test to see when this bcast is satisfied to avoid
    // locking out the send/recv threads
    ospcommon::write(connection, compressed_data_block, compressed_data_size);
    ospcommon::flush(connection);

    delete[] compressed_data_block;
  }
#elif defined(DW_USE_DENSITY)
  size_t TCPFabric::read(void *&mem)
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

    ospcommon::read(connection, &sz32, sizeof(uint_fast64_t));
    ospcommon::read(connection, &compress_size, sizeof(uint_fast64_t));
    byte_t *outCompressed = new byte_t[compress_size];
    ospcommon::read(connection, outCompressed, compress_size);

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
    if (decompress_safe_size >= lion_packet_size) {
      std::chrono::high_resolution_clock::time_point tfinish_compression =
        std::chrono::high_resolution_clock::now();

      auto decompression_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(
          tfinish_compression - tfinish_read)
          .count();
      auto decompression_time_seconds =
        std::chrono::duration_cast<std::chrono::seconds>(tfinish_compression -
          tfinish_read)
          .count();
      auto read_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        tfinish_read - tstart_read)
        .count();
      auto read_time_seconds = std::chrono::duration_cast<std::chrono::seconds>(
        tfinish_read - tstart_read)
        .count();
      std::cout << "[ " << rcount++ << " ] Decompressiom ratio : "
                << (float(result.bytesWritten) / compress_size) << " "
                << result.bytesWritten << " /  " << compress_size << " ";
      std::cout << "Time decompression : " << decompression_time << "ms ("
                << decompression_time_seconds << "s) send: " << read_time
                << "ms (" << read_time_seconds << "s)" << std::endl;
    }
#endif

    delete[] outCompressed;
    return result.bytesWritten;
  }

  void TCPFabric::send(const void *mem, size_t size)
  {
    assert(size < (1LL << 30));
    uint_fast64_t sz32               = size;
    uint_fast64_t compress_safe_size = density_compress_safe_size(sz32);

    DENSITY_ALGORITHM compression = DENSITY_ALGORITHM_CHEETAH;
//    if (compress_safe_size >= lion_packet_size) {
//      compression = DENSITY_ALGORITHM_CHAMELEON;
//    }

#ifdef DENSITY_MEASURE_TIMES
    std::chrono::high_resolution_clock::time_point tstart_compression =
        std::chrono::high_resolution_clock::now();
#endif

    byte_t *outCompressed = new byte_t[compress_safe_size];
    density_processing_result result;
    result = density_compress(
        (uint8_t *)mem, sz32, outCompressed, compress_safe_size, compression);

    if (result.state != DENSITY_STATE_OK) {
      printf("[Compression] Compression %llu bytes to %llu bytes\n",
             result.bytesRead,
             result.bytesWritten);
      delete[] outCompressed;
      throw std::runtime_error("Error compressing data");
    }

#ifdef DENSITY_MEASURE_TIMES
    std::chrono::high_resolution_clock::time_point tfinish_compression =
        std::chrono::high_resolution_clock::now();
#endif

    ospcommon::write(connection, &sz32, sizeof(uint_fast64_t));
    ospcommon::write(connection, &result.bytesWritten, sizeof(uint_fast64_t));
    ospcommon::write(connection, outCompressed, result.bytesWritten);
    ospcommon::flush(connection);

#ifdef DENSITY_MEASURE_TIMES
    if (compress_safe_size >= lion_packet_size) {
      std::chrono::high_resolution_clock::time_point tfinish_send =
        std::chrono::high_resolution_clock::now();

      auto compression_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(
          tfinish_compression - tstart_compression)
          .count();
      auto compression_time_seconds =
        std::chrono::duration_cast<std::chrono::seconds>(tfinish_compression -
          tstart_compression)
          .count();
      auto send_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        tfinish_send - tfinish_compression)
        .count();
      auto send_time_seconds = std::chrono::duration_cast<std::chrono::seconds>(
        tfinish_send - tfinish_compression)
        .count();
      std::cout << "[ " << scount++ << " ] Compression ratio : "
                << (float(sz32) / result.bytesWritten) << " "
                << sz32 << " /  " << result.bytesWritten << " ";
      std::cout << "Time compression : " << compression_time << "ms ("
                << compression_time_seconds << "s) send: " << send_time << "ms ("
                << send_time_seconds << "s)" << std::endl;
    }
#endif

    delete[] outCompressed;
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
    buffer.resize(sz32);
    mem = buffer.data();
    ospcommon::read(connection, mem, sz32);
    return sz32;
  }

    void TCPFabric::send(const void *mem, size_t size)
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
#endif


  bool TCPFabric::hasData() {
    int fd = (int)*((int*)connection);
    int count = 0;
    if(ioctl(fd,FIONREAD,&count) < 0) throw std::runtime_error("TCP connection dropped");
    return (count > 0);
  }

}  // namespace mpicommon