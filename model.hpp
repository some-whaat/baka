#pragma once

#include "device.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <memory>
#include "unibuffer.hpp"

namespace baka {


class Model {
    public:

        struct Vertex {
            glm::vec3 position;
            glm::vec2 uv;
            glm::vec3 normal;
            // glm::vec3 tangent;
            glm::vec3 color;
            

            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
            
            bool operator==(const Vertex &other) const {
                return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
            }
        };

        struct Builder {
            std::vector<Vertex> vertices{};
            std::vector<uint32_t> indices{};

            void loadModel(const std::string &filepath);
        };

        Model(Device &device, const Model::Builder &builder);
        ~Model();

        Model(const Model&) = delete;
        Model &operator=(const Model&) = delete;

        static std::unique_ptr<Model> createModelFromFile(Device &device, const std::string &filepath);
        static std::unique_ptr<Model> createQuad(Device &device, float size = 1.0f);
        static std::unique_ptr<Model> createGrid(Device &device, float size, int subdivisions);

        void bind(VkCommandBuffer command_buffer);
        void draw(VkCommandBuffer command_buffer);

    private:
        
        void createVertexBuffers(const std::vector<Vertex> &vertices);
        void createIndexBuffers(const std::vector<uint32_t> &indices);

        Device& device;

        std::unique_ptr<Buffer> vertex_buffer;
        uint32_t vertex_count;


        bool has_index_buffer = false;
        std::unique_ptr<Buffer> index_buffer;
        uint32_t index_count;

};



} // namespace baka