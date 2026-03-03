#pragma once

#include "window.hpp"
#include "pipeline.hpp"
#include "device.hpp"
#include "swap_chain.hpp"
#include "model.hpp"
#include "object.hpp"
#include "render_system.hpp"
#include "renderer.hpp"

// std
#include <memory>
#include <vector>

namespace baka {

class App {

    public:
        static constexpr int WIDTH = 1600;
        static constexpr int HEIGHT = 1200;

        float fps = 0;

        App();
        ~App();

        App(const App&) = delete;
        App &operator=(const App&) = delete;

        void run();


    private:
        void loadObjs();
        void createPipelineLayout();
        void createPipeline();
        void createCommandBuffers();
        void drawFrame();
        void recreateSwapChain();
        void recordCommandBuffer(int image_index);

        double last_time = glfwGetTime();
        int frame_count = 0;

        void updateFrameRate();

        Window window{WIDTH, HEIGHT, "YAY, Vulkan!"};
        Device _device{window};
        Renderer renderer{window, _device};
        // std::unique_ptr<SwapChain> swap_chain;
        // std::unique_ptr<Pipeline> pipeline;//{"shaders/first_shader.vert.spv", "shaders/first_shader.frag.spv", device, Pipeline::defaultPipelineConfigInfo(WIDTH, HEIGHT)};
        // VkPipelineLayout pipeline_layout;
        // std::vector<VkCommandBuffer> command_buffers;

        std::vector<Object> objects;
};
    
}  // namespace baka