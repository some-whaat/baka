#pragma once

#include "device.hpp"

#include <string>
#include <vector>

namespace baka {

    struct PipelineConfigInfo
    {
        /* data */
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
            void operator=(const Pipeline&) = delete;

            static PipelineConfigInfo defaultPipelineConfigInfo(uint32_t width, uint32_t height);

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