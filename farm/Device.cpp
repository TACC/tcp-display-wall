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
#include "Device.h"
#include <mpi/MPOffloadWorker.h>
#include <mpi/common/setup.h>
#include "ospcommon/utility/getEnvVar.h"

#include <future>
#include "work/FarmWork.h"

ospray::dw::farm::Device::~Device() {}

ospray::mpi::work::WorkTypeRegistry &ospray::dw::farm::Device::getWorkRegistry()
{
  return workRegistry;
}

void ospray::dw::farm::Device::initializeDevice()
{
  ospray::api::Device::commit();

  initialized = true;

  farm::registerOSPWorkItems(workRegistry);

  int _ac           = 2;
  const char *_av[] = {"ospray_mpi_worker", "--osp:mpi"};

  std::string mode = getParam<std::string>("mpiMode", "mpi");

  mpicommon::init(&_ac, _av, true);

  postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
      << "#o: initMPI::OSPonRanks: " << mpicommon::world.rank << '/'
      << mpicommon::world.size;

  MPI_CALL(Barrier(mpicommon::world.comm));

  ospray::mpi::throwIfNotMpiParallel();

  if (mpicommon::IamTheMaster())
    ospray::mpi::setupMaster();
  else {
    ospray::mpi::setupWorker();
  }

  if (mpicommon::world.size != 1) {
    if (mpicommon::world.rank < 0) {
      throw std::runtime_error(
          "OSPRay MPI startup error. Use \"mpirun "
          "-n 1 <command>\" when calling an "
          "application that tries to spawn to start "
          "the application you were trying to "
          "start.");
    }
  }

  /* set up fabric and stuff - by now all the communicators should
     be properly set up */
  mpiFabric =
      make_unique<mpicommon::MPIBcastFabric>(mpicommon::worker, MPI_ROOT, 0);
  readStream  = make_unique<networking::BufferedReadStream>(*mpiFabric);
  writeStream = make_unique<networking::BufferedWriteStream>(*mpiFabric);
}

void ospray::dw::farm::Device::commit()
{
  if (!initialized)
    initializeDevice();

  if (mpicommon::IamAWorker())
    ospray::mpi::runWorker(workRegistry);

  if (!tcp_initialized && mpicommon::IamTheMaster()) {
    auto DW_HOSTNAME = utility::getEnvVar<std::string>("DW_HOSTNAME")
                           .value_or(std::string("localhost"));

    auto DW_HOSTPORT = utility::getEnvVar<int>("DW_HOSTPORT").value_or(4444);

    try {
      tcpFabric =
          make_unique<mpicommon::TCPFabric>(DW_HOSTNAME, DW_HOSTPORT, false);
      tcpreadStream  = make_unique<networking::BufferedReadStream>(*tcpFabric);
      tcpwriteStream = make_unique<networking::BufferedWriteStream>(*tcpFabric);
    } catch (std::exception ex) {
      std::cerr << "Unable to connect to display wall at " << DW_HOSTNAME << ":"
                << DW_HOSTPORT << std::endl;
    }
  }

  auto OSPRAY_DYNAMIC_LOADBALANCER =
      utility::getEnvVar<int>("OSPRAY_DYNAMIC_LOADBALANCER");

  auto useDynamicLoadBalancer = getParam<int>(
      "dynamicLoadBalancer", OSPRAY_DYNAMIC_LOADBALANCER.value_or(false));

  auto OSPRAY_PREALLOCATED_TILES =
      utility::getEnvVar<int>("OSPRAY_PREALLOCATED_TILES");

  auto preAllocatedTiles =
      OSPRAY_PREALLOCATED_TILES.value_or(getParam<int>("preAllocatedTiles", 4));

  mpi::work::SetLoadBalancer slbWork(
      ObjectHandle(), useDynamicLoadBalancer, preAllocatedTiles);
  processWork(slbWork);

  bool exit = false;

  while (!exit) {
    auto work = ospray::mpi::readWork(workRegistry, *tcpreadStream);
    auto tag  = typeIdOf(work);

    exit = (tag == typeIdOf<mpi::work::CommandFinalize>());
    processWork(*work, true);
    postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL) << "Finished " << typeString(work);
  }
}

void ospray::dw::farm::Device::sendWorkDisplayWall(mpi::work::Work &work,
                                                   bool flushWriteStream)
{
  auto tag = typeIdOf(work);
  tcpwriteStream->write(&tag, sizeof(tag));
  work.serialize(*tcpwriteStream);
  tcpwriteStream->flush();
}

OSP_REGISTER_DEVICE(ospray::dw::farm::Device, dwfarm);
OSP_REGISTER_DEVICE(ospray::dw::farm::Device, farm);