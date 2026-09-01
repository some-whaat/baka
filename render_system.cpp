#include "render_system.hpp"

// std
#include <array>
#include <stdexcept>
#include <cstring>
#include <random>
#include <iostream>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "material.hpp"

namespace baka {

struct PushConstantData {
  glm::mat4 projection_view{1.f};
  uint32_t objectIndex;
  float time;
  float _pad0;
};

struct PushConstantDataJustMatrix {
  glm::mat4 projection_view{1.f};
};

void RenderSystem::createObjectBuffer(size_t objectCount) {
  if (object_buffer) {
    object_buffer->cleanUp();
    object_buffer.reset();
  }

  if (objectCount == 0) {
    objectDescriptorSet.reset();
    return;
  }

  const VkDeviceSize bufferSize = objectCount * sizeof(glm::mat4);
  device.createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      object_buffer_raw,
      object_buffer_memory);

  if (object_buffer_raw == VK_NULL_HANDLE || object_buffer_memory == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to allocate object buffer storage!");
  }

  if (vkMapMemory(device.getDevice(), object_buffer_memory, 0, bufferSize, 0, &object_mapped_data) != VK_SUCCESS) {
    throw std::runtime_error("failed to map object buffer memory!");
  }

  objectDescriptorSet = std::make_unique<DescriptorSet>(device, descriptorPool, objectDescriptorSetLayout->get());
  objectDescriptorSet->updateBuffer(0, object_buffer_raw, bufferSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
}

RenderSystem::RenderSystem(Device &_device, VkRenderPass render_pass, VkExtent2D extent)
    : device{_device}, render_pass(render_pass), swapchainExtent(extent) {
  createDescriptorSetLayout();
  createDescriptorPool();

  createPipelineLayout();
  gBuffer = std::make_unique<GBuffer>(device);
  gBuffer->create(extent);

  createObjectBuffer(16);

  try {
    createGBufferDescriptorSet();
  } catch (const std::exception &) {
    gbufferDescriptorSet.reset();
  }

  try {
    createLightingPipeline("shaders/lighting.vert.spv", "shaders/lighting.frag.spv");
  } catch (const std::exception &) {
    lightingPipeline.reset();
    if (lightingPipelineLayout != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(device.getDevice(), lightingPipelineLayout, nullptr);
      lightingPipelineLayout = VK_NULL_HANDLE;
    }
  }
}

RenderSystem::~RenderSystem() {
  lightingPipeline.reset();

  if (lightingPipelineLayout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device.getDevice(), lightingPipelineLayout, nullptr);
    lightingPipelineLayout = VK_NULL_HANDLE;
  }

  if (pipeline_layout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device.getDevice(), pipeline_layout, nullptr);
    pipeline_layout = VK_NULL_HANDLE;
  }

  if (descriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device.getDevice(), descriptorPool, nullptr);
    descriptorPool = VK_NULL_HANDLE;
  }

  cleanupSceneDataBuffer();

  if (object_mapped_data) {
    vkUnmapMemory(device.getDevice(), object_buffer_memory);
    object_mapped_data = nullptr;
  }

  if (object_buffer_raw != VK_NULL_HANDLE) {
    vkDestroyBuffer(device.getDevice(), object_buffer_raw, nullptr);
    object_buffer_raw = VK_NULL_HANDLE;
  }

  if (object_buffer_memory != VK_NULL_HANDLE) {
    vkFreeMemory(device.getDevice(), object_buffer_memory, nullptr);
    object_buffer_memory = VK_NULL_HANDLE;
  }
}

void RenderSystem::setupObjectDescriptors(std::vector<Object>& objects) {
  for (auto& obj : objects) {
    if (!obj.material) continue;

    obj.material->createDescriptorSet(device.getDevice(), descriptorPool, materialDescriptorSetLayout->get());
    obj.descriptor_set = obj.material->getDescriptorSet();
    getOrCreatePipeline(obj.material->getVertPath(), obj.material->getFragPath());
  }

  createSceneDataBuffer();
  createObjectBuffer(objects.size());

  if (object_mapped_data) {
    for (size_t i = 0; i < objects.size(); ++i) {
      glm::mat4 m = objects[i].transform.getMat4();
      std::memcpy(reinterpret_cast<char*>(object_mapped_data) + i * sizeof(glm::mat4), &m, sizeof(glm::mat4));
    }
  }
}

Pipeline* RenderSystem::getOrCreatePipeline(const std::string& vertFilepath, const std::string& fragFilepath) {
  return getOrCreatePipeline(vertFilepath, fragFilepath, render_pass);
}

Pipeline* RenderSystem::getOrCreatePipeline(const std::string& vertFilepath, const std::string& fragFilepath, VkRenderPass targetRenderPass) {
  std::string key = vertFilepath + "|" + fragFilepath + "|" + std::to_string((uint64_t)targetRenderPass);
  auto it = material_pipelines.find(key);
  if (it != material_pipelines.end()) {
    return it->second.get();
  }

  PipelineConfigInfo pipelineConfig{};
  Pipeline::defaultPipelineConfigInfo(pipelineConfig);
  pipelineConfig.renderPass = targetRenderPass;
  pipelineConfig.pipelineLayout = pipeline_layout;

  if (gBuffer && targetRenderPass == gBuffer->getRenderPass()) {
    pipelineConfig.colorBlendAttachments.clear();
    for (int i = 0; i < 3; ++i) {
      pipelineConfig.colorBlendAttachments.push_back(pipelineConfig.colorBlendAttachment);
    }
  }

  auto pipelinePtr = std::make_unique<Pipeline>(device, vertFilepath, fragFilepath, pipelineConfig);
  Pipeline* raw = pipelinePtr.get();
  material_pipelines.emplace(key, std::move(pipelinePtr));
  return raw;
}

void RenderSystem::renderToGBuffer(VkCommandBuffer command_buffer, std::vector<Object> &objects, const Camera camera, double frame_time) {
  if (!gBuffer) return;

  gBuffer->beginGeometryPass(command_buffer, swapchainExtent);
  updateSceneDataBuffer(frame_time);

  const glm::mat4 projection_view = camera.getProjection() * camera.getView();

  for (size_t i = 0; i < objects.size(); ++i) {
    auto &obj = objects[i];
    if (!obj.material) continue;

    Pipeline* p = getOrCreatePipeline(obj.material->getVertPath(), obj.material->getFragPath(), gBuffer->getRenderPass());
    p->bind(command_buffer);

    std::array<VkDescriptorSet, 3> descriptorSets = {
        obj.descriptor_set,
        sceneDescriptorSet->get(),
        objectDescriptorSet->get()};

    vkCmdBindDescriptorSets(
        command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout,
        0,
        static_cast<uint32_t>(descriptorSets.size()),
        descriptorSets.data(),
        0,
        nullptr);

    frame_count += frame_time;
    PushConstantData push{};
    push.projection_view = projection_view;
    push.objectIndex = static_cast<uint32_t>(i);
    push.time = frame_count;

    vkCmdPushConstants(
        command_buffer,
        pipeline_layout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PushConstantData),
        &push);

    obj.model->bind(command_buffer);
    obj.model->draw(command_buffer);
  }

  gBuffer->endGeometryPass(command_buffer);
}

void RenderSystem::renderGeometry(VkCommandBuffer command_buffer, std::vector<Object> &objects, const Camera camera, double frame_time) {
  renderToGBuffer(command_buffer, objects, camera, frame_time);
}

void RenderSystem::renderLighting(VkCommandBuffer command_buffer, const Camera camera) {
  if (!lightingPipeline || !gbufferDescriptorSet || !sceneDescriptorSet) return;

  lightingPipeline->bind(command_buffer);
  std::array<VkDescriptorSet, 2> sets = { gbufferDescriptorSet->get(), sceneDescriptorSet->get() };
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lightingPipelineLayout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);

  PushConstantDataJustMatrix pc{};
  pc.projection_view = camera.getProjection() * camera.getView();
  vkCmdPushConstants(command_buffer, lightingPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);

  vkCmdDraw(command_buffer, 3, 1, 0, 0);
}

void RenderSystem::renderObjects(VkCommandBuffer command_buffer, std::vector<Object> &objects, const Camera camera, double frame_time) {
  renderToGBuffer(command_buffer, objects, camera, frame_time);
  renderLighting(command_buffer, camera);
}

void RenderSystem::createPipelineLayout() {
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PushConstantData);

  std::array<VkDescriptorSetLayout, 3> descriptorSetLayouts = {
      materialDescriptorSetLayout->get(),
      sceneDescriptorSetLayout->get(),
      objectDescriptorSetLayout->get()};

  VkPipelineLayoutCreateInfo pipeline_layoutInfo{};
  pipeline_layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
  pipeline_layoutInfo.pSetLayouts = descriptorSetLayouts.data();
  pipeline_layoutInfo.pushConstantRangeCount = 1;
  pipeline_layoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(device.getDevice(), &pipeline_layoutInfo, nullptr, &pipeline_layout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }
}

void RenderSystem::createDescriptorSetLayout() {
  std::vector<VkDescriptorSetLayoutBinding> samplerBindings(3);
  for (uint32_t i = 0; i < 3; ++i) {
    samplerBindings[i].binding = i;
    samplerBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBindings[i].descriptorCount = 1;
    samplerBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerBindings[i].pImmutableSamplers = nullptr;
  }
  materialDescriptorSetLayout = std::make_unique<DescriptorSetLayout>(device, samplerBindings);

  VkDescriptorSetLayoutBinding sceneDataBinding{};
  sceneDataBinding.binding = 0;
  sceneDataBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  sceneDataBinding.descriptorCount = 1;
  sceneDataBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  sceneDataBinding.pImmutableSamplers = nullptr;
  sceneDescriptorSetLayout = std::make_unique<DescriptorSetLayout>(device, std::vector<VkDescriptorSetLayoutBinding>{sceneDataBinding});

  VkDescriptorSetLayoutBinding objectBinding{};
  objectBinding.binding = 0;
  objectBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  objectBinding.descriptorCount = 1;
  objectBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  objectBinding.pImmutableSamplers = nullptr;
  objectDescriptorSetLayout = std::make_unique<DescriptorSetLayout>(device, std::vector<VkDescriptorSetLayoutBinding>{objectBinding});

  std::vector<VkDescriptorSetLayoutBinding> gBindings(3);
  for (uint32_t i = 0; i < 3; ++i) {
    gBindings[i].binding = i;
    gBindings[i].descriptorCount = 1;
    gBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    gBindings[i].pImmutableSamplers = nullptr;
    gBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  }
  gbufferDescriptorSetLayout = std::make_unique<DescriptorSetLayout>(device, gBindings);
}

void RenderSystem::createDescriptorPool() {
  std::array<VkDescriptorPoolSize, 2> poolSizes{};

  poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[0].descriptorCount = 256;

  poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[1].descriptorCount = 32;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = 256;

  if (vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

const int lights_am = 33;

void RenderSystem::createSceneDataBuffer() {
  std::random_device rd;
  std::mt19937 rng(rd());
  std::uniform_real_distribution<float> dist(0.5f, 1.0f);

  scene_data.point_lights.resize(lights_am);
  for (int i = 0; i < lights_am; ++i) {
    scene_data.point_lights[i].position = glm::vec4(0.f, 0.f, 2.f, 1.f);
    scene_data.point_lights[i].velocity = glm::normalize(glm::vec3(dist(rng), dist(rng), dist(rng)));
    scene_data.point_lights[i].color = glm::vec4(scene_data.point_lights[i].velocity, 1.f);
    scene_data.point_lights[i].live_time = 0.f;
  }

  const VkDeviceSize bufferSize = sizeof(PointLightData) * scene_data.point_lights.size();

  device.createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      scene_data_buffer,
      scene_data_buffer_memory);

  if (vkMapMemory(device.getDevice(), scene_data_buffer_memory, 0, bufferSize, 0, &scene_mapped_data) != VK_SUCCESS) {
    throw std::runtime_error("failed to map scene data buffer memory!");
  }

  std::memcpy(scene_mapped_data, scene_data.point_lights.data(), static_cast<size_t>(bufferSize));

  sceneDescriptorSet = std::make_unique<DescriptorSet>(device, descriptorPool, sceneDescriptorSetLayout->get());
  sceneDescriptorSet->updateBuffer(0, scene_data_buffer, bufferSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
}

void RenderSystem::updateSceneDataBuffer(double frame_time) {
  if (scene_data.point_lights.empty()) return;

  const VkDeviceSize bufferSize = sizeof(PointLightData) * scene_data.point_lights.size();

  for (int i = 0; i < lights_am; ++i) {
    scene_data.point_lights[i].live_time += static_cast<float>(frame_time);
    scene_data.point_lights[i].position.x += (scene_data.point_lights[i].color.x - 0.5f) * 0.001f;
    scene_data.point_lights[i].position.z += (scene_data.point_lights[i].color.z - 0.5f) * 0.001f;
    scene_data.point_lights[i].position.y = std::sin(scene_data.point_lights[i].live_time) * 0.1f;
  }

  if (scene_mapped_data) {
    std::memcpy(scene_mapped_data, scene_data.point_lights.data(), static_cast<size_t>(bufferSize));
  }
}

void RenderSystem::cleanupSceneDataBuffer() {
  if (scene_mapped_data) {
    vkUnmapMemory(device.getDevice(), scene_data_buffer_memory);
    scene_mapped_data = nullptr;
  }

  if (scene_data_buffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(device.getDevice(), scene_data_buffer, nullptr);
    scene_data_buffer = VK_NULL_HANDLE;
  }

  if (scene_data_buffer_memory != VK_NULL_HANDLE) {
    vkFreeMemory(device.getDevice(), scene_data_buffer_memory, nullptr);
    scene_data_buffer_memory = VK_NULL_HANDLE;
  }

  sceneDescriptorSet.reset();
}

void RenderSystem::createGBufferDescriptorSet() {
  if (!gBuffer) return;

  gbufferDescriptorSet = std::make_unique<DescriptorSet>(device, descriptorPool, gbufferDescriptorSetLayout->get());

  auto views = gBuffer->getColorImageViews();
  for (uint32_t i = 0; i < 3; ++i) {
    gbufferDescriptorSet->updateImage(i, views[i], gBuffer->getSampler());
  }
}

void RenderSystem::createLightingPipeline(const std::string& vertPath, const std::string& fragPath) {
  std::array<VkDescriptorSetLayout, 2> layouts = {
      gbufferDescriptorSetLayout->get(),
      sceneDescriptorSetLayout->get()};

  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
  layoutInfo.pSetLayouts = layouts.data();

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PushConstantDataJustMatrix);

  layoutInfo.pushConstantRangeCount = 1;
  layoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(device.getDevice(), &layoutInfo, nullptr, &lightingPipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create lighting pipeline layout!");
  }

  PipelineConfigInfo config{};
  Pipeline::defaultPipelineConfigInfo(config);
  config.useVertexInput = false;
  config.pipelineLayout = lightingPipelineLayout;
  config.renderPass = render_pass;

  lightingPipeline = std::make_unique<Pipeline>(device, vertPath, fragPath, config);
}

}  // namespace baka