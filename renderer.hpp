#pragma once

#include "window.hpp"
#include "pipeline.hpp"
#include "device.hpp"
#include "swap_chain.hpp"
#include "model.hpp"
#include "object.hpp"


// std
#include <cassert>
#include <memory>
#include <vector>

namespace baka {

class Renderer {

    public:
        static constexpr int WIDTH = 1600;
        static constexpr int HEIGHT = 1200;

        float fps = 0;

        Renderer();
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer &operator=(const Renderer&) = delete;

        void run();


    private:
        void loadModels();
        void createPipelineLayout();
        void createPipeline();
        void createCommandBuffers();
        void drawFrame();
        void recreateSwapChain();
        void recordCommandBuffer(int image_index);

        double last_time = glfwGetTime();
        int frame_count = 0;

        void updateFrameRate();

        Window window{WIDTH, HEIGHT, "YAY, Vulkan!"};
        Device device{window};
        std::unique_ptr<SwapChain> swap_chain;
        std::unique_ptr<Pipeline> pipeline;//{"shaders/first_shader.vert.spv", "shaders/first_shader.frag.spv", device, Pipeline::defaultPipelineConfigInfo(WIDTH, HEIGHT)};
        VkPipelineLayout pipeline_layout;
        std::vector<VkCommandBuffer> command_buffers;
        std::unique_ptr<Model> model;
};

// class Renderer {

//  public:

//   Renderer(Window &window, Device &device);
//   ~Renderer();

//   Renderer(const Renderer &) = delete;
//   Renderer &operator=(const Renderer &) = delete;

//   VkRenderPass getSwapChainRenderPass() const { return swap_chain->getRenderPass(); }
//   bool isFrameInProgress() const { return isFrameStarted; }

//   VkCommandBuffer getCurrentCommandBuffer() const {
//     assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
//     return commandBuffers[currentFrameIndex];
//   }

//   int getFrameIndex() const {
//     assert(isFrameStarted && "Cannot get frame index when frame not in progress");
//     return currentFrameIndex;
//   }

//   VkCommandBuffer beginFrame();
//   void endFrame();
//   void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
//   void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

//  private:
//   void createCommandBuffers();
//   void freeCommandBuffers();
//   void recreateSwapChain();

//   Window &window;
//   Device &device;
//   std::unique_ptr<SwapChain> swap_chain;
//   std::vector<VkCommandBuffer> commandBuffers;

//   uint32_t currentImageIndex;
//   int currentFrameIndex{0};
//   bool isFrameStarted{false};
// };

}  // namespace 