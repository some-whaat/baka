#include "render_system.hpp"

// std
#include <array>
#include <stdexcept>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace baka {
  	
struct PushConstantData {
  glm::mat4 transform{1.f};
//   float time;
  // glm::vec3 normal;
  //  alignas(16) glm::vec3 color;
  
};

RenderSystem::RenderSystem(Device &_device, VkRenderPass render_pass) : device{_device} {
  createDescriptorSetLayout();
  createDescriptorPool();
  createPipelineLayout();
  createPipeline(render_pass);
}

RenderSystem::~RenderSystem() { vkDestroyPipelineLayout(device.getDevice(), pipeline_layout, nullptr); }


void RenderSystem::renderObjects(VkCommandBuffer command_buffer, std::vector<Object> &objects, const Camera camera, /* TEMPORARY, delete */ double frame_time) {
  pipeline->bind(command_buffer);

//   static float anim = 0;
//   anim += 0.01f;

    auto projection_view = camera.getProjection() * camera.getView();

  for (auto& obj : objects) {
     // For each object allocate+update a descriptor set that points to its texture, then bind it
    VkDescriptorSet localDescriptorSet = VK_NULL_HANDLE;
    if (obj.texture) {
      VkDescriptorSetAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      allocInfo.descriptorPool = descriptorPool;
      allocInfo.descriptorSetCount = 1;
      allocInfo.pSetLayouts = &descriptorSetLayout;

      if (vkAllocateDescriptorSets(device.getDevice(), &allocInfo, &localDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor set for object!");
      }

      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      imageInfo.imageView = obj.texture->getImageView();
      imageInfo.sampler = obj.texture->getSampler();

      VkWriteDescriptorSet descriptorWrite{};
      descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrite.dstSet = localDescriptorSet;
      descriptorWrite.dstBinding = 0;
      descriptorWrite.dstArrayElement = 0;
      descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrite.descriptorCount = 1;
      descriptorWrite.pImageInfo = &imageInfo;

      vkUpdateDescriptorSets(device.getDevice(), 1, &descriptorWrite, 0, nullptr);

      vkCmdBindDescriptorSets(
          command_buffer,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          pipeline_layout,
          0,
          1,
          &localDescriptorSet,
          0,
          nullptr);
    }

    obj.transform.rot.y = obj.transform.rot.y + frame_time;
    obj.transform.rot.x = 90;

    PushConstantData push{};
    // push.offset = obj.transform.pos;
    // push.color = obj.color;

    // =========================================================================|
    glm::mat4 maat = projection_view * obj.transform.getMat4(); // <====== TEMPORARY (better send both and calculate on GPU)
    // =========================================================================|
    
    push.transform = maat;
    
    // push.time = anim;
    // std::cout << maat[0][0];
    // push.rot = obj.transform.rot.x;

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

  VkPipelineLayoutCreateInfo pipeline_layoutInfo{};
  pipeline_layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layoutInfo.setLayoutCount = 1;
  pipeline_layoutInfo.pSetLayouts = &descriptorSetLayout;
  pipeline_layoutInfo.pushConstantRangeCount = 1;
  pipeline_layoutInfo.pPushConstantRanges = &pushConstantRange;
  if (vkCreatePipelineLayout(device.getDevice(), &pipeline_layoutInfo, nullptr, &pipeline_layout) !=
      VK_SUCCESS) {
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
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;  // binding number in shader
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;  // Used in fragment shader
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;
    
    if (vkCreateDescriptorSetLayout(device.getDevice(), &layoutInfo, nullptr, &descriptorSetLayout) 
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void RenderSystem::createDescriptorPool() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    // Allow multiple descriptor sets (one per object). Make this large enough for typical scenes.
    poolSize.descriptorCount = 100;
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 100;  // Maximum number of descriptor sets that can be allocated
    
    if (vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &descriptorPool) 
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}


// void RenderSystem::updateFrameRate() {
//     double current_time = glfwGetTime();
//     frame_count++;
    
//     if (current_time - last_time >= 1.0) {
//         fps = frame_count / (current_time - last_time);
//         frame_count = 0;
//         last_time = current_time;
//     }

//     window.changeTitle("fps: " + std::to_string(fps));
// }

}  // namespace baka