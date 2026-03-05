// responcble for device and swap_chain, drawing to the window. calculates fps too

#pragma once

#include "window.hpp"
#include "device.hpp"
#include "swap_chain.hpp"

// std
#include <cassert>
#include <memory>
#include <vector>

namespace baka {

class Renderer {

    public:
        // static constexpr int WIDTH = 1600;
        // static constexpr int HEIGHT = 1200;

        float fps = 0;

        Renderer(Window &_window, Device &_device);
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer &operator=(const Renderer&) = delete;


        VkRenderPass getSwapChainRenderPass() const { return swap_chain->getRenderPass(); }
        bool isFrameInProgress() const { return is_frame_started; }

        VkCommandBuffer getCurrentCommandBuffer() const {
        assert(is_frame_started && "Cannot get command buffer when frame not in progress");
        return command_buffers[curr_img_index];
        }

        int getFrameIndex() const {
            assert(is_frame_started && "Cannot get frame index when frame not in progress");
            return curr_img_index;
        }

        VkCommandBuffer beginFrame();
        void endFrame();

        void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
        void endSwapChainRenderPass(VkCommandBuffer commandBuffer);
 

    private:
    
        void createCommandBuffers(); 
        void freeCommandBuffers();
        // void drawFrame();
        void recreateSwapChain();

        uint32_t curr_img_index;
        bool is_frame_started = false;

        double last_time = glfwGetTime();
        int frame_count = 0;

        void updateFrameRate();

        Window& window;
        Device& device;

        std::unique_ptr<SwapChain> swap_chain;
        std::vector<VkCommandBuffer> command_buffers;
};
    
}  // namespace baka