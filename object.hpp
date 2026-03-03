#pragma once

#include "model.hpp"


#include <memory>
#include <vector>

namespace baka {

    struct Transform {

        glm::vec3 pos = glm::vec3(0.f, 0.f, 0.f);
        glm::vec3 rot = glm::vec3(0.f, 0.f, 0.f); // Euler
        glm::vec3 scale = glm::vec3(1.f, 1.f, 1.f);

        Transform() {}

        glm::mat4 getMat4() {
            
            // float c3 = 1.f;
            // float s3 = 0.f;
            // float c2 = 1.f;
            // float s2 = 0.f;
            // float c1 = 1.f;
            // float s1 = 0.f;

            // if (rot.z != 0) {
            //     c3 = glm::cos(rot.z);
            //     s3 = glm::sin(rot.z);
            // }
            // if (rot.x != 0) {
            //     c2 = glm::cos(rot.x);
            //     s2 = glm::sin(rot.x);
            // }
            // if (rot.y != 0) {
            //     c1 = glm::cos(rot.y);
            //     s1 = glm::sin(rot.y);
            // }

            const float c3 = glm::cos(rot.z);
            const float s3 = glm::sin(rot.z);
            const float c2 = glm::cos(rot.x);
            const float s2 = glm::sin(rot.x);
            const float c1 = glm::cos(rot.y);
            const float s1 = glm::sin(rot.y);

            return glm::mat4{
            {
                scale.x * (c1 * c3 + s1 * s2 * s3),
                scale.x * (c2 * s3),
                scale.x * (c1 * s2 * s3 - c3 * s1),
                0.0f,
            },

            {
                scale.y * (c3 * s1 * s2 - c1 * s3),
                scale.y * (c2 * c3),
                scale.y * (c1 * c3 * s2 + s1 * s3),
                0.0f,
            },

            {
                scale.z * (c2 * s1),
                scale.z * (-s2),
                scale.z * (c1 * c2),
                0.0f,
            },

            {pos, 1.0f}};
        }

        // Transform standart() {
        //     return Transform();
        // }
    };
    

    class Object {

        public:


        Transform transform;
        // glm::vec3 color;
        std::shared_ptr<Model> model;

        Object() {};

        Object(std::shared_ptr<Model> _model, Transform _transform) {
            model = std::move(_model);
            transform = _transform;
        }

    };

}