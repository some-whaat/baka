#pragma once

#include "window.hpp"
#include "pipeline.hpp"
#include "device.hpp"
#include "swap_chain.hpp"
#include "model.hpp"

// std
#include <memory>
#include <vector>

namespace baka {

class App {

    public:
        static constexpr int WIDTH = 1600;
        static constexpr int HEIGHT = 1200;

        App();
        ~App();

        App(const App&) = delete;
        App &operator=(const App&) = delete;

        void run();


    private:
        void loadModels();
        void createPipelineLayout();
        void createPipeline();
        void createCommandBuffers();
        void drawFrame();

        Window window{WIDTH, HEIGHT, "YAY, Vulkan!"};
        Device _device{window};
        SwapChain swap_chain{_device, window.getExtent()};
        std::unique_ptr<Pipeline> pipeline;//{"shaders/first_shader.vert.spv", "shaders/first_shader.frag.spv", device, Pipeline::defaultPipelineConfigInfo(WIDTH, HEIGHT)};
        VkPipelineLayout pipeline_layout;
        std::vector<VkCommandBuffer> command_buffers;
        std::unique_ptr<Model> model;
};
    
}  // namespace baka