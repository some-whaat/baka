#pragma once

#include "pipeline.hpp"
#include "device.hpp"
#include "model.hpp"
#include "object.hpp"
#include "camera.hpp"

// std
#include <memory>
#include <vector>

namespace baka {

class RenderSystem {

    public:

        float fps = 0;

        RenderSystem(Device &device, VkRenderPass render_pass);
        ~RenderSystem();

        RenderSystem(const RenderSystem&) = delete;
        RenderSystem &operator=(const RenderSystem&) = delete;

        void renderObjects(VkCommandBuffer command_buffer, std::vector<Object> &objects);

    private:
        void createPipelineLayout();
        void createPipeline(VkRenderPass render_pass);


        double last_time = glfwGetTime();
        int frame_count = 0;

        void updateFrameRate();

        Device &device;

        std::unique_ptr<Pipeline> pipeline;
        VkPipelineLayout pipeline_layout;
};
    
}  // namespace baka