// responsble for pipline

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

    struct PointLightData {
        // alignas(16) glm::vec3 position;
        // alignas(16) glm::vec4 color; // w is brightness

        glm::vec3 position;
        glm::vec4 color; // w is brightness
    };

    struct SceneData {
        std::vector<PointLightData> point_lights;
    };

    public:

        float fps = 0;

        RenderSystem(Device &device, VkRenderPass render_pass);
        ~RenderSystem();

        RenderSystem(const RenderSystem&) = delete;
        RenderSystem &operator=(const RenderSystem&) = delete;

        void renderObjects(VkCommandBuffer command_buffer, std::vector<Object> &objects, const Camera camera, /* TEMPORARY, delete */ double frame_time);
        void setupObjectDescriptors(std::vector<Object>& objects);

    private:
        void createPipelineLayout();
        void createPipeline(VkRenderPass render_pass);

        std::vector<VkDescriptorSet> descriptorSets;
        VkDescriptorSet sceneDescriptorSet;
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSetLayout sceneDescriptorSetLayout;
        VkDescriptorPool descriptorPool;
        void createDescriptorSetLayout();
        void createDescriptorPool();
        void createDescriptorSets();

        void createSceneDataBuffer();
        void updateSceneDataBuffer();
        void cleanupSceneDataBuffer();
        


        double last_time = glfwGetTime();
        float frame_count = 0;

        void updateFrameRate();

        Device &device;

        SceneData scene_data;
        VkBuffer scene_data_buffer;
        VkDeviceMemory scene_data_buffer_memory;
        void* scene_mapped_data = nullptr;

        std::unique_ptr<Pipeline> pipeline;
        VkPipelineLayout pipeline_layout;
};
    
}  // namespace baka