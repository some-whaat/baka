// responsble for pipline

#pragma once

#include "pipeline.hpp"
#include "device.hpp"
#include "model.hpp"
#include "object.hpp"
#include "camera.hpp"
#include "material.hpp"

// std
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

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
        // per-material pipeline creation helper
        Pipeline* getOrCreatePipeline(const std::string& vertFilepath, const std::string& fragFilepath);

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

        // store pipelines per material shader pair
        std::unordered_map<std::string, std::unique_ptr<Pipeline>> material_pipelines;
        VkPipelineLayout pipeline_layout;
        VkRenderPass render_pass;

        // GBuffer gBuffer;
        // VkRenderPass deferredRenderPass;
        // std::unique_ptr<Pipeline> geometryPipeline;
        // std::unique_ptr<Pipeline> lightingPipeline;
};
    
}  // namespace baka