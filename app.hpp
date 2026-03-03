#pragma once

#include "window.hpp"
#include "pipeline.hpp"
#include "device.hpp"
#include "renderer.hpp"
#include "model.hpp"
#include "object.hpp"

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
        void loadModels();
        void createPipelineLayout();
        void createPipeline();
        void renderObjects(VkCommandBuffer command_buffer);


        double last_time = glfwGetTime();
        int frame_count = 0;

        void updateFrameRate();

        Window window{WIDTH, HEIGHT, "YAY, Vulkan!"};
        Device device{window};
        Renderer renderer{window, device};
        std::vector<Object> objects;

        std::unique_ptr<Pipeline> pipeline;//{"shaders/first_shader.vert.spv", "shaders/first_shader.frag.spv", device, Pipeline::defaultPipelineConfigInfo(WIDTH, HEIGHT)};
        VkPipelineLayout pipeline_layout;
        // std::unique_ptr<Model> model;
};
    
}  // namespace baka