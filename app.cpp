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
    camera.setPerspectiveProj(glm::radians(50.f), aspect, 0.1f, 100.);

    camera.updatePosRotKeys(window.getWindow(), frame_time);

    if (auto command_buffer = renderer.beginFrame()) {
      // render geometry into GBuffer before starting swapchain render pass
      render_system.renderGeometry(command_buffer, objects, camera, frame_time);

      renderer.beginSwapChainRenderPass(command_buffer);
      // lighting/composite pass - must be inside swapchain render pass
      render_system.renderLighting(command_buffer, camera);
      renderer.endSwapChainRenderPass(command_buffer);
      renderer.endFrame();
       
    }

    frame_count += 1;
  }
  vkDeviceWaitIdle(device.getDevice());
}

void App::loadObjects() {

  std::shared_ptr<Model> model = Model::createModelFromFile(device, "/home/somewhat/projects/grathics_stuff/baka/models/cute_building/viking_room.obj"); // createCubeModel(device);
  
  std::shared_ptr<Texture> texture = std::make_shared<Texture>(device.getDevice(), device.getPhysicalDevice(), device.getCommandPool(), device.getGraphicsQueue());
  texture->loadFromFile("pictures/viking_room.png");
  texture->createImageView(VK_FORMAT_R8G8B8A8_SRGB);
  texture->createSampler();

  auto mat = std::make_shared<Material>(texture, "shaders/first_shader.vert.spv", "shaders/first_shader.frag.spv");

  auto obj = Object();
  obj.model = model;
  obj.material = mat;
  obj.transform.pos = {0.0f, .0f, 3.f,};
  obj.transform.scale = {.5f, .5f, .5f};
  obj.transform.rot = {0.f, 0.f, 0.f};//glm::vec3(.25f * 6.28f, 0.f, 0.f);
  objects.push_back(std::move(obj));

  auto floor = Object();
  floor.model = Model::createGrid(device, 2., 99);

  std::shared_ptr<Texture> floorTex = std::make_shared<Texture>(device.getDevice(), device.getPhysicalDevice(), device.getCommandPool(), device.getGraphicsQueue(), "pictures/blackwhight_guy.png");
  std::shared_ptr<Material> floorMat = std::make_shared<Material>(floorTex, "shaders/displacement.vert.spv", "shaders/first_shader.frag.spv");
  std::shared_ptr<Texture> floor_disp_map = std::make_shared<Texture>(device.getDevice(), device.getPhysicalDevice(), device.getCommandPool(), device.getGraphicsQueue(), "pictures/blackwhight_guy.png");
  std::shared_ptr<Texture> floor_normal_map = std::make_shared<Texture>(device.getDevice(), device.getPhysicalDevice(), device.getCommandPool(), device.getGraphicsQueue(), "pictures/wavy_normal.jpg");
  floorMat->setDisplacementMap(floor_disp_map);
  floorMat->setNormalMap(floor_normal_map);
  floor.material = floorMat;

  floor.transform.pos = {0.0f, .5f, 3.f,};
  floor.transform.scale = {1.f, 1.f, 1.f};

  objects.push_back(std::move(floor));

  auto chick = Object();
  chick.model =   Model::createModelFromFile(device, "models/chickentative/chickentative.obj");
  std::shared_ptr<Texture> chickTex = std::make_shared<Texture>(device.getDevice(), device.getPhysicalDevice(), device.getCommandPool(), device.getGraphicsQueue(), "pictures/catapillar.jpg");
  std::shared_ptr<Material> chickMat = std::make_shared<Material>(chickTex, "shaders/first_shader.vert.spv", "shaders/first_shader.frag.spv");
  chick.material = chickMat;
  chick.transform.pos = {0.0f, .5f, 3.f,};
  chick.transform.scale = {1.f, 1.f, 1.f};
  chick.transform.rot = {1.57f, 0.0f, 0.0f};
  objects.push_back(std::move(chick));

  
}

}  // namespace baka