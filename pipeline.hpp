#pragma once

#include "device.hpp"

#include <string>
#include <vector>

namespace baka {

// I've desided to give up on my naming convention for this thing as all Vulcan vars named likeThat anyway
struct PipelineConfigInfo {
  PipelineConfigInfo(const PipelineConfigInfo&) = delete;
  PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

  VkViewport viewport;
  VkRect2D scissor; 
  VkPipelineViewportStateCreateInfo viewportInfo;
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
  VkPipelineRasterizationStateCreateInfo rasterizationInfo;
  VkPipelineMultisampleStateCreateInfo multisampleInfo;
  VkPipelineColorBlendAttachmentState colorBlendAttachment;
  VkPipelineColorBlendStateCreateInfo colorBlendInfo;
  VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
  std::vector<VkDynamicState> dynamicStateEnables;
  VkPipelineDynamicStateCreateInfo dynamicStateInfo;
  VkPipelineLayout pipelineLayout = nullptr;
  VkRenderPass renderPass = nullptr;
  uint32_t subpass = 0;
};


class Pipeline {
    public:
        Pipeline(
            const std::string& vert_filepath,
            const std::string& frag_filepath,
            Device& _device,
            const PipelineConfigInfo& config_info
        );

        ~Pipeline();
        
        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;

        static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);

        void bind(VkCommandBuffer command_buffer);

    private:

        static std::vector<char> readFile(const std::string& filepath);

        void createGraphicsPipeline(
            const std::string& vert_filepath,
            const std::string& frag_filepath,
            const PipelineConfigInfo& config_info
        );

        void createShaderModule(const std::vector<char>& code, VkShaderModule* shader_module);
        
                
        Device& device;
        VkPipeline graphics_pipeline;
        VkShaderModule vert_shader_module;
        VkShaderModule frag_shader_module;

};

}  // namespace baka