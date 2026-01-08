#pragma once

#include "window.hpp"
#include "pipeline.hpp"
#include "device.hpp"

namespace baka {

    class App {

        public:
            static constexpr int WIDTH = 1600;
            static constexpr int HEIGHT = 1200;

            void run();

        private:
            Window window{WIDTH, HEIGHT, "YAY, Vulkan!"};
            Device device{window};
            Pipeline pipeline{"shaders/first_shader.vert.spv", "shaders/first_shader.frag.spv", device, Pipeline::defaultPipelineConfigInfo(WIDTH, HEIGHT)};
    };
    
}  // namespace baka