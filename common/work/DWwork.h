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

#include <mpi/common/OSPWork.h>
#include <ospray/fb/FrameBuffer.h>
#include <memory>

#include <chrono>

namespace ospray {
  namespace dw {

    struct Buffer
    {
      std::vector<std::shared_ptr<byte_t>> _buffer_data;
      std::vector<size_t> _buffer_size;

      std::mutex _m;

      Buffer() {}


      size_t size() { return _buffer_size.size();}
      size_t clear() { _buffer_size.clear(), _buffer_data.clear(); }

      void addTile(const std::shared_ptr<mpicommon::Message> &msg)
      {
//        std::unique_lock<std::mutex> lock(_m);
        _buffer_size.push_back(msg->size);
        std::shared_ptr<byte_t> tile((byte_t*)std::malloc(msg->size));
        std::memcpy(tile.get(), msg->data, msg->size);
        _buffer_data.push_back(tile);
      }

      size_t buffersize() const
      {
        size_t sum = sizeof(size_t);
        for (auto &m : _buffer_size)
          sum += (sizeof(size_t) + m);
        return sum;
      }

      std::shared_ptr<byte_t> encode()
      {
//        std::unique_lock<std::mutex> lock(_m);
        std::shared_ptr<byte_t> buf((byte_t*)malloc(buffersize()));
        byte_t *pos    = buf.get();
        *(size_t *)pos = _buffer_data.size();
        pos += sizeof(size_t);
        std::memcpy(
            pos, _buffer_size.data(), sizeof(size_t) * _buffer_size.size());
        pos += sizeof(size_t) * _buffer_size.size();

        int p = 0;
        for (auto &m : _buffer_data) {
          std::memcpy(pos, m.get(), _buffer_size[p++]);
          pos += _buffer_size[p-1];
        }
        return buf;
      }

      void decode(byte_t* ptr)
      {

        size_t size = *(size_t *)ptr;
        ptr += sizeof(size_t);

        _buffer_data.clear();
        _buffer_size.resize(size);

        std::memcpy(_buffer_size.data(), ptr, sizeof(size_t) * size);

        ptr += sizeof(size_t) * size;

        for(auto& s : _buffer_size) {
          std::shared_ptr<byte_t> tile((byte_t*)std::malloc(s));
          std::memcpy(tile.get(),ptr,s);
          _buffer_data.push_back(tile);
          ptr += s;
        }

      }
    };



    struct SetTile : public mpi::work::Work
    {
      SetTile() {
      }

      SetTile(ospray::ObjectHandle &handle,
              const uint64 &size,
              const byte_t *msg);
      ~SetTile() override;
      virtual void run() override;
      virtual void runOnMaster() override;
      void serialize(networking::WriteStream &b) const;
      void deserialize(networking::ReadStream &b) override;

     protected:
      ospray::ObjectHandle fbHandle;
      uint64 size;
      byte_t *data;
    };

  }  // namespace dw
}  // namespace ospray