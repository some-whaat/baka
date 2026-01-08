#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace baka {

    class Window {
        
        public:

            Window(int w, int h, std::string name);
            ~Window();

            bool shouldClose() { return glfwWindowShouldClose(window); }

            Window(const Window &) = delete;
            Window &operator=(const Window &) = delete;

        private:
        
            GLFWwindow *window;  

            void initWindow();

            const int width;
            const int height;

            std::string window_name;

    };
}