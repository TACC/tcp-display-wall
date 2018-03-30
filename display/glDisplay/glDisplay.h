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

#ifndef OSPRAY_GLDISPLAY_H
#define OSPRAY_GLDISPLAY_H

#include <GLFW/glfw3.h>
#include <atomic>
#include <memory>
#include <thread>

#include <display/fb/DisplayFramebuffer.h>

namespace ospray {
    namespace dw {

        struct glDisplay {
            // Static display manipulation
            //

            // There can only be one instance
            static std::shared_ptr<glDisplay> instance;

            /*  ! Initialize screen
             *
             *
             */
            static void init(const vec2i &size);

            /*  ! Initialize screen
             *
             *
             */
            static void setScreen(const vec2i &size);

            /*  ! Initialize screen
             *
             *
             */
            static void start(const vec2i &screenID);

            /* Is screen initializwd
             *
             *
             */
            static bool initialized();

            /* Load new frame result into texture
             *
             *
             */
            static void loadFrame(const byte_t *map, const vec2i &size);

            static vec2i getWindowSize();

            glDisplay();

            virtual ~glDisplay();

            void splatTextureOnScreen();

            void attachTexture(const int &b);

            void renderLoop(const vec2i &screenID);

            void loadImage(const byte_t *map, const vec2i &size);

            void initialize(const vec2i &size);

        protected:
            void initTexture();

            GLuint texture[2];
            int current_texture;
            GLFWwindow *window = nullptr;

            std::mutex m;

            bool _initialized = false;

            int prev;

            std::vector<byte_t> buffers[2];
            vec2i sizes[2];

            vec2i windowSize;
            vec2i screenID;

            mpicommon::Group displaygroup;
        };
    }  // namespace dw
}  // namespace ospray

#endif  // OSPRAY_GLDISPLAY_H
