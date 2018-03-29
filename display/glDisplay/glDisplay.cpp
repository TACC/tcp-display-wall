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

#include "glDisplay.h"
#include <GL/gl.h>
#include "ospcommon/utility/getEnvVar.h"

namespace ospray {
  namespace dw {

    std::shared_ptr<glDisplay> glDisplay::instance = nullptr;

    void glDisplay::init(const vec2i &size)
    {
      if (instance) {
        throw "Trying to init the display again";
      }
      instance = std::make_shared<glDisplay>();
      instance->initialize(size);
    }

    void glDisplay::start(const vec2i &ID)
    {
      if (!instance and !instance->initialized())
        throw "Display not initialized";
      instance->renderLoop(ID);
    }

    bool glDisplay::initialized()
    {
      return (instance != nullptr && instance->_initialized);
    }

    void glDisplay::loadFrame(const byte_t *map, const vec2i &size)
    {
      instance->loadImage(map, size);
    }

    vec2i glDisplay::getWindowSize()
    {
      return instance->windowSize;
    }

    glDisplay::glDisplay() : _initialized(false)
    {
      displaygroup = mpicommon::worker.dup();
    }

    void glDisplay::initialize(const vec2i &size)
    {
      std::lock_guard<std::mutex> l(m);
      if (!glfwInit()) {
        throw "Unable to initialize GLFW context";
      }

      // Setup window
      auto error_callback = [](int error, const char *description) {
        fprintf(stderr, "Error %d: %s\n", error, description);
      };

      glfwSetErrorCallback(error_callback);

      if (!glfwInit())
        throw std::runtime_error("Could not initialize glfw!");

      auto FULLSCREEN = utility::getEnvVar<int>("DW_FULLSCREEN").value_or(1);

      vec2i wsize = size;

      if (FULLSCREEN) {
        const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

        wsize.x = mode->width;
        wsize.y = mode->height;
      }
      window =
          glfwCreateWindow(wsize.x,
                           wsize.y,
                           "Window",
                           (FULLSCREEN) ? glfwGetPrimaryMonitor() : nullptr,
                           nullptr);

      if (window == nullptr) {
        glfwTerminate();
        throw "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.";
      }

      windowSize = wsize;

      glfwMakeContextCurrent(window);

      if (FULLSCREEN) {
        const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwSetWindowMonitor(window,
                             glfwGetPrimaryMonitor(),
                             0,
                             0,
                             wsize.x,
                             wsize.y,
                             mode->refreshRate);
      }

      initTexture();
      _initialized = true;
    }

    void glDisplay::renderLoop(const vec2i &ID)
    {
      screenID          = ID;
      std::string title = "Display (" + std::to_string(screenID.x) + "," +
                          std::to_string(screenID.y) + ")";
      glfwSetWindowTitle(window, title.c_str());
      int w, h;
      glfwGetWindowSize(window, &w, &h);
      glfwSetWindowPos(window, w * screenID.x, h * screenID.y);
      prev = 0;
      glfwSwapInterval(10);
      do {
        if (prev != current_texture) {
          std::lock_guard<std::mutex> l(m);
          prev = current_texture;
          attachTexture(prev);
        }
        splatTextureOnScreen();
      } while (true);
    }

    void glDisplay::initTexture()
    {
      glEnable(GL_TEXTURE_2D);

      for (int t = 0; t < 2; t++) {
        sizes[t] = vec2i(windowSize.x, windowSize.y);
        buffers[t].resize(sizes[t].x * sizes[t].y * 4);

        glGenTextures(1, &texture[t]);
        glBindTexture(GL_TEXTURE_2D, texture[t]);
        std::fill(buffers[t].begin(), buffers[t].end(), 0x00);

        for (int i = 0; i < buffers[t].size(); i += 4) {
          buffers[t][i + 0] = 0x55;
          buffers[t][i + 1] = 0x55;
          buffers[t][i + 2] = 0x55;
          buffers[t][i + 3] = 0xff;
        }

        // set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // set texture content
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA8,
                     sizes[t].x,
                     sizes[t].y,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     &(buffers[t][0]));
        glDisable(GL_TEXTURE_2D);
      }
    }

    glDisplay::~glDisplay()
    {
      std::lock_guard<std::mutex> l(m);
      glfwDestroyWindow(window);
      glfwTerminate();
    }

    void glDisplay::splatTextureOnScreen()
    {
      int width, height;
      glfwGetFramebufferSize(window, &width, &height);
      glViewport(0, 0, width, height);
      glClear(GL_COLOR_BUFFER_BIT);
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(-1.f, 1.f, -1.f, 1.f, 1.f, -1.f);
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glClearColor(0.f, 0.f, 0.f, 1.f);
      glClear(GL_COLOR_BUFFER_BIT);
      glBindTexture(GL_TEXTURE_2D, texture[current_texture & 1]);
      glEnable(GL_TEXTURE_2D);
      glBegin(GL_QUADS);
      glTexCoord2f(0.f, 0.f);
      glVertex3f(-1.f, -1.f, 0.f);
      glTexCoord2f(0.f, 1.f);
      glVertex3f(-1.f, 1.f, 0.f);
      glTexCoord2f(1.f, 1.f);
      glVertex3f(1.f, 1.f, 0.f);
      glTexCoord2f(1.f, 0.f);
      glVertex3f(1.f, -1.f, 0.f);
      glEnd();
      glDisable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, 0);
      displaygroup.barrier();
      glfwSwapBuffers(window);
      //      displaygroup.barrier();
    }

    void glDisplay::loadImage(const byte_t *map, const vec2i &size)
    {
      std::lock_guard<std::mutex> l(m);
      int n = (current_texture + 1) & 1;
      buffers[n].resize(size.x * size.y * 4);
      std::memcpy(&buffers[n][0], map, size.x * size.y * 4);
      sizes[n]        = size;
      current_texture = n;
      displaygroup.barrier();
    }

    void glDisplay::attachTexture(const int &b)
    {
      glDeleteTextures(1, &texture[b & 1]);
      glGenTextures(1, &texture[b & 1]);
      // bind the texture
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, texture[b & 1]);
      // set texture parameters
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      // set texture content
      byte_t *img = buffers[b & 1].data();
      glTexImage2D(GL_TEXTURE_2D,
                   0,
                   GL_RGBA8,
                   sizes[b & 1].x,
                   sizes[b & 1].y,
                   0,
                   GL_RGBA,
                   GL_UNSIGNED_BYTE,
                   img);
      glDisable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
  }  // namespace dw
}  // namespace ospray