#include "render_system.hpp"

// std
#include <array>
#include <stdexcept>
#include <cstring>
#include <random>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "material.hpp"

namespace baka {
   
  struct PushConstantData {
    glm::mat4 projection_view{1.f};
    uint32_t objectIndex;
    float time;
    float _pad0; // padding to 16 bytes
  };

  struct PushConstantDataJustMatrix {
    glm::mat4 projection_view{1.f};
    // uint32_t objectIndex;
    // float time;
    // float _pad0; // padding to 16 bytes
  };

  // create or resize object storage buffer for per-object transforms
  void RenderSystem::createObjectBuffer(size_t objectCount) {
      // cleanup existing buffer if present
      if (object_buffer != VK_NULL_HANDLE) {
          if (object_mapped_data) {
              vkUnmapMemory(device.getDevice(), object_buffer_memory);
              object_mapped_data = nullptr;
          }
          vkDestroyBuffer(device.getDevice(), object_buffer, nullptr);
          vkFreeMemory(device.getDevice(), object_buffer_memory, nullptr);
          object_buffer = VK_NULL_HANDLE;
          object_buffer_memory = VK_NULL_HANDLE;
          objectDescriptorSet = VK_NULL_HANDLE;
      }

      if (objectCount == 0) return;
      VkDeviceSize bufferSize = objectCount * sizeof(glm::mat4);
      device.createBuffer(bufferSize,
                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          object_buffer,
                          object_buffer_memory);

      if (vkMapMemory(device.getDevice(), object_buffer_memory, 0, bufferSize, 0, &object_mapped_data) != VK_SUCCESS) {
          throw std::runtime_error("failed to map object buffer memory!");
      }

      // write descriptor set
      VkDescriptorSetAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      allocInfo.descriptorPool = descriptorPool;
      allocInfo.descriptorSetCount = 1;
      allocInfo.pSetLayouts = &objectDescriptorSetLayout;
      if (vkAllocateDescriptorSets(device.getDevice(), &allocInfo, &objectDescriptorSet) != VK_SUCCESS) {
          throw std::runtime_error("failed to allocate object descriptor set!");
      }

      VkDescriptorBufferInfo bufferInfo{};
      bufferInfo.buffer = object_buffer;
      bufferInfo.offset = 0;
      bufferInfo.range = bufferSize;

      VkWriteDescriptorSet descriptorWrite{};
      descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrite.dstSet = objectDescriptorSet;
      descriptorWrite.dstBinding = 0;
      descriptorWrite.dstArrayElement = 0;
      descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      descriptorWrite.descriptorCount = 1;
      descriptorWrite.pBufferInfo = &bufferInfo;

      vkUpdateDescriptorSets(device.getDevice(), 1, &descriptorWrite, 0, nullptr);
  }

  RenderSystem::RenderSystem(Device &_device, VkRenderPass render_pass, VkExtent2D extent) : device{_device}, render_pass(render_pass), swapchainExtent(extent) {
     createDescriptorSetLayout();
     createDescriptorPool();
     // create object descriptor set layout so pipeline layout can include it
     VkDescriptorSetLayoutBinding objectBinding{};
     objectBinding.binding = 0;
     objectBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
     objectBinding.descriptorCount = 1;
     objectBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
     objectBinding.pImmutableSamplers = nullptr;

     VkDescriptorSetLayoutCreateInfo objectLayoutInfo{};
     objectLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
     objectLayoutInfo.bindingCount = 1;
     objectLayoutInfo.pBindings = &objectBinding;
     if (vkCreateDescriptorSetLayout(device.getDevice(), &objectLayoutInfo, nullptr, &objectDescriptorSetLayout) != VK_SUCCESS) {
         throw std::runtime_error("failed to create object descriptor set layout");
     }

     createPipelineLayout();
     // create GBuffer
     gBuffer = std::make_unique<GBuffer>(device);
     gBuffer->create(extent);
    
    // create object buffer with small default count (will be reallocated if needed in setupObjectDescriptors)
    createObjectBuffer(16);

     // create descriptor set layout for sampling GBuffer
     VkDescriptorSetLayoutBinding gBinding[3];
     for (uint32_t i = 0; i < 3; ++i) {
        gBinding[i].binding = i;
        gBinding[i].descriptorCount = 1;
        gBinding[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        gBinding[i].pImmutableSamplers = nullptr;
        gBinding[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
     }
     VkDescriptorSetLayoutCreateInfo gLayoutInfo{};
     gLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
     gLayoutInfo.bindingCount = 3;
     gLayoutInfo.pBindings = gBinding;
     if (vkCreateDescriptorSetLayout(device.getDevice(), &gLayoutInfo, nullptr, &gbufferDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create gbuffer descriptor set layout");
     }
     try {
         createGBufferDescriptorSet();
     } catch (const std::exception &e) {
         // if descriptor set creation failed, destroy the gbuffer descriptor set layout and continue
         if (gbufferDescriptorSetLayout != VK_NULL_HANDLE) {
             vkDestroyDescriptorSetLayout(device.getDevice(), gbufferDescriptorSetLayout, nullptr);
             gbufferDescriptorSetLayout = VK_NULL_HANDLE;
         }
     }

     // create lighting pipeline (shaders may be missing; do not let this abort construction)
     try {
         createLightingPipeline("shaders/lighting.vert.spv", "shaders/lighting.frag.spv");
     } catch (const std::exception &e) {
         // if lighting pipeline creation failed, clean up any partially created layout and skip lighting
         if (lightingPipelineLayout != VK_NULL_HANDLE) {
             vkDestroyPipelineLayout(device.getDevice(), lightingPipelineLayout, nullptr);
             lightingPipelineLayout = VK_NULL_HANDLE;
         }
         lightingPipeline.reset();
     }
     // default pipeline not created here; pipelines are created per-material on demand
  }

  RenderSystem::~RenderSystem() {
    // destroy lighting pipeline first
    lightingPipeline.reset();

    if (lightingPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device.getDevice(), lightingPipelineLayout, nullptr);
        lightingPipelineLayout = VK_NULL_HANDLE;
    }

    if (pipeline_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device.getDevice(), pipeline_layout, nullptr);
        pipeline_layout = VK_NULL_HANDLE;
    }

    if (gbufferDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device.getDevice(), gbufferDescriptorSetLayout, nullptr);
        gbufferDescriptorSetLayout = VK_NULL_HANDLE;
    }

    if (sceneDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device.getDevice(), sceneDescriptorSetLayout, nullptr);
        sceneDescriptorSetLayout = VK_NULL_HANDLE;
    }

    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device.getDevice(), descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
    }

    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device.getDevice(), descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }

    cleanupSceneDataBuffer();

    // cleanup object buffer
    if (object_mapped_data) {
        vkUnmapMemory(device.getDevice(), object_buffer_memory);
        object_mapped_data = nullptr;
    }
    if (object_buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device.getDevice(), object_buffer, nullptr);
        object_buffer = VK_NULL_HANDLE;
    }
    if (object_buffer_memory != VK_NULL_HANDLE) {
        vkFreeMemory(device.getDevice(), object_buffer_memory, nullptr);
        object_buffer_memory = VK_NULL_HANDLE;
    }
}


  void RenderSystem::setupObjectDescriptors(std::vector<Object>& objects) {
      for (auto& obj : objects) {
          if (!obj.material) continue;

          obj.material->createDescriptorSet(device.getDevice(), descriptorPool, descriptorSetLayout);
          obj.descriptor_set = obj.material->getDescriptorSet();

          getOrCreatePipeline(obj.material->getVertPath(), obj.material->getFragPath());
      }

      // allocate and fill per-object buffer with model matrices
      createSceneDataBuffer();
      createObjectBuffer(objects.size());
      if (object_mapped_data) {
          for (size_t i = 0; i < objects.size(); ++i) {
              glm::mat4 m = objects[i].transform.getMat4();
              std::memcpy(reinterpret_cast<char*>(object_mapped_data) + i * sizeof(glm::mat4), &m, sizeof(glm::mat4));
          }
      }
  }


Pipeline* RenderSystem::getOrCreatePipeline(const std::string& vertFilepath, const std::string& fragFilepath) {
    return getOrCreatePipeline(vertFilepath, fragFilepath, render_pass);
}


Pipeline* RenderSystem::getOrCreatePipeline(const std::string& vertFilepath, const std::string& fragFilepath, VkRenderPass targetRenderPass) {
    std::string key = vertFilepath + "|" + fragFilepath + "|" + std::to_string((uint64_t)targetRenderPass);
    auto it = material_pipelines.find(key);
    if (it != material_pipelines.end()) {
        return it->second.get();
    }

    PipelineConfigInfo pipelineConfig{};
    Pipeline::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = targetRenderPass;
    pipelineConfig.pipelineLayout = pipeline_layout;

    // If this pipeline targets the GBuffer render pass it must have one color blend attachment state per
    // color attachment in that render pass (GBuffer uses 3 color attachments). Otherwise Vulkan reports
    // a mismatch between pipeline and render pass and rendering can fail (black screen or validation errors).
    if (gBuffer && targetRenderPass == gBuffer->getRenderPass()) {
        pipelineConfig.colorBlendAttachments.clear();
        // create three attachments copying the default attachment state
        for (int i = 0; i < 3; ++i) pipelineConfig.colorBlendAttachments.push_back(pipelineConfig.colorBlendAttachment);
    }

    auto pipelinePtr = std::make_unique<Pipeline>(device, vertFilepath, fragFilepath, pipelineConfig);
    Pipeline* raw = pipelinePtr.get();
    material_pipelines.emplace(key, std::move(pipelinePtr));
    return raw;
}


void RenderSystem::renderToGBuffer(VkCommandBuffer command_buffer, std::vector<Object> &objects, const Camera camera, double frame_time) {
    if (!gBuffer) return;
    VkExtent2D extent = swapchainExtent;
    gBuffer->beginGeometryPass(command_buffer, extent);

    
    updateSceneDataBuffer();
    

    glm::mat4 projection_view = camera.getProjection() * camera.getView();

    for (size_t i = 0; i < objects.size(); ++i) {
        auto &obj = objects[i];
        if (!obj.material) continue;

        Pipeline* p = getOrCreatePipeline(obj.material->getVertPath(), obj.material->getFragPath(), gBuffer->getRenderPass());
        p->bind(command_buffer);

        std::array<VkDescriptorSet, 3> descriptorSets = { obj.descriptor_set, sceneDescriptorSet, objectDescriptorSet };
        vkCmdBindDescriptorSets(
            command_buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_layout,
            0,
            static_cast<uint32_t>(descriptorSets.size()),
            descriptorSets.data(),
            0,
            nullptr);
        
        frame_count += frame_time;
        PushConstantData push{};
        push.projection_view = projection_view;
        push.objectIndex = static_cast<uint32_t>(i);
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

    gBuffer->endGeometryPass(command_buffer);
}


void RenderSystem::renderGeometry(VkCommandBuffer command_buffer, std::vector<Object> &objects, const Camera camera, double frame_time) {
    // geometry pass must be recorded outside the swapchain render pass
    renderToGBuffer(command_buffer, objects, camera, frame_time);
}


void RenderSystem::renderLighting(VkCommandBuffer command_buffer, const Camera camera) {
    if (!lightingPipeline) return;
    // Bind lighting pipeline and GBuffer descriptor set + scene descriptor set
    lightingPipeline->bind(command_buffer);
    std::array<VkDescriptorSet,2> sets = { gbufferDescriptorSet, sceneDescriptorSet };
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lightingPipelineLayout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
    
    PushConstantDataJustMatrix pc{};
    pc.projection_view = camera.getProjection() * camera.getView();
    vkCmdPushConstants(command_buffer, lightingPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);

    
    // Draw fullscreen triangle inside active swapchain render pass
    vkCmdDraw(command_buffer, 3, 1, 0, 0);
}


  void RenderSystem::renderObjects(VkCommandBuffer command_buffer, std::vector<Object> &objects, const Camera camera, double frame_time) {
    // Deprecated: keep compatibility by performing both passes if called
    renderToGBuffer(command_buffer, objects, camera, frame_time);
    // lighting must be inside swapchain render pass; if caller is inside it, draw lighting too
    renderLighting(command_buffer, camera);
}


  void RenderSystem::createPipelineLayout() {
      VkPushConstantRange pushConstantRange{};
      pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
      pushConstantRange.offset = 0;
      pushConstantRange.size = sizeof(PushConstantData);


      std::array<VkDescriptorSetLayout, 3> descriptorSetLayouts = {
          descriptorSetLayout,
          sceneDescriptorSetLayout,
          objectDescriptorSetLayout
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
      // Material descriptor set: allow up to 3 combined image samplers (albedo, normal, displacement)
      VkDescriptorSetLayoutBinding samplerBindings[3];
      for (uint32_t i = 0; i < 3; ++i) {
          samplerBindings[i].binding = i;
          samplerBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
          samplerBindings[i].descriptorCount = 1;
          samplerBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
          samplerBindings[i].pImmutableSamplers = nullptr;
      }
      
      VkDescriptorSetLayoutCreateInfo layoutInfo{};
      layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      layoutInfo.bindingCount = 3;
      layoutInfo.pBindings = samplerBindings;
      
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
      // support up to 3 samplers per material (albedo, normal, displacement)
      // assume up to 100 material/object descriptor sets + 3 gbuffer samplers
      poolSizes[0].descriptorCount = 100 * 3 + 3;
      
      poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      // we need storage buffer descriptors for scene + object buffers
      poolSizes[1].descriptorCount = 2;
      
      VkDescriptorPoolCreateInfo poolInfo{};
      poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
      poolInfo.pPoolSizes = poolSizes.data();
      poolInfo.maxSets = 102;  // 100 object sets + 1 scene set + gbuffer
      
      if (vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
          throw std::runtime_error("failed to create descriptor pool!");
      }
  }
  
  const int lights_am = 33;

  void RenderSystem::createSceneDataBuffer() {
      // initialize RNG for random color generation
      std::random_device rd;
      std::mt19937 rng(rd());
      std::uniform_real_distribution<float> dist(0.0f, 1.0f);

      scene_data.point_lights.resize(lights_am);
      for (int i = 0; i < lights_am; i++) {
        // keep a fixed initial position; color is random per light
        scene_data.point_lights[i].position = glm::vec4(0.f, 0.f, 2.f, 1.f);
        scene_data.point_lights[i].color = glm::vec4(dist(rng), dist(rng), dist(rng), 1.f);
        std::cout << scene_data.point_lights[i].color.x << scene_data.point_lights[i].color.y << scene_data.point_lights[i].color.z << std::endl;
      }
      

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
      
      for (int i = 0; i < lights_am; i++) {
        scene_data.point_lights[i].position += (scene_data.point_lights[i].color - 0.5f) * 0.001f; // glm::vec4(sin(i) * 4., 0.f, cos(i) * 4., 1.f);
        }

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

  void RenderSystem::createGBufferDescriptorSet() {
      // allocate descriptor set for gbuffer samplers
      VkDescriptorSetAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      allocInfo.descriptorPool = descriptorPool;
      allocInfo.descriptorSetCount = 1;
      allocInfo.pSetLayouts = &gbufferDescriptorSetLayout;

      if (vkAllocateDescriptorSets(device.getDevice(), &allocInfo, &gbufferDescriptorSet) != VK_SUCCESS) {
          throw std::runtime_error("failed to allocate gbuffer descriptor set!");
      }

      auto views = gBuffer->getColorImageViews();
      VkDescriptorImageInfo imageInfos[3];
      for (uint32_t i = 0; i < 3; ++i) {
          imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
          imageInfos[i].imageView = views[i];
          imageInfos[i].sampler = gBuffer->getSampler();
      }

      std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
      for (uint32_t i = 0; i < 3; ++i) {
          descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          descriptorWrites[i].dstSet = gbufferDescriptorSet;
          descriptorWrites[i].dstBinding = i;
          descriptorWrites[i].dstArrayElement = 0;
          descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
          descriptorWrites[i].descriptorCount = 1;
          descriptorWrites[i].pImageInfo = &imageInfos[i];
      }

      vkUpdateDescriptorSets(device.getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
  }


  void RenderSystem::createLightingPipeline(const std::string& vertPath, const std::string& fragPath) {
      // create pipeline layout specific to lighting pass: gbuffer samplers + scene data
      std::array<VkDescriptorSetLayout, 2> layouts = { gbufferDescriptorSetLayout, sceneDescriptorSetLayout };

      VkPipelineLayoutCreateInfo layoutInfo{};
      layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
      layoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
      layoutInfo.pSetLayouts = layouts.data();
      
      VkPushConstantRange pushConstantRange{};
      pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
      pushConstantRange.offset = 0;
      pushConstantRange.size = sizeof(PushConstantDataJustMatrix);

      layoutInfo.pushConstantRangeCount = 1;
      layoutInfo.pPushConstantRanges = &pushConstantRange;

      if (vkCreatePipelineLayout(device.getDevice(), &layoutInfo, nullptr, &lightingPipelineLayout) != VK_SUCCESS) {
          throw std::runtime_error("failed to create lighting pipeline layout!");
      }

      PipelineConfigInfo config{};
      Pipeline::defaultPipelineConfigInfo(config);
      config.useVertexInput = false; // fullscreen pass
      config.pipelineLayout = lightingPipelineLayout;
      config.renderPass = render_pass;

      lightingPipeline = std::make_unique<Pipeline>(device, vertPath, fragPath, config);
  }

 }  // namespace baka