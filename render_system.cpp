#include "render_system.hpp"

// std
#include <array>
#include <stdexcept>
#include <cstring>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

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

  RenderSystem::RenderSystem(Device &_device, VkRenderPass render_pass) : device{_device} {
    createDescriptorSetLayout();
    createDescriptorPool();
    createPipelineLayout();
    createPipeline(render_pass);
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
          if (!obj.texture) continue;
          
          VkDescriptorSetAllocateInfo allocInfo{};
          allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
          allocInfo.descriptorPool = descriptorPool;
          allocInfo.descriptorSetCount = 1;
          allocInfo.pSetLayouts = &descriptorSetLayout;
          
          if (vkAllocateDescriptorSets(device.getDevice(), &allocInfo, &obj.descriptor_set) != VK_SUCCESS) {
              throw std::runtime_error("failed to allocate descriptor set!");
          }
          
          VkDescriptorImageInfo imageInfo{};
          imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
          imageInfo.imageView = obj.texture->getImageView();
          imageInfo.sampler = obj.texture->getSampler();
          
          VkWriteDescriptorSet descriptorWrite{};
          descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          descriptorWrite.dstSet = obj.descriptor_set;
          descriptorWrite.dstBinding = 0;
          descriptorWrite.dstArrayElement = 0;
          descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
          descriptorWrite.descriptorCount = 1;
          descriptorWrite.pImageInfo = &imageInfo;
          
          vkUpdateDescriptorSets(device.getDevice(), 1, &descriptorWrite, 0, nullptr);
      }



    //   PointLightData point_light;
    //   point_light.color = {1., 1., 0., 1.};
    //   point_light.position = {0., 0.2, -0.5};

    //   scene_data.point_lights = {point_light};

    //   VkDeviceSize scene_data_buffer_size = sizeof(PointLightData) * scene_data.point_lights.size() + sizeof(uint32_t);

    //   device.createBuffer(
    //       scene_data_buffer_size,
    //       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    //       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    //       scene_data_buffer,
    //       scene_data_buffer_memory);

          
      createSceneDataBuffer();
  }

void RenderSystem::createSceneDataBuffer() {
    PointLightData point_light;
    point_light.color = {0., 1., 1., 0.9};
    point_light.position = {0., 0.0, 3.0};

    scene_data.point_lights = {point_light};

    // Calculate proper buffer size with alignment
    VkDeviceSize scene_data_buffer_size = sizeof(uint32_t) + sizeof(PointLightData) * scene_data.point_lights.size();
    
    // Ensure minimum alignment (16 bytes is typical for SSBO)
    scene_data_buffer_size = (scene_data_buffer_size + 15) & ~15;

    device.createBuffer(
        scene_data_buffer_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        scene_data_buffer,
        scene_data_buffer_memory);

    // Map and copy data
    // void* scene_mapped_data;
    vkMapMemory(device.getDevice(), scene_data_buffer_memory, 0, scene_data_buffer_size, 0, &scene_mapped_data);
    
    // First copy the count
    uint32_t numLights = static_cast<uint32_t>(scene_data.point_lights.size());
    memcpy(scene_mapped_data, &numLights, sizeof(uint32_t));
    
    // Then copy the light data immediately after count (offset = sizeof(uint32_t))
    memcpy(static_cast<char*>(scene_mapped_data) + sizeof(uint32_t), scene_data.point_lights.data(), 
          sizeof(PointLightData) * scene_data.point_lights.size());
    
    // vkUnmapMemory(device.getDevice(), scene_data_buffer_memory);

    // scene descriptor
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = scene_data_buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = scene_data_buffer_size;  // Use actual buffer size
    
    VkDescriptorSetAllocateInfo sceneAllocInfo{};
    sceneAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    sceneAllocInfo.descriptorPool = descriptorPool;
    sceneAllocInfo.descriptorSetCount = 1;
    sceneAllocInfo.pSetLayouts = &sceneDescriptorSetLayout;
    
    if (vkAllocateDescriptorSets(device.getDevice(), &sceneAllocInfo, &sceneDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate scene descriptor set!");
    }
    
    VkWriteDescriptorSet sceneWrite{};
    sceneWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    sceneWrite.dstSet = sceneDescriptorSet;
    sceneWrite.dstBinding = 0;
    sceneWrite.dstArrayElement = 0;
    sceneWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    sceneWrite.descriptorCount = 1;
    sceneWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(device.getDevice(), 1, &sceneWrite, 0, nullptr);
}


void RenderSystem::updateSceneDataBuffer() {
    if (!scene_mapped_data) return;
    
    memcpy(static_cast<char*>(scene_mapped_data) + sizeof(uint32_t), 
           scene_data.point_lights.data(), 
           sizeof(PointLightData) * scene_data.point_lights.size());
    
    // Since we're using HOST_COHERENT memory, no need to flush
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
}


 void RenderSystem::renderObjects(VkCommandBuffer command_buffer, std::vector<Object> &objects, const Camera camera, double frame_time) {
    
    pipeline->bind(command_buffer);
    glm::mat<4, 4, glm::f32, glm::packed_highp> projection_view = camera.getProjection() * camera.getView();
    
    glm::vec3 light_new_pos = glm::vec3(sin(frame_count), .5, -cos(frame_count))+ objects[0].transform.pos;
    // std::cout << "x: " << light_new_pos.x << std::endl;
    // std::cout << "y: " << light_new_pos.y << std::endl;
    // std::cout << "z: " <<  light_new_pos.z << std::endl;
    scene_data.point_lights[0].position = light_new_pos;
    updateSceneDataBuffer();


    frame_count += (float)frame_time;

    // objects[0].transform.rot.y = -0.8;
    objects[0].transform.rot.x = sin(frame_count);//90;
    // objects[0].transform.rot.z = 10;

    for (auto& obj : objects) {
        // if (!obj.texture || obj.descriptor_set == VK_NULL_HANDLE) continue;
        
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
        // model is the object's model->world transform (used for lighting)
        push.model = obj.transform.getMat4();
        // transform is full MVP (used for gl_Position)
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


  void RenderSystem::createPipeline(VkRenderPass render_pass) {

    assert(pipeline_layout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    Pipeline::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = render_pass;
    pipelineConfig.pipelineLayout = pipeline_layout;
    pipeline = std::make_unique<Pipeline>(
        device,
        "shaders/first_shader.vert.spv",
        "shaders/first_shader.frag.spv",
        pipelineConfig);

    assert(pipeline_layout != nullptr && "Cannot create pipeline before pipeline layout");
  }

  void RenderSystem::createDescriptorSetLayout() {
      // per-object texture sampler
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

      // scene-wide uniform buffer
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
  

}  // namespace baka