
#include "device.hpp"
#include "descriptors.hpp"

namespace baka {

DescriptorSetLayout::DescriptorSetLayout(Device& _device, std::vector<VkDescriptorSetLayoutBinding> bindings)
    : device{_device} {
  if (bindings.empty()) {
    return;
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  if (vkCreateDescriptorSetLayout(device.getDevice(), &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

DescriptorSetLayout::~DescriptorSetLayout() {
  if (layout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device.getDevice(), layout, nullptr);
    layout = VK_NULL_HANDLE;
  }
}

DescriptorSet::DescriptorSet(Device& _device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout layout)
    : device{_device} {
  if (layout == VK_NULL_HANDLE) {
    return;
  }

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &layout;

  if (vkAllocateDescriptorSets(device.getDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor set!");
  }
}

void DescriptorSet::updateBuffer(
    uint32_t binding,
    VkBuffer buffer,
    VkDeviceSize range,
    VkDeviceSize offset,
    VkDescriptorType type) {
  if (descriptorSet == VK_NULL_HANDLE) {
    return;
  }

  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = buffer;
  bufferInfo.offset = offset;
  bufferInfo.range = range;

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = descriptorSet;
  descriptorWrite.dstBinding = binding;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = type;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = &bufferInfo;

  vkUpdateDescriptorSets(device.getDevice(), 1, &descriptorWrite, 0, nullptr);
}

void DescriptorSet::updateImage(
    uint32_t binding,
    VkImageView imageView,
    VkSampler sampler,
    VkImageLayout imageLayout) {
  if (descriptorSet == VK_NULL_HANDLE) {
    return;
  }

  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = imageLayout;
  imageInfo.imageView = imageView;
  imageInfo.sampler = sampler;

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = descriptorSet;
  descriptorWrite.dstBinding = binding;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pImageInfo = &imageInfo;

  vkUpdateDescriptorSets(device.getDevice(), 1, &descriptorWrite, 0, nullptr);
}
}