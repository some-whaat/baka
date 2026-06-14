#include "model.hpp"

#include "utils.hpp"

// libs
#define TINYOBJLOADER_IMPLEMENTATION
#include "libs/tiny_obj_loader.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>


// std
#include <cassert>
#include <cstring>
#include <unordered_map>


namespace std {
template <>
struct hash<baka::Model::Vertex> {
  size_t operator()(baka::Model::Vertex const &vertex) const {
    size_t seed = 0;
    baka::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
    return seed;
  }
};
}  // namespace std

namespace baka {

Model::Model(Device &_device, const Model::Builder &builder) : device(_device) {
    createVertexBuffers(builder.vertices);
    createIndexBuffers(builder.indices);
}

Model::~Model() {
    vkDestroyBuffer(device.getDevice(), vertex_buffer, nullptr);
    vkFreeMemory(device.getDevice(), vertex_buffer_memory, nullptr);

    if (has_index_buffer) {
      vkDestroyBuffer(device.getDevice(), index_buffer, nullptr);
      vkFreeMemory(device.getDevice(), index_buffer_memory, nullptr);
    }
}

std::unique_ptr<Model> Model::createModelFromFile(Device &device, const std::string &filepath) {
  Builder builder{};
  builder.loadModel(filepath);
  return std::make_unique<Model>(device, builder);
}

std::unique_ptr<Model> Model::createQuad(Device &device, float size) {
  Builder builder{};
  float half = size / 2.0f;

  // positions on XY plane (Z = 0)
  Vertex v0{}; v0.position = {-half, -half, 0.0f}; v0.uv = {0.0f, 0.0f}; v0.normal = {0.0f, 0.0f, 1.0f}; v0.color = {1.0f, 1.0f, 1.0f};
  Vertex v1{}; v1.position = {half, -half, 0.0f};  v1.uv = {1.0f, 0.0f}; v1.normal = {0.0f, 0.0f, 1.0f}; v1.color = {1.0f, 1.0f, 1.0f};
  Vertex v2{}; v2.position = {half, half, 0.0f};   v2.uv = {1.0f, 1.0f}; v2.normal = {0.0f, 0.0f, 1.0f}; v2.color = {1.0f, 1.0f, 1.0f};
  Vertex v3{}; v3.position = {-half, half, 0.0f};  v3.uv = {0.0f, 1.0f}; v3.normal = {0.0f, 0.0f, 1.0f}; v3.color = {1.0f, 1.0f, 1.0f};

  builder.vertices = {v0, v1, v2, v3};
  // two triangles: (0,1,2) and (2,3,0)
  builder.indices = {0, 1, 2, 2, 3, 0};

  return std::make_unique<Model>(device, builder);
}

std::unique_ptr<Model> Model::createGrid(Device &device, float size, int subdivisions) {
  Builder builder{};
  float half = size / 2.0f;
  float step = size / subdivisions;

  for (float z = -half; z <= half; z += step) {
    for (float x = -half; x <= half; x += step) {
      Vertex v{}; v.position = {x, 0.0f, z};
      v.uv = {(x+half)/size, (z+half)/size};
      v.normal = {0.0f, 1.0f, 0.0f};
      v.color = {1.0f, 1.0f, 1.0f};

      builder.vertices.push_back(v);
    }
  }

  // indices
  for (int z = 0; z < subdivisions; z++) {
      for (int x = 0; x < subdivisions; x++) {

        int top_left = z * subdivisions + x;
        int top_right = z * subdivisions + (x + 1);
        int bottom_left = (z + 1) * subdivisions + x;
        int bottom_right = (z + 1) * subdivisions + (x + 1);

        builder.indices.push_back(top_left);
        builder.indices.push_back(bottom_left);
        builder.indices.push_back(bottom_right);

        builder.indices.push_back(top_left);
        builder.indices.push_back(bottom_right);
        builder.indices.push_back(top_right);
      }
  }

  return std::make_unique<Model>(device, builder);
}

void Model::createVertexBuffers(const std::vector<Vertex> &vertices) {
  vertex_count = static_cast<uint32_t>(vertices.size());
  assert(vertex_count >= 3 && "Vertex count must be at least 3");

  VkDeviceSize buffer_size = sizeof(vertices[0]) * vertex_count;
  device.createBuffer(
      buffer_size,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      vertex_buffer,
      vertex_buffer_memory);

  void *data;
  vkMapMemory(device.getDevice(), vertex_buffer_memory, 0, buffer_size, 0, &data);
  memcpy(data, vertices.data(), static_cast<size_t>(buffer_size)); // copyies data from host (cpu) to device (gpu)
  vkUnmapMemory(device.getDevice(), vertex_buffer_memory);
}

void Model::createIndexBuffers(const std::vector<uint32_t> &indices) {
  index_count = static_cast<uint32_t>(indices.size());

  has_index_buffer = index_count > 0;

  if (!has_index_buffer) {
    return;
  }

  VkDeviceSize buffer_size = sizeof(indices[0]) * index_count;
  device.createBuffer(
      buffer_size,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      index_buffer,
      index_buffer_memory);

  void *data;
  vkMapMemory(device.getDevice(), index_buffer_memory, 0, buffer_size, 0, &data);
  memcpy(data, indices.data(), static_cast<size_t>(buffer_size)); // copyies data from host (cpu) to device (gpu)
  vkUnmapMemory(device.getDevice(), index_buffer_memory);
}


void Model::draw(VkCommandBuffer command_buffer) {
  if (has_index_buffer) {

    vkCmdDrawIndexed(command_buffer, index_count, 1, 0, 0, 0);
  } 
  else {

    vkCmdDraw(command_buffer, vertex_count, 1, 0, 0);
  }
}

void Model::bind(VkCommandBuffer command_buffer) {
  VkBuffer buffers[] = {vertex_buffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(command_buffer, 0, 1, buffers, offsets);

  if (has_index_buffer) {
    vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);
  }
}

std::vector<VkVertexInputBindingDescription> Model::Vertex::getBindingDescriptions() {
  std::vector<VkVertexInputBindingDescription> binding_descriptions(1);

  binding_descriptions[0].binding = 0;
  binding_descriptions[0].stride = sizeof(Vertex);
  binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  return binding_descriptions;
}

std::vector<VkVertexInputAttributeDescription> Model::Vertex::getAttributeDescriptions() {
  std::vector<VkVertexInputAttributeDescription> attribute_descriptions(4);

  attribute_descriptions[0].binding = 0;
  attribute_descriptions[0].location = 0;
  attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attribute_descriptions[0].offset = offsetof(Vertex, position);

  attribute_descriptions[1].binding = 0;
  attribute_descriptions[1].location = 1;
  attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
  attribute_descriptions[1].offset = offsetof(Vertex, uv);

  attribute_descriptions[2].binding = 0;
  attribute_descriptions[2].location = 2;
  attribute_descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
  attribute_descriptions[2].offset = offsetof(Vertex, normal);

  attribute_descriptions[3].binding = 0;
  attribute_descriptions[3].location = 3;
  attribute_descriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
  attribute_descriptions[3].offset = offsetof(Vertex, color);

  return attribute_descriptions;
}

void Model::Builder::loadModel(const std::string &filepath) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  
  std::string warn, err;
  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
    throw std::runtime_error(warn + err);
  }

  vertices.clear();
  indices.clear();

  std::unordered_map<Vertex, uint32_t> uniqueVertices{};
  for (const auto &shape : shapes) {
    for (const auto &index : shape.mesh.indices) {
      Vertex vertex{};

      if (index.vertex_index >= 0) {
        vertex.position = {
            attrib.vertices[3 * index.vertex_index + 0],
            attrib.vertices[3 * index.vertex_index + 1],
            attrib.vertices[3 * index.vertex_index + 2],
        };

        auto color_index = 3 * index.vertex_index + 2;
        if (color_index < attrib.colors.size()) { // if there is color
          vertex.color = {
              attrib.colors[color_index - 2],
              attrib.colors[color_index - 1],
              attrib.colors[color_index - 0],
          };
        } else {
          vertex.color = {1.f, 1.f, 1.f};  // set default color
        }
      }

      if (index.normal_index >= 0) {
        vertex.normal = {
            attrib.normals[3 * index.normal_index + 0],
            attrib.normals[3 * index.normal_index + 1],
            attrib.normals[3 * index.normal_index + 2],
        };
      }

      if (index.texcoord_index >= 0) {
        vertex.uv = {
            attrib.texcoords[2 * index.texcoord_index + 0],
            attrib.texcoords[2 * index.texcoord_index + 1],
        };
      }

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
        vertices.push_back(vertex);
      }
      indices.push_back(uniqueVertices[vertex]);
    }
  }
}


} // namespace baka