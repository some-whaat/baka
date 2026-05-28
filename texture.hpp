#pragma once


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>


class Texture {
public:
    Texture(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue);
    Texture(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, const std::string& filepath);
    ~Texture();
    
    // Delete copy constructor and assignment
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    
    // Move constructor and assignment
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;
    
    void loadFromFile(const std::string& filepath);
    void createImageView(VkFormat format);
    void createSampler();
    void init(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue);
    
    VkImage getImage() const { return image; }
    VkImageView getImageView() const { return imageView; }
    VkSampler getSampler() const { return sampler; }
    uint32_t getMipLevels() const { return mipLevels; }
    
    void cleanup();
    
private:
    // Vulkan device references
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkCommandPool commandPool;
    VkQueue graphicsQueue;
    
    // Texture resources
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    uint32_t mipLevels = 1;
    
    // Helper methods
    void createImage(uint32_t width, uint32_t height, VkFormat format, 
                    VkImageTiling tiling, VkImageUsageFlags usage,
                    VkMemoryPropertyFlags properties);
    void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, 
                              VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, uint32_t width, uint32_t height);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};