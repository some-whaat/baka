#include "render_system.hpp"

// std
#include <array>
#include <stdexcept>
#include <cstring>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "material.hpp"

namespace baka {
   
  struct PushConstantData {
    glm::mat4 model{1.f};
    glm::mat4 transform{1.f}; // mvp
    float time;
    // padding to 16-byte boundary
    float _pad0;
    float _pad1;
    float _pad2;
  };

  RenderSystem::RenderSystem(Device &_device, VkRenderPass render_pass) : device{_device}, render_pass(render_pass) {
    createDescriptorSetLayout();
    createDescriptorPool();
    createPipelineLayout();
    // default pipeline not created here; pipelines are created per-material on demand
  }

  RenderSystem::~RenderSystem() {
    vkDestroyPipelineLayout(device.getDevice(), pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(device.getDevice(), sceneDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device.getDevice(), descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(device.getDevice(), descriptorPool, nullptr);

    cleanupSceneDataBuffer();
}


  void RenderSystem::setupObjectDescriptors(std::vector<Object>& objects) {
      for (auto& obj : objects) {
          if (!obj.material) continue;

          obj.material->createDescriptorSet(device.getDevice(), descriptorPool, descriptorSetLayout);
          obj.descriptor_set = obj.material->getDescriptorSet();

          // ensure pipeline exists for this material's shaders
          getOrCreatePipeline(obj.material->getVertPath(), obj.material->getFragPath());
      }

      createSceneDataBuffer();
  }


Pipeline* RenderSystem::getOrCreatePipeline(const std::string& vertFilepath, const std::string& fragFilepath) {
    std::string key = vertFilepath + "|" + fragFilepath;
    auto it = material_pipelines.find(key);
    if (it != material_pipelines.end()) {
        return it->second.get();
    }

    PipelineConfigInfo pipelineConfig{};
    Pipeline::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = render_pass;
    pipelineConfig.pipelineLayout = pipeline_layout;

    auto pipelinePtr = std::make_unique<Pipeline>(device, vertFilepath, fragFilepath, pipelineConfig);
    Pipeline* raw = pipelinePtr.get();
    material_pipelines.emplace(key, std::move(pipelinePtr));
    return raw;
}


  void RenderSystem::renderObjects(VkCommandBuffer command_buffer, std::vector<Object> &objects, const Camera camera, double frame_time) {
    
    glm::mat<4, 4, glm::f32, glm::packed_highp> projection_view = camera.getProjection() * camera.getView();
    
    glm::vec3 light_new_pos = glm::vec3(sin(frame_count), .5, -cos(frame_count))+ objects[0].transform.pos;
    scene_data.point_lights[0].position = light_new_pos;
    updateSceneDataBuffer();


    frame_count += (float)frame_time;

    objects[0].transform.rot.x = sin(frame_count);

    for (auto& obj : objects) {
        if (!obj.material) continue;

        // bind the pipeline for this material
        Pipeline* p = getOrCreatePipeline(obj.material->getVertPath(), obj.material->getFragPath());
        p->bind(command_buffer);

        std::array<VkDescriptorSet, 2> descriptorSets = {
            obj.descriptor_set,
            sceneDescriptorSet
        };
        
        vkCmdBindDescriptorSets(
            command_buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_layout,
            0,
            static_cast<uint32_t>(descriptorSets.size()),
            descriptorSets.data(),
            0,
            nullptr);
        
        PushConstantData push{};
        
        push.model = obj.transform.getMat4();
        push.transform = projection_view * push.model;
        push.time = frame_count;
        
        vkCmdPushConstants(
            command_buffer,
            pipeline_layout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(PushConstantData),
            &push);
        
        obj.model->bind(command_buffer);
        obj.model->draw(command_buffer);
    }
}


  void RenderSystem::createPipelineLayout() {
      VkPushConstantRange pushConstantRange{};
      pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
      pushConstantRange.offset = 0;
      pushConstantRange.size = sizeof(PushConstantData);


      std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = {
          descriptorSetLayout,
          sceneDescriptorSetLayout 
      };

      VkPipelineLayoutCreateInfo pipeline_layoutInfo{};
      pipeline_layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
      pipeline_layoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
      pipeline_layoutInfo.pSetLayouts = descriptorSetLayouts.data();
      pipeline_layoutInfo.pushConstantRangeCount = 1;
      pipeline_layoutInfo.pPushConstantRanges = &pushConstantRange;
      
      if (vkCreatePipelineLayout(device.getDevice(), &pipeline_layoutInfo, nullptr, &pipeline_layout) != VK_SUCCESS) {
          throw std::runtime_error("failed to create pipeline layout!");
      }
  }


//   void RenderSystem::createPipeline(VkRenderPass render_pass) {
//     // kept for compatibility but not used for per-material pipelines
//   }

  void RenderSystem::createDescriptorSetLayout() {
      VkDescriptorSetLayoutBinding samplerLayoutBinding{};
      samplerLayoutBinding.binding = 0;
      samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      samplerLayoutBinding.descriptorCount = 1;
      samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
      samplerLayoutBinding.pImmutableSamplers = nullptr;
      
      VkDescriptorSetLayoutCreateInfo layoutInfo{};
      layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      layoutInfo.bindingCount = 1;
      layoutInfo.pBindings = &samplerLayoutBinding;
      
      if (vkCreateDescriptorSetLayout(device.getDevice(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
          throw std::runtime_error("failed to create descriptor set layout!");
      }

      VkDescriptorSetLayoutBinding sceneDataBinding{};
      sceneDataBinding.binding = 0;
      sceneDataBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      sceneDataBinding.descriptorCount = 1;
      sceneDataBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;// | VK_SHADER_STAGE_VERTEX_BIT;
      sceneDataBinding.pImmutableSamplers = nullptr;

      VkDescriptorSetLayoutCreateInfo sceneLayoutInfo{};
      sceneLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      sceneLayoutInfo.bindingCount = 1;
      sceneLayoutInfo.pBindings = &sceneDataBinding;

      if (vkCreateDescriptorSetLayout(device.getDevice(), &sceneLayoutInfo, nullptr, &sceneDescriptorSetLayout) != VK_SUCCESS) {
          throw std::runtime_error("failed to create scene descriptor set layout!");
      }
  }

  void RenderSystem::createDescriptorPool() {
      std::array<VkDescriptorPoolSize, 2> poolSizes{};
      
      poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      poolSizes[0].descriptorCount = 100;
      
      poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      poolSizes[1].descriptorCount = 1;
      
      VkDescriptorPoolCreateInfo poolInfo{};
      poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
      poolInfo.pPoolSizes = poolSizes.data();
      poolInfo.maxSets = 101;  // 100 object sets + 1 scene set
      
      if (vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
          throw std::runtime_error("failed to create descriptor pool!");
      }
  }
  

  void RenderSystem::createSceneDataBuffer() {
      // ensure we have at least one point light
      scene_data.point_lights.resize(1);
      scene_data.point_lights[0].position = glm::vec3(0.f, 0.f, 0.f);
      scene_data.point_lights[0].color = glm::vec4(1.f, 1.f, 1.f, 1.f);

      VkDeviceSize bufferSize = sizeof(decltype(scene_data.point_lights)::value_type) * scene_data.point_lights.size();

      // create a host visible coherent storage buffer so CPU can update it directly
      device.createBuffer(bufferSize,
                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          scene_data_buffer,
                          scene_data_buffer_memory);

      // map memory for persistent updates
      if (vkMapMemory(device.getDevice(), scene_data_buffer_memory, 0, bufferSize, 0, &scene_mapped_data) != VK_SUCCESS) {
          throw std::runtime_error("failed to map scene data buffer memory!");
      }

      // copy initial data
      std::memcpy(scene_mapped_data, scene_data.point_lights.data(), static_cast<size_t>(bufferSize));

      // allocate and write descriptor set for the scene buffer
      VkDescriptorSetAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      allocInfo.descriptorPool = descriptorPool;
      allocInfo.descriptorSetCount = 1;
      allocInfo.pSetLayouts = &sceneDescriptorSetLayout;

      if (vkAllocateDescriptorSets(device.getDevice(), &allocInfo, &sceneDescriptorSet) != VK_SUCCESS) {
          throw std::runtime_error("failed to allocate scene descriptor set!");
      }

      VkDescriptorBufferInfo bufferInfo{};
      bufferInfo.buffer = scene_data_buffer;
      bufferInfo.offset = 0;
      bufferInfo.range = bufferSize;

      VkWriteDescriptorSet descriptorWrite{};
      descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrite.dstSet = sceneDescriptorSet;
      descriptorWrite.dstBinding = 0;
      descriptorWrite.dstArrayElement = 0;
      descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      descriptorWrite.descriptorCount = 1;
      descriptorWrite.pBufferInfo = &bufferInfo;

      vkUpdateDescriptorSets(device.getDevice(), 1, &descriptorWrite, 0, nullptr);
  }


  void RenderSystem::updateSceneDataBuffer() {
      if (scene_data.point_lights.empty()) return;

      VkDeviceSize bufferSize = sizeof(decltype(scene_data.point_lights)::value_type) * scene_data.point_lights.size();

      if (scene_mapped_data) {
          std::memcpy(scene_mapped_data, scene_data.point_lights.data(), static_cast<size_t>(bufferSize));
      } else {
          void* data;
          if (vkMapMemory(device.getDevice(), scene_data_buffer_memory, 0, bufferSize, 0, &data) != VK_SUCCESS) {
              throw std::runtime_error("failed to map scene data buffer memory for update!");
          }
          std::memcpy(data, scene_data.point_lights.data(), static_cast<size_t>(bufferSize));
          vkUnmapMemory(device.getDevice(), scene_data_buffer_memory);
      }
  }


  void RenderSystem::cleanupSceneDataBuffer() {
      if (scene_mapped_data) {
          vkUnmapMemory(device.getDevice(), scene_data_buffer_memory);
          scene_mapped_data = nullptr;
      }
      if (scene_data_buffer != VK_NULL_HANDLE) {
          vkDestroyBuffer(device.getDevice(), scene_data_buffer, nullptr);
          scene_data_buffer = VK_NULL_HANDLE;
      }
      if (scene_data_buffer_memory != VK_NULL_HANDLE) {
          vkFreeMemory(device.getDevice(), scene_data_buffer_memory, nullptr);
          scene_data_buffer_memory = VK_NULL_HANDLE;
      }
      sceneDescriptorSet = VK_NULL_HANDLE;
  }

 }  // namespace baka