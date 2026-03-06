#pragma once

#include "device.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace baka {


class Model {
    public:

        struct Vertex {
            glm::vec3 position;
            glm::vec3 color;

            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
        };

        struct Builder {
            std::vector<Vertex> vertices{};
            std::vector<uint32_t> indices{};
        };

        Model(Device &device, const Model::Builder &builder);
        ~Model();

        Model(const Model&) = delete;
        Model &operator=(const Model&) = delete;

        void bind(VkCommandBuffer command_buffer);
        void draw(VkCommandBuffer command_buffer);

    private:
        
        void createVertexBuffers(const std::vector<Vertex> &vertices);
        void createIndexBuffers(const std::vector<uint32_t> &indices);

        Device& device;

        VkBuffer vertex_buffer;
        VkDeviceMemory vertex_buffer_memory;
        uint32_t vertex_count;


        bool has_index_buffer = false;
        VkBuffer index_buffer;
        VkDeviceMemory index_buffer_memory;
        uint32_t index_count;

};



} // namespace baka