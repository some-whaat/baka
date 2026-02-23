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
            VkExtent2D getExtent() {return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }
            bool wasWindowResized() {return was_framebuffer_resized; }
            void resetFramebufferResized() {was_framebuffer_resized = false; }


            void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

            void changeTitle(std::string new_title);

            // to avoid memory shenanigans
            Window(const Window &) = delete;
            Window &operator=(const Window &) = delete;

        private:
        
            GLFWwindow *window;  

            void initWindow();

            static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

            int width;
            int height;
            bool was_framebuffer_resized = false;

            std::string window_name;

    };
}