#pragma once

#include "device.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>

namespace baka {

class GBuffer {
  public:
    GBuffer(Device &device);
    ~GBuffer();

    // Create g-buffer resources for the given extent
    void create(VkExtent2D extent);
    void cleanup();

    VkRenderPass getRenderPass() const { return renderPass; }
    VkFramebuffer getFramebuffer() const { return framebuffer; }

    // Begin and end geometry pass recordings
    void beginGeometryPass(VkCommandBuffer cmd, VkExtent2D extent);
    void endGeometryPass(VkCommandBuffer cmd);

    // Color attachments are (position, normal, albedo)
    std::array<VkImageView, 3> getColorImageViews() const;
    VkImageView getDepthImageView() const { return depthImageView; }
    VkSampler getSampler() const { return sampler; }

  private:
    Device &device;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;

    std::array<VkImage, 3> colorImages{};
    std::array<VkDeviceMemory, 3> colorImageMemories{};
    std::array<VkImageView, 3> colorImageViews{};

    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    VkSampler sampler = VK_NULL_HANDLE;
};

} // namespace baka
