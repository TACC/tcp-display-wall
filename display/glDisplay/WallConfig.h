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
#ifndef OSPRAY_WALLCONFIG_H
#define OSPRAY_WALLCONFIG_H

#include <common/Managed.h>
#include <ospcommon/vec.h>

#include <map>
#include <set>

namespace ospray {
    namespace dw {

        using namespace ospcommon;

        struct wallconfig : public ManagedObject {
            ~wallconfig() = default;
            wallconfig();
            void sync();
            std::set<int> &getRanks(const vec2i &pos);

            vec2i displayConfig;
            vec2i basel_compensation;
            int orientation;
            vec2i completeScreeen;
            vec2i localScreen;
            vec2i localPosition;
            vec2i screenID;
            vec2i maxTiles;

        protected:
            std::map<vec2i, std::set<int>> TileRankMap;
//            std::vector<vec2i> screensPos;
//            std::vector<int> screensIDs;

            int displayRank(const int &x, const int &y);
        private:
            void whichRanks(std::vector<vec2i> &screensPos, const vec2i &pos);
        };
    }  // namespace dw
}  // namespace ospray

#endif  // OSPRAY_WALLCONFIG_H
