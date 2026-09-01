

    /*
     * Encapsulates a vulkan buffer
     *
     * Initially based off VulkanBuffer by Sascha Willems -
     * https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanBuffer.h
     */

#include "unibuffer.hpp"

#include <cassert>
#include <cstring>

namespace baka {

VkDeviceSize Buffer::getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) {
  if (minOffsetAlignment > 0) {
    return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
  }
  return instanceSize;
}

Buffer::Buffer(
    Device &_device,
    VkDeviceSize _instanceSize,
    uint32_t _instanceCount,
    VkBufferUsageFlags _usageFlags,
    VkMemoryPropertyFlags _memoryPropertyFlags,
    VkDeviceSize _minOffsetAlignment)
    : device{_device},
      instanceSize{_instanceSize},
      instanceCount{_instanceCount},
      usageFlags{_usageFlags},
      memoryPropertyFlags{_memoryPropertyFlags} {
  alignmentSize = getAlignment(_instanceSize, _minOffsetAlignment);
  bufferSize = alignmentSize * _instanceCount;
  device.createBuffer(bufferSize, _usageFlags, _memoryPropertyFlags, buffer, memory);
}

Buffer::~Buffer() {
  cleanUp();
}

void Buffer::cleanUp() {
  unmap();
  if (buffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(device.getDevice(), buffer, nullptr);
    buffer = VK_NULL_HANDLE;
  }
  if (memory != VK_NULL_HANDLE) {
    vkFreeMemory(device.getDevice(), memory, nullptr);
    memory = VK_NULL_HANDLE;
  }
}

VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset) {
  assert(buffer && memory && "Called map on buffer before create");
  return vkMapMemory(device.getDevice(), memory, offset, size, 0, &mapped);
}

void Buffer::unmap() {
  if (mapped) {
    vkUnmapMemory(device.getDevice(), memory);
    mapped = nullptr;
  }
}

void Buffer::writeToBuffer(void *data, VkDeviceSize size, VkDeviceSize offset) {
  assert(mapped && "Cannot copy to unmapped buffer");

  if (size == VK_WHOLE_SIZE) {
    memcpy(mapped, data, bufferSize);
  } else {
    char *memOffset = static_cast<char *>(mapped);
    memOffset += offset;
    memcpy(memOffset, data, size);
  }
}

VkResult Buffer::flush(VkDeviceSize size, VkDeviceSize offset) {
  VkMappedMemoryRange mappedRange = {};
  mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  mappedRange.memory = memory;
  mappedRange.offset = offset;
  mappedRange.size = size;
  return vkFlushMappedMemoryRanges(device.getDevice(), 1, &mappedRange);
}

VkResult Buffer::invalidate(VkDeviceSize size, VkDeviceSize offset) {
  VkMappedMemoryRange mappedRange = {};
  mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  mappedRange.memory = memory;
  mappedRange.offset = offset;
  mappedRange.size = size;
  return vkInvalidateMappedMemoryRanges(device.getDevice(), 1, &mappedRange);
}

VkDescriptorBufferInfo Buffer::descriptorInfo(VkDeviceSize size, VkDeviceSize offset) {
  return VkDescriptorBufferInfo{buffer, offset, size};
}

void Buffer::writeToIndex(void *data, int index) {
  writeToBuffer(data, instanceSize, index * alignmentSize);
}

VkResult Buffer::flushIndex(int index) { return flush(alignmentSize, index * alignmentSize); }

VkDescriptorBufferInfo Buffer::descriptorInfoForIndex(int index) {
  return descriptorInfo(alignmentSize, index * alignmentSize);
}

VkResult Buffer::invalidateIndex(int index) {
  return invalidate(alignmentSize, index * alignmentSize);
}


}  // namespace baka
     