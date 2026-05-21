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
  camera.setViewDirection(glm::vec3(0.f), glm::vec3(0.0f, 0.f, 1.0f));
}

App::~App() {}

void App::run() {
  RenderSystem render_system{device, renderer.getSwapChainRenderPass()};

  render_system.setupObjectDescriptors(objects);

  double current_time = glfwGetTime();

  static int frame_count = 0;

  while (!window.shouldClose()) {
    glfwPollEvents();
    
    double new_time = glfwGetTime();
    double frame_time =  new_time - current_time;
    current_time = new_time;
    
    
    if (frame_count >= 60) {
      double fps = 1./frame_time;
      window.changeTitle("fps: " + std::to_string(fps));
      frame_count = 0;
    }

    
    float aspect = renderer.getAspectRatio();
    // ============================================================|
    // camera.setOrthographicProj(-aspect, aspect, -1, 1, -1, 1);// <=| works correctly only if top, bottom are 1, -1
    // ============================================================|
    camera.setPerspectiveProj(glm::radians(50.f), aspect, 0.1f, 10);


    if (auto command_buffer = renderer.beginFrame()) {
      renderer.beginSwapChainRenderPass(command_buffer);
      render_system.renderObjects(command_buffer, objects, camera, /* TEMPORARY, delete */ frame_time);
      renderer.endSwapChainRenderPass(command_buffer);
      renderer.endFrame();
       
    }

    frame_count += 1;
  }
  vkDeviceWaitIdle(device.getDevice());
}

// std::unique_ptr<Model> createCubeModel(Device& device) {
//   Model::Builder model_builder{};
//   model_builder.vertices = {
 
//       // left face (white)
//       {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
//       {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
//       {{-.5f, -.5f, .5f}, {.9f, .9f, .9f}},
//       {{-.5f, .5f, -.5f}, {.9f, .9f, .9f}},
 
//       // right face (yellow)
//       {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
//       {{.5f, .5f, .5f}, {.8f, .8f, .1f}},
//       {{.5f, -.5f, .5f}, {.8f, .8f, .1f}},
//       {{.5f, .5f, -.5f}, {.8f, .8f, .1f}},
 
//       // top face (orange, remember y axis points down)
//       {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
//       {{.5f, -.5f, .5f}, {.9f, .6f, .1f}},
//       {{-.5f, -.5f, .5f}, {.9f, .6f, .1f}},
//       {{.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
 
//       // bottom face (red)
//       {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
//       {{.5f, .5f, .5f}, {.8f, .1f, .1f}},
//       {{-.5f, .5f, .5f}, {.8f, .1f, .1f}},
//       {{.5f, .5f, -.5f}, {.8f, .1f, .1f}},
 
//       // nose face (blue)
//       {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
//       {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
//       {{-.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
//       {{.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
 
//       // tail face (green)
//       {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
//       {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
//       {{-.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
//       {{.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
 
//   };

//   model_builder.indices = {0,  1,  2,  0,  3,  1,  4,  5,  6,  4,  7,  5,  8,  9,  10, 8,  11, 9,
//                         12, 13, 14, 12, 15, 13, 16, 17, 18, 16, 19, 17, 20, 21, 22, 20, 23, 21};


//   // for (auto& v : vertices) {
//   //   // v.position = v.position * rot_mat_x;
//   //   // v.position += offset;
//   // }
//   return std::make_unique<Model>(device, model_builder);

// }

void App::loadObjects() {

  std::shared_ptr<Model> model = Model::createModelFromFile(device, "/home/somewhat/projects/grathics_stuff/baka/models/cute_building/viking_room.obj"); // createCubeModel(device);
  
  std::shared_ptr<Texture> texture = std::make_shared<Texture>(device.getDevice(), device.getPhysicalDevice(), device.getCommandPool(), device.getGraphicsQueue());
  texture->loadFromFile("pictures/viking_room.png");
  texture->createImageView(VK_FORMAT_R8G8B8A8_SRGB);
  texture->createSampler();

  auto obj = Object();
  obj.model = model;
  obj.texture = texture;
  // obj.color = {.1f, .8f, .1f};
  obj.transform.pos = {0.0f, .0f, 3.f,};
  obj.transform.scale = {.5f, .5f, .5f};
  obj.transform.rot = {0.f, 0.f, 0.f};//glm::vec3(.25f * 6.28f, 0.f, 0.f);

  objects.push_back(std::move(obj));
}

}  // namespace baka