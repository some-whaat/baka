#include "window.hpp"

namespace baka {

    Window::Window(int w, int h, std::string name) : WIDTH{w}, HEIGHT{h}, window_name{name} {
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
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, window_name.c_str(), nullptr, nullptr);
    }

}  // namespace baka