#pragma once

#include <string>
#include <vector>

namespace baka {

    class Pipeline {
        public:
            Pipeline(const std::string& vertFilepath, const std::string& frag_filepath);

        private:
        
            static std::vector<char> readFile(const std::string& filepath);

            void createGraphicsPipeline(const std::string& vertFilepath, const std::string& frag_filepath);
    };

}  // namespace baka