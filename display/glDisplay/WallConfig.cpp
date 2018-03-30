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
        template <typename T>
        bool inline inRange(const T &x, const T &lower, const T &upper)
        {
            return (x >= lower && x < upper);
        }

        template <typename T>
        T inline minInRange(const T &x, const T &lower, const T &upper)
        {
            return std::min(std::max(x, lower), upper);
        }

        bool inline tileBelongsTo(const ospcommon::vec2i &tpos,
                                  const ospcommon::vec2i &tsize,
                                  const ospcommon::vec2i &dpos,
                                  const ospcommon::vec2i &dsize)
        {
            bool test = inRange<int>(tpos.x, dpos.x, dpos.x + dsize.x) ||
                        inRange(dpos.x, tpos.x, tpos.x + tsize.x);
            test &= inRange<int>(tpos.y, dpos.y, dpos.y + dsize.y) ||
                    inRange(dpos.y, tpos.y, tpos.y + tsize.y);
            return test;
        }


        inline int tileID(const ospcommon::vec2i &max, const ospcommon::vec2i &tile) {
            return (tile.y * max.x) + tile.x;
        }

        wallconfig::wallconfig() {
            sync();
        }

        void wallconfig::sync() {
            if (mpicommon::IamTheMaster()) {
                auto envfilename = utility::getEnvVar<std::string>("DW_CONFIG_FILE");
                auto filename = envfilename.value_or("default.conf");

                std::ifstream conffile(filename, std::ios::in);
                if (conffile.is_open()) {
                    conffile >> localScreen.x >> localScreen.y;
                    conffile >> displayConfig.x >> displayConfig.y;
                    conffile >> basel_compensation.x >> basel_compensation.y;
                    conffile >> orientation;

                    // Send data to all workers
                    MPI_CALL(Bcast(
                            this, sizeof(wallconfig), MPI_BYTE, 0, mpicommon::world.comm));

                    completeScreeen.x = localScreen.x * displayConfig.x +
                                        basel_compensation.x * (displayConfig.x - 1);
                    completeScreeen.y = localScreen.y * displayConfig.y +
                                        basel_compensation.y * (displayConfig.y - 1);

                    maxTiles = ospcommon::divRoundUp(completeScreeen, ospcommon::vec2i(TILE_SIZE));

                    std::vector<vec2i> screensPos;

                    int ID =0;
                    if (orientation == 0)
                        for (int y = 0; y < completeScreeen.y; y += localScreen.y + basel_compensation.y)
                            for (int x = 0; x < completeScreeen.x; x += localScreen.x + basel_compensation.x) {
                                screensPos.push_back(vec2i(x, y));
                            }
                    else
                        for (int x = 0; x < completeScreeen.x; x += localScreen.x + basel_compensation.x) {
                            for (int y = 0; y < completeScreeen.y; y += localScreen.x + basel_compensation.y)
                                screensPos.push_back(vec2i(x, y));
                            }



                    for (int y = 0; y < completeScreeen.y; y += TILE_SIZE)
                        for (int x = 0; x < completeScreeen.x; x += TILE_SIZE) {
                                whichRanks(screensPos,vec2i(x,y));
                        }

                } else {
                    throw "Unable to open file : " + filename;
                }
            } else {
                // Send data to all workers
                MPI_CALL(Bcast(
                        this, sizeof(wallconfig), MPI_BYTE, 0, mpicommon::world.comm));


                dw::glDisplay::setScreen(localScreen);


                int x = (orientation == 0) ? (mpicommon::worker.rank % displayConfig.x)
                                           : (mpicommon::worker.rank / displayConfig.x);
                int y = (orientation == 0) ? (mpicommon::worker.rank / displayConfig.x)
                                           : (mpicommon::worker.rank % displayConfig.y);

                screenID = vec2i(x, y);

                localPosition.x = localScreen.x * x + basel_compensation.x * x;
                localPosition.y = localScreen.y * y + basel_compensation.y * y;

                completeScreeen.x = localScreen.x * displayConfig.x +
                                    basel_compensation.x * (displayConfig.x - 1);
                completeScreeen.y = localScreen.y * displayConfig.y +
                                    basel_compensation.y * (displayConfig.y - 1);

            }
        }

        int wallconfig::displayRank(const int &x, const int &y) {
            return (orientation == 0) ? (x + y * displayConfig.x)
                                      : (y + x * displayConfig.y);
        }

        void wallconfig::whichRanks(std::vector<vec2i> &screensPos,const vec2i &pos) {
            auto ID = tileID(maxTiles,pos);
            TileRankMap[ID] = std::set<int>();
            for(int i = 0; i < screensPos.size(); i++) {
                if(tileBelongsTo(pos,vec2i(TILE_SIZE), screensPos[i], localScreen)) {
                    TileRankMap[ID].insert(i);
                }
            }
        }

        std::set<int> &wallconfig::getRanks(const vec2i &pos) {
            int ID = tileID(maxTiles, pos);
            return TileRankMap[ID];
        }

    }  // namespace dw
}  // namespace ospray

/*
 *
 * Tile not here 4 : (4096,2560) x (1920,1080) x (3840,2160)
 * Tile not here 1 : (4864,1280) x (1920,0) x (3840,1080)
 * Tile not here 0 : (6912,256) x (1920,0) x (3840,1080)
 *
 *
 */