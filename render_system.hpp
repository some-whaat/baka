// responsble for pipline

#pragma once

#include "pipeline.hpp"
#include "device.hpp"
#include "model.hpp"
#include "object.hpp"
#include "camera.hpp"
#include "material.hpp"
#include "gbuffer.hpp"

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

        glm::vec4 position; // xyz = position, w = padding
        glm::vec4 color; // rgba (w = brightness)
    };

    struct SceneData {
        std::vector<PointLightData> point_lights;
    };

    public:

        float fps = 0;

        RenderSystem(Device &device, VkRenderPass render_pass, VkExtent2D extent = VkExtent2D{800,600});
        ~RenderSystem();

        RenderSystem(const RenderSystem&) = delete;
        RenderSystem &operator=(const RenderSystem&) = delete;

        void renderObjects(VkCommandBuffer command_buffer, std::vector<Object> &objects, const Camera camera, /* TEMPORARY, delete */ double frame_time);
        // split rendering: geometry to GBuffer (must be done outside swapchain render pass)
        void renderGeometry(VkCommandBuffer command_buffer, std::vector<Object> &objects, const Camera camera, double frame_time);
        // lighting/composite pass (must be called while swapchain render pass is active)
        void renderLighting(VkCommandBuffer command_buffer);

        void setupObjectDescriptors(std::vector<Object>& objects);

    private:
        void createPipelineLayout();
        // per-material pipeline creation helper
        Pipeline* getOrCreatePipeline(const std::string& vertFilepath, const std::string& fragFilepath);
        Pipeline* getOrCreatePipeline(const std::string& vertFilepath, const std::string& fragFilepath, VkRenderPass targetRenderPass);

        // Render geometry into the G-buffer
        void renderToGBuffer(VkCommandBuffer command_buffer, std::vector<Object> &objects, const Camera camera, double frame_time);

        std::vector<VkDescriptorSet> descriptorSets;
        VkDescriptorSet sceneDescriptorSet;
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSetLayout sceneDescriptorSetLayout;
        VkDescriptorSetLayout objectDescriptorSetLayout; // for per-object matrices (storage buffer)
        VkDescriptorSetLayout gbufferDescriptorSetLayout;
        VkDescriptorSet gbufferDescriptorSet;
        VkDescriptorPool descriptorPool;
        VkDescriptorSet objectDescriptorSet; // single set containing all object transforms
        VkBuffer object_buffer;
        VkDeviceMemory object_buffer_memory;
        void* object_mapped_data = nullptr;
        void createObjectBuffer(size_t objectCount);
        void createDescriptorSetLayout();
        void createDescriptorPool();
        void createDescriptorSets();

        // lighting (deferred compose) pipeline + layout
        std::unique_ptr<Pipeline> lightingPipeline;
        VkPipelineLayout lightingPipelineLayout;
        void createLightingPipeline(const std::string& vertPath, const std::string& fragPath);
        void createGBufferDescriptorSet();

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
        VkExtent2D swapchainExtent;

        // GBuffer gBuffer;
        std::unique_ptr<GBuffer> gBuffer;
        // lighting pipeline cleanup will be handled in destructor
        // Note: lightingPipelineLayout is destroyed in destructor
};
    
}  // namespace baka