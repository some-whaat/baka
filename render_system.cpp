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
  // glm::vec2 offset;
//   alignas(16) glm::vec3 color;
  
};

RenderSystem::RenderSystem(Device &_device, VkRenderPass render_pass) : device{_device} {
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
    obj.transform.rot.y = obj.transform.rot.y + frame_time;
    obj.transform.rot.x = obj.transform.rot.x + frame_time * 2.f;

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
  pipeline_layoutInfo.setLayoutCount = 0;
  pipeline_layoutInfo.pSetLayouts = nullptr;
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