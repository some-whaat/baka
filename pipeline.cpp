#include "pipeline.hpp"

// std
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace baka {

    Pipeline::Pipeline(const std::string& vert_filepath, const std::string& frag_filepath) {
        createGraphicsPipeline(vert_filepath, frag_filepath);
    }

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

    void Pipeline::createGraphicsPipeline(const std::string& vert_filepath, const std::string& frag_filepath) {
        
        auto vert_code = readFile(vert_filepath);
        auto frag_code = readFile(frag_filepath);

        std::cout << "Vertex Shader Code Size: " << vert_code.size() << '\n';
        std::cout << "Fragment Shader Code Size: " << frag_code.size() << '\n';
    }

}  // namespace baka