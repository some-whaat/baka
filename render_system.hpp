// responsble for pipline

#pragma once

#include "pipeline.hpp"
#include "device.hpp"
#include "model.hpp"
#include "object.hpp"
#include "camera.hpp"
#include "gbuffer.hpp"
#include "unibuffer.hpp"
#include "descriptors.hpp"

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace baka {

struct PointLightData {
    glm::vec4 position;
    glm::vec3 velocity;
    glm::vec4 color;
    float live_time;
};

struct SceneData {
    std::vector<PointLightData> point_lights;
};

class RenderSystem {
 public:
    class ParticleSystem {
    public:
        virtual ~ParticleSystem() = default;
        virtual void init() = 0;
        virtual void update(float delta) = 0;
    };

    class FountainParticleSystem : public ParticleSystem {
    public:
        FountainParticleSystem(float spread = 1.5f, float height = 2.5f, float velocity = 3.0f)
            : spread_(spread), height_(height), velocity_(velocity) {}

        void init() override {}
        void update(float delta) override { (void)delta; }

        static std::vector<PointLightData> makePreset(
            size_t count,
            float spread = 1.5f,
            float height = 2.5f,
            float velocity = 3.0f) {
            std::vector<PointLightData> particles;
            particles.reserve(count);

            for (size_t i = 0; i < count; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(std::max<size_t>(1, count));
                const float angle = 6.28318530718f * t;
                PointLightData light{};
                light.position = glm::vec4(std::sin(angle) * spread, 0.0f, std::cos(angle) * spread, 1.0f);
                light.velocity = glm::vec3(std::sin(angle) * velocity, height, std::cos(angle) * velocity);
                light.color = glm::vec4(0.8f + 0.2f * std::sin(angle), 0.5f + 0.5f * std::cos(angle), 1.0f, 1.0f);
                light.live_time = 0.0f;
                particles.push_back(light);
            }

            return particles;
        }

    private:
        float spread_;
        float height_;
        float velocity_;
    };

    float fps = 0;

    RenderSystem(Device &device, VkRenderPass render_pass, VkExtent2D extent = VkExtent2D{800,600});
    ~RenderSystem();

    RenderSystem(const RenderSystem&) = delete;
    RenderSystem &operator=(const RenderSystem&) = delete;

    void renderObjects(VkCommandBuffer command_buffer, std::vector<Object> &objects, const Camera camera, double frame_time);
    void renderGeometry(VkCommandBuffer command_buffer, std::vector<Object> &objects, const Camera camera, double frame_time);
    void renderLighting(VkCommandBuffer command_buffer, const Camera camera);

    void setupObjectDescriptors(std::vector<Object>& objects);

 private:
    void createPipelineLayout();
    Pipeline* getOrCreatePipeline(const std::string& vertFilepath, const std::string& fragFilepath);
    Pipeline* getOrCreatePipeline(const std::string& vertFilepath, const std::string& fragFilepath, VkRenderPass targetRenderPass);

    void renderToGBuffer(VkCommandBuffer command_buffer, std::vector<Object> &objects, const Camera camera, double frame_time);

    std::vector<VkDescriptorSet> descriptorSets;
    VkDescriptorPool descriptorPool;
    std::unique_ptr<DescriptorSetLayout> materialDescriptorSetLayout;
    std::unique_ptr<DescriptorSetLayout> sceneDescriptorSetLayout;
    std::unique_ptr<DescriptorSetLayout> objectDescriptorSetLayout;
    std::unique_ptr<DescriptorSetLayout> gbufferDescriptorSetLayout;
    std::unique_ptr<DescriptorSet> sceneDescriptorSet;
    std::unique_ptr<DescriptorSet> objectDescriptorSet;
    std::unique_ptr<DescriptorSet> gbufferDescriptorSet;
    std::unique_ptr<Buffer> object_buffer;
    VkBuffer object_buffer_raw = VK_NULL_HANDLE;
    VkDeviceMemory object_buffer_memory = VK_NULL_HANDLE;
    void* object_mapped_data = nullptr;
    void createObjectBuffer(size_t objectCount);
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets();

    std::unique_ptr<Pipeline> lightingPipeline;
    VkPipelineLayout lightingPipelineLayout = VK_NULL_HANDLE;
    void createLightingPipeline(const std::string& vertPath, const std::string& fragPath);
    void createGBufferDescriptorSet();

    void createSceneDataBuffer();
    void updateSceneDataBuffer(double frame_time);
    void cleanupSceneDataBuffer();

    float frame_count = 0;

    Device &device;

    SceneData scene_data;
    VkBuffer scene_data_buffer = VK_NULL_HANDLE;
    VkDeviceMemory scene_data_buffer_memory = VK_NULL_HANDLE;
    void* scene_mapped_data = nullptr;

    std::unordered_map<std::string, std::unique_ptr<Pipeline>> material_pipelines;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkRenderPass render_pass;
    VkExtent2D swapchainExtent;

    std::unique_ptr<GBuffer> gBuffer;
};

}  // namespace baka