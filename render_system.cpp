#include "render_system.hpp"
#include "device.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <stdexcept>
#include <memory>

namespace baka {

struct PushConstantData {
  glm::mat4 transform{1.f};
  alignas(16) glm::vec3 color{};
};

RenderSystem::RenderSystem(Device &device, VkRenderPass renderPass) : device{device} {
  createPipelineLayout();
  createPipeline(renderPass);
}

RenderSystem::~RenderSystem() {
  vkDestroyPipelineLayout(device.getDevice(), pipeline_layout, nullptr);
}

void RenderSystem::createPipelineLayout() {
  
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PushConstantData);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = nullptr;
  pipelineLayoutInfo.pushConstantRangeCount = 1;

  if (vkCreatePipelineLayout(device.getDevice(), &pipelineLayoutInfo, nullptr, &pipeline_layout) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }
}

void RenderSystem::createPipeline(VkRenderPass renderPass) {
  assert(pipeline_layout != nullptr && "Cannot create pipeline before pipeline layout");

  PipelineConfigInfo pipeline_config{};
  Pipeline::defaultPipelineConfigInfo(pipeline_config);
  pipeline_config.renderPass = renderPass;
  pipeline_config.pipelineLayout = pipeline_layout;
  pipeline = std::make_unique<Pipeline>(
      "shaders/_shader.vert.spv",
      "shaders/_shader.frag.spv",
      device,
      pipeline_config);
}

void RenderSystem::renderObjects( VkCommandBuffer commandBuffer, std::vector<Object>& objects) {
  pipeline->bind(commandBuffer);

  for (auto& obj : objects) {
    obj.transform.rot.y = glm::mod(obj.transform.rot.y + 0.01f, glm::two_pi<float>());
    obj.transform.rot.x = glm::mod(obj.transform.rot.x + 0.005f, glm::two_pi<float>());

    PushConstantData push{};
    // push.color = obj.color;
    push.transform = obj.transform.getMat4();

    vkCmdPushConstants(
        commandBuffer,
        pipeline_layout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PushConstantData),
        &push);
    obj.model->bind(commandBuffer);
    obj.model->draw(commandBuffer);
  }
}

}  // namespace 