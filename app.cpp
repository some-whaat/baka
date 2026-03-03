#include "app.hpp"

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
  float time;
  // glm::vec2 offset;
  alignas(16) glm::vec3 color;
  
};

App::App() {
  loadModels();
  createPipelineLayout();
  createPipeline();
}

App::~App() { vkDestroyPipelineLayout(device.getDevice(), pipeline_layout, nullptr); }

void App::run() {
  while (!window.shouldClose()) {
    glfwPollEvents();
    
    if (auto command_buffer = renderer.beginFrame()) {
      renderer.beginSwapChainRenderPass(command_buffer);
      renderObjects(command_buffer);
      renderer.endSwapChainRenderPass(command_buffer);
      renderer.endFrame();
       
    }

  }
  vkDeviceWaitIdle(device.getDevice());
}

std::unique_ptr<Model> createCubeModel(Device& device) {
  std::vector<Model::Vertex> vertices{
 
      // left face (white)
      {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
      {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
      {{-.5f, -.5f, .5f}, {.9f, .9f, .9f}},
      {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
      {{-.5f, .5f, -.5f}, {.9f, .9f, .9f}},
      {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
 
      // right face (yellow)
      {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
      {{.5f, .5f, .5f}, {.8f, .8f, .1f}},
      {{.5f, -.5f, .5f}, {.8f, .8f, .1f}},
      {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
      {{.5f, .5f, -.5f}, {.8f, .8f, .1f}},
      {{.5f, .5f, .5f}, {.8f, .8f, .1f}},
 
      // top face (orange, remember y axis points down)
      {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
      {{.5f, -.5f, .5f}, {.9f, .6f, .1f}},
      {{-.5f, -.5f, .5f}, {.9f, .6f, .1f}},
      {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
      {{.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
      {{.5f, -.5f, .5f}, {.9f, .6f, .1f}},
 
      // bottom face (red)
      {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
      {{.5f, .5f, .5f}, {.8f, .1f, .1f}},
      {{-.5f, .5f, .5f}, {.8f, .1f, .1f}},
      {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
      {{.5f, .5f, -.5f}, {.8f, .1f, .1f}},
      {{.5f, .5f, .5f}, {.8f, .1f, .1f}},
 
      // nose face (blue)
      {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
      {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
      {{-.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
      {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
      {{.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
      {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
 
      // tail face (green)
      {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
      {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
      {{-.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
      {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
      {{.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
      {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
 
  };

  // for (auto& v : vertices) {
  //   // v.position = v.position * rot_mat_x;
  //   // v.position += offset;
  // }
  return std::make_unique<Model>(device, vertices);

}

void App::loadModels() {
  // std::vector<Model::Vertex> vertices{
  //   {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
  //   {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
  //   {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
  // };

  std::shared_ptr<Model> model = createCubeModel(device);

  auto cube = Object();
  cube.model = model;
  // cube.color = {.1f, .8f, .1f};
  cube.transform.pos = {.0f, .0f, .5f,};
  cube.transform.scale = {.5f, .5f, .5f};
  cube.transform.rot = {0.f, 0.f, 0.f};//glm::vec3(.25f * 6.28f, 0.f, 0.f);

  objects.push_back(std::move(cube));
}

void App::renderObjects(VkCommandBuffer command_buffer) {
  pipeline->bind(command_buffer);

  static float anim = 0;

  anim += 0.01f;

  for (auto& obj : objects) {
    obj.transform.rot.y = obj.transform.rot.y + 0.01f;
    obj.transform.rot.x = obj.transform.rot.x + 0.02f;
    // std::cout << obj.transform.rot.y;

    PushConstantData push{};
    // push.offset = obj.transform.pos;
    // push.color = obj.color;
    glm::mat4 maat = obj.transform.getMat4();
    push.transform = maat;
    push.time = anim;
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

void App::createPipelineLayout() {
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


void App::createPipeline() {

  assert(pipeline_layout != nullptr && "Cannot create pipeline before pipeline layout");

  PipelineConfigInfo pipelineConfig{};
  Pipeline::defaultPipelineConfigInfo(pipelineConfig);
  pipelineConfig.renderPass = renderer.getSwapChainRenderPass();
  pipelineConfig.pipelineLayout = pipeline_layout;
  pipeline = std::make_unique<Pipeline>(
      device,
      "shaders/first_shader.vert.spv",
      "shaders/first_shader.frag.spv",
      pipelineConfig);

  assert(pipeline_layout != nullptr && "Cannot create pipeline before pipeline layout");
}


void App::updateFrameRate() {
    double current_time = glfwGetTime();
    frame_count++;
    
    if (current_time - last_time >= 1.0) {
        fps = frame_count / (current_time - last_time);
        frame_count = 0;
        last_time = current_time;
    }

    window.changeTitle("fps: " + std::to_string(fps));
}

}  // namespace baka