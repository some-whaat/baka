#pragma once

#include "device.hpp"
#include "object.hpp"
#include "pipeline.hpp"

// std
#include <memory>
#include <vector>

namespace baka {

class RenderSystem {

    public:

        RenderSystem(Device &device, VkRenderPass renderPass);
        ~RenderSystem();

        RenderSystem(const RenderSystem &) = delete;
        RenderSystem &operator=(const RenderSystem &) = delete;

        void renderObjects(VkCommandBuffer command_buffer, std::vector<Object> &objects);

    private:

        void createPipelineLayout();
        void createPipeline(VkRenderPass renderPass);

        Device &device;

        std::unique_ptr<Pipeline> pipeline;
        VkPipelineLayout pipeline_layout;
};
}  // namespace baka