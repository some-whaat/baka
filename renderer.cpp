#include "renderer.hpp"

// std
#include <array>
#include <stdexcept>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace baka {
  	
// struct PushConstantData {
//   glm::vec2 offset;
//   alignas(16) glm::vec3 color;
// };

Renderer::Renderer(Window &_window, Device &_device) : window{_window}, device{_device} {
  recreateSwapChain();
  createCommandBuffers();
}

Renderer::~Renderer() { freeCommandBuffers(); }

void Renderer::freeCommandBuffers() {
  vkFreeCommandBuffers(
      device.getDevice(),
      device.getCommandPool(),
      static_cast<uint32_t>(command_buffers.size()),
      command_buffers.data());
  command_buffers.clear();
}

void Renderer::recreateSwapChain() {
  auto extent = window.getExtent();
  while (extent.width == 0 || extent.height == 0 ) {
    extent = window.getExtent();
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(device.getDevice());
  swap_chain = std::make_unique<SwapChain>(device, extent);
}

void Renderer::createCommandBuffers() {
  command_buffers.resize(swap_chain->imageCount());

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = device.getCommandPool();
  allocInfo.commandBufferCount = static_cast<uint32_t>(command_buffers.size());

  if (vkAllocateCommandBuffers(device.getDevice(), &allocInfo, command_buffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
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

VkCommandBuffer Renderer::beginFrame() {
  assert(!is_frame_started && "Can't call beginFrame while already in progress");

  auto result = swap_chain->acquireNextImage(&curr_img_index);
  
  // if resized
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return nullptr;
  }

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  is_frame_started = true;

  auto command_buffer = getCurrentCommandBuffer();
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(command_buffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  return command_buffer; 
}

void Renderer::endFrame() {

  assert(is_frame_started && "Can't call endFrame while frame is not in progress");

  auto command_buffer = getCurrentCommandBuffer();
  if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }

//   auto result = swap_chain->acquireNextImage(&image_index);
  
//   // if resized
//   if (result == VK_ERROR_OUT_OF_DATE_KHR) {
//     recreateSwapChain();
//     return;
//   }

//   if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
//     throw std::runtime_error("failed to acquire swap chain image!");
//   }

//   recordCommandBuffer(image_index);

  auto result = swap_chain->submitCommandBuffers(&command_buffer, &curr_img_index);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasWindowResized()) {
    window.resetFramebufferResized();
    recreateSwapChain();
  }
  
  else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  is_frame_started = false;

  updateFrameRate();
}

void Renderer::beginSwapChainRenderPass(VkCommandBuffer command_buffer) {
  assert(is_frame_started && "Can't call beginSwapChainRenderPass if frame is not in progress");
  assert(
      command_buffer == getCurrentCommandBuffer() &&
      "Can't begin render pass on command buffer from a different frame");

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = swap_chain->getRenderPass();
  renderPassInfo.framebuffer = swap_chain->getFrameBuffer(curr_img_index);

  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swap_chain->getSwapChainExtent();

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
  clearValues[1].depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(swap_chain->getSwapChainExtent().width);
  viewport.height = static_cast<float>(swap_chain->getSwapChainExtent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  VkRect2D scissor{{0, 0}, swap_chain->getSwapChainExtent()};
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void Renderer::endSwapChainRenderPass(VkCommandBuffer command_buffer) {
  assert(is_frame_started && "Can't call endSwapChainRenderPass if frame is not in progress");
  assert(
      command_buffer == getCurrentCommandBuffer() &&
      "Can't end render pass on command buffer from a different frame");
  vkCmdEndRenderPass(command_buffer);
}


}  // namespace baka