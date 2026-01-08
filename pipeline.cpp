#include "pipeline.hpp"

// std
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace baka {

    Pipeline::Pipeline(
            const std::string& vert_filepath,
            const std::string& frag_filepath,
            Device& _device,
            const PipelineConfigInfo& config_info) : device{_device} {
            
        createGraphicsPipeline(vert_filepath, frag_filepath, config_info);
        
    }

    Pipeline::~Pipeline() {}

    std::vector<char> Pipeline::readFile(const std::string& filepath) {
        std::ifstream file{filepath, std::ios::ate | std::ios::binary}; // std::ios::ate = go to the end of the file

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file: " + filepath);
        }

        size_t file_size = static_cast<size_t>(file.tellg()); // bc we used std::ios::ate, our position = file size
        std::vector<char> buffer(file_size);

        file.seekg(0);
        file.read(buffer.data(), file_size);

        file.close();
        return buffer;
    }

    void Pipeline::createGraphicsPipeline(const std::string& vert_filepath, const std::string& frag_filepath, const PipelineConfigInfo& config_info) {

        auto vert_code = readFile(vert_filepath);
        auto frag_code = readFile(frag_filepath);

        std::cout << "Vertex Shader Code Size: " << vert_code.size() << '\n';
        std::cout << "Fragment Shader Code Size: " << frag_code.size() << '\n';
    }

    void Pipeline::createShaderModule(const std::vector<char>& code, VkShaderModule* shader_module) {

        VkShaderModuleCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = code.size();
        create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

        if (vkCreateShaderModule(device.device(), &create_info, nullptr, shader_module) != VK_SUCCESS) {

            throw std::runtime_error("failed to create shader module");
        }
    }

    PipelineConfigInfo Pipeline::defaultPipelineConfigInfo(uint32_t width, uint32_t height) {

        PipelineConfigInfo config_info{};

        return config_info;
    }

}  // namespace baka