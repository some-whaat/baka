#include "app.hpp"

#include "render_system.hpp"

// std
#include <array>
#include <stdexcept>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace baka {

App::App() {
  loadObjects();
  camera.setOrthographicProj(-1, 1, -1, 1, -1, 1);
  camera.setViewDirection(glm::vec3(0.f), glm::vec3(0.3f, 0.f, 1.0f));
}

App::~App() {}

void App::run() {
  RenderSystem render_system{device, renderer.getSwapChainRenderPass()};

  while (!window.shouldClose()) {
    glfwPollEvents();

    float aspect = renderer.getAspectRatio();
    // ============================================================|
    // camera.setOrthographicProj(-aspect, aspect, -1, 1, -1, 1);// <=| works correctly only if top, bottom are 1, -1
    // ============================================================|
    camera.setPerspectiveProj(glm::radians(50.f), aspect, 0.1f, 10);


    if (auto command_buffer = renderer.beginFrame()) {
      renderer.beginSwapChainRenderPass(command_buffer);
      render_system.renderObjects(command_buffer, objects, camera);
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

void App::loadObjects() {

  std::shared_ptr<Model> model = createCubeModel(device);

  auto cube = Object();
  cube.model = model;
  // cube.color = {.1f, .8f, .1f};
  cube.transform.pos = {0.0f, .0f, 5.f,};
  cube.transform.scale = {.5f, .5f, .5f};
  cube.transform.rot = {0.f, 0.f, 0.f};//glm::vec3(.25f * 6.28f, 0.f, 0.f);

  objects.push_back(std::move(cube));
}

}  // namespace baka