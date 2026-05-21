#pragma once

#include "window.hpp"
#include "device.hpp"
#include "renderer.hpp"
#include "model.hpp"
#include "object.hpp"
#include "camera.hpp"
#include "texture.hpp"

// std
#include <memory>
#include <vector>



namespace baka {

class App {

    public:
        static constexpr int WIDTH = 2000;
        static constexpr int HEIGHT = 2000;

        App();
        ~App();

        App(const App&) = delete;
        App &operator=(const App&) = delete;

        void run();


    private:
        void loadObjects();

        void updateFrameRate();

        Window window{WIDTH, HEIGHT, "YAY, Vulkan!"};
        Device device{window};
        Renderer renderer{window, device};
        std::vector<Object> objects;
        Camera camera{};
};
    
}  // namespace baka