#include "app.hpp"

// std
#include <array>
#include <stdexcept>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace baka {
  	
struct PushConstantData {
  glm::vec2 offset;
  alignas(16) glm::vec3 color;
};

App::App() {
  loadModels();
  createPipelineLayout();
  recreateSwapChain();
  createCommandBuffers();
}

App::~App() { vkDestroyPipelineLayout(_device.getDevice(), pipeline_layout, nullptr); }

void App::run() {
  while (!window.shouldClose()) {
    glfwPollEvents();
    drawFrame();
  }

  vkDeviceWaitIdle(_device.getDevice());
}

void App::loadModels() {
  std::vector<Model::Vertex> vertices{
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
  };

  model = std::make_unique<Model>(_device, vertices);
}

void App::createPipelineLayout() {
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PushConstantData);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = nullptr;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
  if (vkCreatePipelineLayout(_device.getDevice(), &pipelineLayoutInfo, nullptr, &pipeline_layout) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }
}

// void App::recreateSwapChain() {
//   auto extent = window.getExtent();
//   while (extent.width == 0 || extent.height == 0) {
//     extent = window.getExtent();
//     glfwWaitEvents();
//   }
//   vkDeviceWaitIdle(device.getDevice());

//   if (swap_chain == nullptr) {
//     lveSwapChain = std::make_unique<LveSwapChain>(lveDevice, extent);
//   } else {
//     lveSwapChain = std::make_unique<LveSwapChain>(lveDevice, extent, std::move(lveSwapChain));
//     if (lveSwapChain->imageCount() != commandBuffers.size()) {
//       freeCommandBuffers();
//       createCommandBuffers();
//     }
//   }

//   createPipeline();
// }

void App::createPipeline() {
  auto pipelineConfig =
      Pipeline::defaultPipelineConfigInfo(swap_chain->width(), swap_chain->height());
  pipelineConfig.renderPass = swap_chain->getRenderPass();
  pipelineConfig.pipelineLayout = pipeline_layout;

  pipeline = std::make_unique<Pipeline>(
      "shaders/first_shader.vert.spv",
      "shaders/first_shader.frag.spv",
      _device,
      pipelineConfig);
}

void App::recreateSwapChain() {
  auto extent = window.getExtent();
  while (extent.width == 0 || extent.height == 0 ) {
    extent = window.getExtent();
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(_device.getDevice());
  swap_chain = std::make_unique<SwapChain>(_device, extent);
  createPipeline();
}

void App::createCommandBuffers() {
  command_buffers.resize(swap_chain->imageCount());

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = _device.getCommandPool();
  allocInfo.commandBufferCount = static_cast<uint32_t>(command_buffers.size());

  if (vkAllocateCommandBuffers(_device.getDevice(), &allocInfo, command_buffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

void App::recordCommandBuffer(int image_index) {

  static float anim = 0;

  anim += 0.01;


  for (int i = 0; i < command_buffers.size(); i++) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(command_buffers[image_index], &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = swap_chain->getRenderPass();
  renderPassInfo.framebuffer = swap_chain->getFrameBuffer(image_index);

  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swap_chain->getSwapChainExtent();

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {0.6f, 0.18f, 0.46f, 1.0f};
  clearValues[1].depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(command_buffers[image_index], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  pipeline->bind(command_buffers[image_index]);
  model->bind(command_buffers[image_index]);
  pipeline->bind(command_buffers[image_index]);
  // vkCmdDraw(command_buffers[image_index], 3, 1, 0, 0);

  for (int j = 0; j < 8; j++) {
    PushConstantData push{};
    push.offset = {0.5*sin(anim + j*-0.3), 0.5*cos(anim + j*-0.3)};
    push.color = {0.0f, 0.0f, 1.f - 0.125f * j};

    vkCmdPushConstants(
      command_buffers[image_index],
      pipeline_layout,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      0,
      sizeof(PushConstantData),
      &push
    );

    model->draw(command_buffers[image_index]);
  }

    vkCmdEndRenderPass(command_buffers[image_index]);
    if (vkEndCommandBuffer(command_buffers[image_index]) != VK_SUCCESS) {
      throw std::runtime_error("failed to record command buffer!");
    }
  }
}

void App::drawFrame() {

  uint32_t image_index;
  auto result = swap_chain->acquireNextImage(&image_index);
  
  // if resized
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
  }

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  recordCommandBuffer(image_index);
  result = swap_chain->submitCommandBuffers(&command_buffers[image_index], &image_index);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasWindowResized()) {
    window.resetFramebufferResized();
    recreateSwapChain();
    return;
  }
  
  if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  updateFrameRate();
}

void App::updateFrameRate() {
    double current_time = glfwGetTime();
    frame_count++;
    
    if (current_time - last_time >= 1.0) {
        fps = frame_count / (current_time - last_time);
        frame_count = 0;
        last_time = current_time;
    }

    window.changeTitle("fps: " + std::to_string(fps));
}

}  // namespace baka