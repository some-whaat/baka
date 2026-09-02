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
#include <algorithm>
#include <cstdlib>
#include <ctime>

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
        virtual std::vector<PointLightData>& getParticles() = 0;
    };

    class FountainParticleSystem : public ParticleSystem {
    public:
        FountainParticleSystem(
            glm::vec4 emmision_pos = glm::vec4(0.0f, 0.0f, 3.0f, 1.0f),
            float spread = 1.5f,
            float height = 2.5f,
            float velocity = 3.0f)
            : emmision_pos_(emmision_pos), spread_(spread), height_(height), velocity_(velocity) {
            std::srand(static_cast<unsigned>(std::time(nullptr)));
            init();
        }

        void init() override {
            particles_ = makePreset(33, spread_, height_, velocity_);
        }

        std::vector<PointLightData>& getParticles() override {
            return particles_;
        }

        void update(float delta) override {
            const float gravity = 4.0f;
            const float drag = 0.985f;
            const float lifetime = 3.0f;

            for (auto &particle : particles_) {
                particle.live_time += delta;

                if (particle.live_time >= lifetime) {
                    const float angle = 6.28318530718f * static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                    const float offset = spread_ * (0.25f + 0.75f * static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
                    
                    particle.position = emmision_pos_ + glm::vec4(
                        std::sin(angle) * offset,
                        0.0f,
                        std::cos(angle) * offset,
                        1.0f);

                    particle.velocity = glm::vec3(
                        std::sin(angle) * velocity_,
                        height_,
                        std::cos(angle) * velocity_);

                    particle.color = glm::vec4(
                        0.8f + 0.2f * std::sin(angle),
                        0.5f + 0.5f * std::cos(angle),
                        1.0f,
                        1.0f);
                        
                    particle.live_time = 0.0f;
                }

                particle.velocity.y += gravity * delta;
                particle.velocity *= drag;
                particle.position += glm::vec4(particle.velocity * delta, 0.0f);
            }
        }

        std::vector<PointLightData> makePreset(
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
                light.position = emmision_pos_ + glm::vec4(std::sin(angle) * spread, 0.0f, std::cos(angle) * spread, 1.0f);
                light.velocity = glm::vec3(std::sin(angle) * velocity, height, std::cos(angle) * velocity);
                light.color = glm::vec4(0.8f + 0.2f * std::sin(angle), 0.5f + 0.5f * std::cos(angle), 1.0f, 1.0f);
                light.live_time = static_cast<float>(i) * 0.1f;
                particles.push_back(light);
            }

            return particles;
        }

    private:
        float spread_;
        float height_;
        float velocity_;
        glm::vec4 emmision_pos_; // todo it better to be vec3
        std::vector<PointLightData> particles_; 
    };

    float fps = 0;

    RenderSystem(
        Device &device,
        VkRenderPass render_pass,
        std::unique_ptr<ParticleSystem> particle_system = nullptr,
        VkExtent2D extent = VkExtent2D{800,600});
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
    std::unique_ptr<ParticleSystem> particle_system;
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