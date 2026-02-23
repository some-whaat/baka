#include "window.hpp"

#include <stdexcept>

namespace baka {

    Window::Window(int w, int h, std::string name) : width{w}, height{h}, window_name{name} {
        initWindow();
    }

    Window::~Window() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void Window::initWindow() {
            
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(width, height, window_name.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    void Window::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) {
        if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create serface");
        }
    }

    void Window::changeTitle(std::string new_title) {
        glfwSetWindowTitle(window, new_title.c_str());
    }

    void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        Window* new_window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        new_window->was_framebuffer_resized = true;
        new_window->width = width;
        new_window->height = height;
    }


}  // namespace baka