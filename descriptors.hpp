#pragma once

#include "device.hpp"

#include <vector>

namespace baka {
        
    class DescriptorSetLayout {
    public:
    DescriptorSetLayout(Device& device, std::vector<VkDescriptorSetLayoutBinding> bindings);
    ~DescriptorSetLayout();

    DescriptorSetLayout(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

    VkDescriptorSetLayout get() const { return layout; }
    operator VkDescriptorSetLayout() const { return layout; }

    private:
    Device& device;
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    };

    class DescriptorSet {
    public:
    DescriptorSet(Device& device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout layout);
    ~DescriptorSet() = default;

    DescriptorSet(const DescriptorSet&) = delete;
    DescriptorSet& operator=(const DescriptorSet&) = delete;

    VkDescriptorSet get() const { return descriptorSet; }
    operator VkDescriptorSet() const { return descriptorSet; }

    void updateBuffer(
        uint32_t binding,
        VkBuffer buffer,
        VkDeviceSize range,
        VkDeviceSize offset = 0,
        VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    void updateImage(
        uint32_t binding,
        VkImageView imageView,
        VkSampler sampler,
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    private:
    Device& device;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    };

}  // namespace baka
 