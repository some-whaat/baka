#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <memory>
#include <string>
#include <stdexcept>
#include <vector>

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

    void setNormalMap(std::shared_ptr<Texture> _normal_map) {
      normal_map = std::move(_normal_map);
      has_normal_map = (normal_map != nullptr);
    }

    void setDisplacementMap(std::shared_ptr<Texture> _displacement_map) {
      displacement_map = std::move(_displacement_map);
      has_displacement_map = (displacement_map != nullptr);
    }

    std::shared_ptr<Texture> getTexture() const { return texture; }
    std::shared_ptr<Texture> getNormalMap() const { return normal_map; }
    std::shared_ptr<Texture> getDisplacementMap() const { return displacement_map; }

    VkDescriptorSet getDescriptorSet() const { return descriptor_set; }
    
    bool hasNormalMap() const { return has_normal_map; }
    bool hasDisplacementMap() const { return has_displacement_map; }

    void createDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, 
                           VkDescriptorSetLayout descriptorSetLayout) {
        // ensure descriptor_set is explicitly cleared if we can't create one
        descriptor_set = VK_NULL_HANDLE;
        if (!texture) return;

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;

        if (vkAllocateDescriptorSets(device, &allocInfo, &descriptor_set) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor set for material!");
        }

        std::vector<VkWriteDescriptorSet> descriptorWrites;
        // fixed-size image info storage so pImageInfo pointers stay valid
        VkDescriptorImageInfo imageInfos[3];
        size_t imageInfoCount = 0;
        descriptorWrites.reserve(3);

        // Albedo / base texture
        imageInfos[imageInfoCount] = {};
        imageInfos[imageInfoCount].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[imageInfoCount].imageView = texture->getImageView();
        imageInfos[imageInfoCount].sampler = texture->getSampler();

        VkWriteDescriptorSet descriptorWriteAlbedo{};
        descriptorWriteAlbedo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteAlbedo.dstSet = descriptor_set;
        descriptorWriteAlbedo.dstBinding = 0;
        descriptorWriteAlbedo.dstArrayElement = 0;
        descriptorWriteAlbedo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWriteAlbedo.descriptorCount = 1;
        descriptorWriteAlbedo.pImageInfo = &imageInfos[imageInfoCount];
        descriptorWrites.push_back(descriptorWriteAlbedo);
        ++imageInfoCount;

        // Normal map
        if (has_normal_map && normal_map) {
            imageInfos[imageInfoCount] = {};
            imageInfos[imageInfoCount].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[imageInfoCount].imageView = normal_map->getImageView();
            imageInfos[imageInfoCount].sampler = normal_map->getSampler();

            VkWriteDescriptorSet descriptorWriteNormal{};
            descriptorWriteNormal.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWriteNormal.dstSet = descriptor_set;
            descriptorWriteNormal.dstBinding = 1;
            descriptorWriteNormal.dstArrayElement = 0;
            descriptorWriteNormal.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWriteNormal.descriptorCount = 1;
            descriptorWriteNormal.pImageInfo = &imageInfos[imageInfoCount];
            descriptorWrites.push_back(descriptorWriteNormal);
            ++imageInfoCount;
        }

        // Displacement map
        if (has_displacement_map && displacement_map) {
            imageInfos[imageInfoCount] = {};
            imageInfos[imageInfoCount].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[imageInfoCount].imageView = displacement_map->getImageView();
            imageInfos[imageInfoCount].sampler = displacement_map->getSampler();

            VkWriteDescriptorSet descriptorWriteDisp{};
            descriptorWriteDisp.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWriteDisp.dstSet = descriptor_set;
            descriptorWriteDisp.dstBinding = 2;
            descriptorWriteDisp.dstArrayElement = 0;
            descriptorWriteDisp.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWriteDisp.descriptorCount = 1;
            descriptorWriteDisp.pImageInfo = &imageInfos[imageInfoCount];
            descriptorWrites.push_back(descriptorWriteDisp);
            ++imageInfoCount;
        }

        if (!descriptorWrites.empty()) {
            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

  private:
    std::shared_ptr<Texture> texture;
    std::shared_ptr<Texture> normal_map;
    std::shared_ptr<Texture> displacement_map;
    std::string vert_path;
    std::string frag_path;
    VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
    bool has_normal_map = false;
    bool has_displacement_map = false;
};

} // namespace baka