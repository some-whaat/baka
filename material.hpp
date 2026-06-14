#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <memory>
#include <string>
#include <stdexcept>

#include "texture.hpp"


namespace baka {

class Material {
  public:
    Material() = default;
    Material(std::shared_ptr<Texture> _texture, const std::string& vert, const std::string& frag)
        : texture(std::move(_texture)), vert_path(vert), frag_path(frag) {}

    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;

    const std::string& getVertPath() const { return vert_path; }
    const std::string& getFragPath() const { return frag_path; }

    std::shared_ptr<Texture> getTexture() const { return texture; }

    VkDescriptorSet getDescriptorSet() const { return descriptor_set; }

    // Allocates and writes the descriptor set for this material (image sampler)
    void createDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout) {
        if (!texture) return;

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;

        if (vkAllocateDescriptorSets(device, &allocInfo, &descriptor_set) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor set for material!");
        }

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = texture->getImageView();
        imageInfo.sampler = texture->getSampler();

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptor_set;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }

  private:
    std::shared_ptr<Texture> texture;
    std::string vert_path;
    std::string frag_path;
    VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
};

} // namespace baka
