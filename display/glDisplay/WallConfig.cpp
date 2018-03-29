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
#include <fstream>
#include <iostream>

#include <display/glDisplay/glDisplay.h>
#include "WallConfig.h"
#include "ospcommon/utility/getEnvVar.h"

namespace ospray {
  namespace dw {

    wallconfig::wallconfig() : viewerSize(0, 0), viewerNode(false)
    {
      sync();
    }

    void wallconfig::sync()
    {
      if (mpicommon::IamTheMaster()) {
        auto envfilename = utility::getEnvVar<std::string>("DW_CONFIG_FILE");
        auto filename    = envfilename.value_or("default.conf");

        std::ifstream conffile(filename, std::ios::in);
        if (conffile.is_open()) {
          conffile >> localScreen.x >> localScreen.y;
          conffile >> displayConfig.x >> displayConfig.y;
          conffile >> basel_compensation.x >> basel_compensation.y;
          conffile >> orientation;

          // Send data to all workers
          MPI_CALL(Bcast(
              this, sizeof(wallconfig), MPI_BYTE, 0, mpicommon::world.comm));

          localPosition = vec2i(0, 0);

          vec2i aa(0, 0);

          MPI_CALL(Reduce(&aa.x,
                          &localScreen.x,
                          1,
                          MPI_INT,
                          MPI_MAX,
                          0,
                          mpicommon::world.comm));
          MPI_CALL(Reduce(&aa.y,
                          &localScreen.y,
                          1,
                          MPI_INT,
                          MPI_MAX,
                          0,
                          mpicommon::world.comm));

          completeScreeen.x = localScreen.x * displayConfig.x +
                              basel_compensation.x * (displayConfig.x - 1);
          completeScreeen.y = localScreen.y * displayConfig.y +
                              basel_compensation.y * (displayConfig.y - 1);
          viewerSize = localScreen;

          for (int y = 0; y < completeScreeen.y; y += TILE_SIZE)
            for (int x = 0; x < completeScreeen.x; x += TILE_SIZE) {
              TileRankMap[vec2i(x, y)] = std::set<int>();
              TileRankMap[vec2i(x, y)].insert(whichRank(vec2i(x, y)));
              TileRankMap[vec2i(x, y)].insert(
                  whichRank(vec2i(x, y + TILE_SIZE)));
              TileRankMap[vec2i(x, y)].insert(
                  whichRank(vec2i(x + TILE_SIZE, y)));
              TileRankMap[vec2i(x, y)].insert(
                  whichRank(vec2i(x + TILE_SIZE, y + TILE_SIZE)));
            }

        } else {
          throw "Unable to open file : " + filename;
        }
      } else {
        // Send data to all workers
        MPI_CALL(Bcast(
            this, sizeof(wallconfig), MPI_BYTE, 0, mpicommon::world.comm));
        localScreen = dw::glDisplay::getWindowSize();

        // localScreen = vec2i(512,769);

        int x = (orientation == 0) ? (mpicommon::worker.rank % displayConfig.x)
                                   : (mpicommon::worker.rank / displayConfig.x);
        int y = (orientation == 0) ? (mpicommon::worker.rank / displayConfig.x)
                                   : (mpicommon::worker.rank % displayConfig.y);

        screenID = vec2i(x, displayConfig.y - y);

        vec2i localBasel(0, 0);
        if (x > 0 && y < displayConfig.x)
          localBasel.x = basel_compensation.x * x;
        if (y > 0 && y < displayConfig.y)
          localBasel.y = basel_compensation.y * x;

        localPosition.x = localScreen.x * x + localBasel.x;
        localPosition.y = localScreen.y * y + localBasel.y;

        MPI_CALL(Reduce(&localScreen.x,
                        &localScreen.x,
                        1,
                        MPI_INT,
                        MPI_MAX,
                        0,
                        mpicommon::world.comm));
        MPI_CALL(Reduce(&localScreen.y,
                        &localScreen.y,
                        1,
                        MPI_INT,
                        MPI_MAX,
                        0,
                        mpicommon::world.comm));

        completeScreeen.x = localScreen.x * displayConfig.x +
                            basel_compensation.x * (displayConfig.x - 1);
        completeScreeen.y = localScreen.y * displayConfig.y +
                            basel_compensation.y * (displayConfig.y - 1);
        viewerSize = localScreen;
      }

      for (int i = 0; i < mpicommon::world.size; i++) {
        if (mpicommon::world.rank == i) {
          std::cout << "[" << mpicommon::world.rank
                    << "] Configuration: " << std::endl;
          std::cout << "         Display config : " << displayConfig
                    << std::endl;
          std::cout << "         Global screen size : " << completeScreeen
                    << std::endl;
          std::cout << "         Local position : " << localPosition
                    << std::endl;
          std::cout << "         Local screen size : " << localScreen
                    << std::endl;
        }
        mpicommon::world.barrier();
      }
    }

    int wallconfig::displayRank(const int &x, const int &y)
    {
      return (orientation == 0) ? (x + y * displayConfig.x)
                                : (y + x * displayConfig.y);
    }

    int wallconfig::whichRank(const vec2i &pos)
    {
      int x = std::min((int)std::floor(float(pos.x) /
                                       (localScreen.x + basel_compensation.x)),
                       displayConfig.x - 1);
      int y = std::min((int)std::floor(float(pos.y) /
                                       (localScreen.y + basel_compensation.y)),
                       displayConfig.y - 1);
      return displayRank(x, y);
    }

    std::set<int> &wallconfig::getRanks(const vec2i &pos)
    {
      return TileRankMap[pos];
    }

  }  // namespace dw
}  // namespace ospray
