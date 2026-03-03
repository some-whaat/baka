#include "renderer.hpp"

// std
#include <array>
#include <cassert>
#include <stdexcept>

namespace baka {

// Renderer::Renderer(Window& window, Device& device) : window{window}, device{device} {
//   recreateSwapChain();
//   createCommandBuffers();
// }

// Renderer::~Renderer() { freeCommandBuffers(); }

// void Renderer::recreateSwapChain() {
//   auto extent = window.getExtent();
//   while (extent.width == 0 || extent.height == 0) {
//     extent = window.getExtent();
//     glfwWaitEvents();
//   }
//   vkDeviceWaitIdle(device.getDevice());

//   if (swap_chain == nullptr) {
//     swap_chain = std::make_unique<SwapChain>(device, extent);
//   } else {
//     std::shared_ptr<SwapChain> oldSwapChain = std::move(swap_chain);
//     swap_chain = std::make_unique<SwapChain>(device, extent);

//     if (!oldSwapChain->compareSwapFormats(*swap_chain.get())) {
//       throw std::runtime_error("Swap chain image(or depth) format has changed!");
//     }
//   }
// }

// void Renderer::createCommandBuffers() {
//   commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

//   VkCommandBufferAllocateInfo allocInfo{};
//   allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//   allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//   allocInfo.commandPool = device.getCommandPool();
//   allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

//   if (vkAllocateCommandBuffers(device.getDevice(), &allocInfo, commandBuffers.data()) !=
//       VK_SUCCESS) {
//     throw std::runtime_error("failed to allocate command buffers!");
//   }
// }

// void Renderer::freeCommandBuffers() {
//   vkFreeCommandBuffers(
//       device.getDevice(),
//       device.getCommandPool(),
//       static_cast<uint32_t>(commandBuffers.size()),
//       commandBuffers.data());
//   commandBuffers.clear();
// }

// VkCommandBuffer Renderer::beginFrame() {
//   assert(!isFrameStarted && "Can't call beginFrame while already in progress");

//   auto result = swap_chain->acquireNextImage(&currentImageIndex);
//   if (result == VK_ERROR_OUT_OF_DATE_KHR) {
//     recreateSwapChain();
//     return nullptr;
//   }

//   if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
//     throw std::runtime_error("failed to acquire swap chain image!");
//   }

//   isFrameStarted = true;

//   auto commandBuffer = getCurrentCommandBuffer();
//   VkCommandBufferBeginInfo beginInfo{};
//   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

//   if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
//     throw std::runtime_error("failed to begin recording command buffer!");
//   }
//   return commandBuffer;
// }

// void Renderer::endFrame() {
//   assert(isFrameStarted && "Can't call endFrame while frame is not in progress");
//   auto commandBuffer = getCurrentCommandBuffer();
//   if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
//     throw std::runtime_error("failed to record command buffer!");
//   }

//   auto result = swap_chain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
//   if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
//       window.wasWindowResized()) {
//     window.resetFramebufferResized();
//     recreateSwapChain();
//   } else if (result != VK_SUCCESS) {
//     throw std::runtime_error("failed to present swap chain image!");
//   }

//   isFrameStarted = false;
//   currentFrameIndex = (currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
// }

// void Renderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
//   assert(isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");
//   assert(
//       commandBuffer == getCurrentCommandBuffer() &&
//       "Can't begin render pass on command buffer from a different frame");

//   VkRenderPassBeginInfo renderPassInfo{};
//   renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
//   renderPassInfo.renderPass = swap_chain->getRenderPass();
//   renderPassInfo.framebuffer = swap_chain->getFrameBuffer(currentImageIndex);

//   renderPassInfo.renderArea.offset = {0, 0};
//   renderPassInfo.renderArea.extent = swap_chain->getSwapChainExtent();

//   std::array<VkClearValue, 2> clearValues{};
//   clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
//   clearValues[1].depthStencil = {1.0f, 0};
//   renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
//   renderPassInfo.pClearValues = clearValues.data();

//   vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

//   VkViewport viewport{};
//   viewport.x = 0.0f;
//   viewport.y = 0.0f;
//   viewport.width = static_cast<float>(swap_chain->getSwapChainExtent().width);
//   viewport.height = static_cast<float>(swap_chain->getSwapChainExtent().height);
//   viewport.minDepth = 0.0f;
//   viewport.maxDepth = 1.0f;
  
//   // Ensure viewport dimensions are valid
//   if (viewport.width <= 0.0f) viewport.width = 1.0f;
//   if (viewport.height <= 0.0f) viewport.height = 1.0f;
  
//   VkRect2D scissor{{0, 0}, swap_chain->getSwapChainExtent()};
//   vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
//   vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
// }

// void Renderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
//   assert(isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
//   assert(
//       commandBuffer == getCurrentCommandBuffer() &&
//       "Can't end render pass on command buffer from a different frame");
//   vkCmdEndRenderPass(commandBuffer);
// }

struct PushConstantData {
  glm::vec2 offset;
  alignas(16) glm::vec3 color;
};


void Renderer::createPipelineLayout() {
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

void Renderer::recreateSwapChain() {
  auto extent = window.getExtent();
  while (extent.width == 0 || extent.height == 0) {
    extent = window.getExtent();
    glfwWaitEvents();
  }
  vkDeviceWaitIdle(device.getDevice());

  if (swap_chain == nullptr) {
    SwapChain = std::make_unique<SwapChain>(Device, extent);
  } else {
    SwapChain = std::make_unique<SwapChain>(Device, extent, std::move(SwapChain));
    if (SwapChain->imageCount() != commandBuffers.size()) {
      freeCommandBuffers();
      createCommandBuffers();
    }
  }

  createPipeline();
}

void Renderer::createPipeline() {
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

void Renderer::recreateSwapChain() {
  auto extent = window.getExtent();
  while (extent.width == 0 || extent.height == 0 ) {
    extent = window.getExtent();
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(_device.getDevice());
  swap_chain = std::make_unique<SwapChain>(_device, extent);
  createPipeline();
}

void Renderer::createCommandBuffers() {
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

void Renderer::recordCommandBuffer(int image_index) {

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
  for (Object& obj : objects) {
    /////////////////////////////////////======================= <----
    /////////////////////////////////////======================= <----
    obj.model->bind(command_buffers[image_index]);
  }
  
  pipeline->bind(command_buffers[image_index]);
  // vkCmdDraw(command_buffers[image_index], 3, 1, 0, 0);

  for (int j = 0; j < 1; j++) {
    PushConstantData push{};
    push.offset = {0.5*sin(anim + j*-0.3), 0.5*cos(anim + j*-0.3), 0.0};
    push.time = anim;

    vkCmdPushConstants(
      command_buffers[image_index],
      pipeline_layout,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      0,
      sizeof(PushConstantData),
      &push
    );
    
    vkCmdDraw(command_buffers[image_index], 3, 1, 0, 0);
    // model->draw(command_buffers[image_index]);
  }

    vkCmdEndRenderPass(command_buffers[image_index]);
    if (vkEndCommandBuffer(command_buffers[image_index]) != VK_SUCCESS) {
      throw std::runtime_error("failed to record command buffer!");
    }
  }
}

void Renderer::drawFrame() {

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

void Renderer::updateFrameRate() {
    double current_time = glfwGetTime();
    frame_count++;
    
    if (current_time - last_time >= 1.0) {
        fps = frame_count / (current_time - last_time);
        frame_count = 0;
        last_time = current_time;
    }

    window.changeTitle("fps: " + std::to_string(fps));
}

}  // namespace 