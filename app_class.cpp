#include "app_class.hpp"


namespace baka {

    void App::run() {

        while (!window.shouldClose()) {
            glfwPollEvents();
        }
    }

}  // namespace baka