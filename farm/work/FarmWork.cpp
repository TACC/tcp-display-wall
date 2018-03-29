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
#include "FarmWork.h"
#include "fb/FarmFramebuffer.h"

void ospray::dw::farm::CreateFrameBuffer::run()
{
  mpi::work::CreateFrameBuffer::run();
}
void ospray::dw::farm::CreateFrameBuffer::runOnMaster()
{
  const bool hasDepthBuffer    = channels & OSP_FB_DEPTH;
  const bool hasAccumBuffer    = channels & OSP_FB_ACCUM;
  const bool hasVarianceBuffer = channels & OSP_FB_VARIANCE;

  assert(dimensions.x > 0);
  assert(dimensions.y > 0);

  FrameBuffer *fb =
      new ospray::dw::farm::DistributedFrameBuffer(dimensions,
                                                   handle,
                                                   format,
                                                   hasDepthBuffer,
                                                   hasAccumBuffer,
                                                   hasVarianceBuffer);

  handle.assign(fb);
}

ospray::dw::farm::CreateFrameBuffer::CreateFrameBuffer(
    ospray::ObjectHandle handle,
    ospcommon::vec2i dimensions,
    OSPFrameBufferFormat format,
    ospray::uint32 channels)
    : mpi::work::CreateFrameBuffer(handle, dimensions, format, channels)
{
}

void ospray::dw::farm::registerOSPWorkItems(
    mpi::work::WorkTypeRegistry &registry)
{
  mpi::work::registerOSPWorkItems(registry);

  // Register common work
  mpi::work::registerWorkUnit<dw::SetTile>(registry);

  // Create a different buffer (instead of writing the buffer just forward)
  mpi::work::registerWorkUnit<dw::farm::CreateFrameBuffer>(registry);
}
