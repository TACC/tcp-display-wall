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

#include "DWwork.h"

ospray::dw::SetTile::SetTile(ospray::ObjectHandle &handle,
                             const uint64 &size,
                             const byte_t *msg)
    : fbHandle(handle), size(size)
{
  data = (byte_t *)malloc(size);
  std::memcpy(data, msg, size);
}

void ospray::dw::SetTile::runOnMaster()
{
  throw std::runtime_error(
      "Instanced the wrong  SetTile classs check your work resgistry");
}

void ospray::dw::SetTile::run()
{
  throw std::runtime_error(
      "Instanced the wrong  SetTile classs check your work resgistry");
}

void ospray::dw::SetTile::serialize(networking::WriteStream &b) const
{
  b << (int64)fbHandle;
  b << (uint64)size;
  b.write(data, size);
}

void ospray::dw::SetTile::deserialize(networking::ReadStream &b)
{
  b >> fbHandle.i64;
  b >> size;
  data = (byte_t *)malloc(size);
  b.read(data, size);
}

ospray::dw::SetTile::~SetTile()
{
  if (data != nullptr)
    free(data);
}
