#include "camera.hpp"

// std
#include <cassert>
#include <limits>

namespace baka {
    void Camera::setOrthographicProj(float left, float right, float top, float bottom, float near, float far) {
        projection_matrix = glm::mat4{1.0f};
        projection_matrix[0][0] = 2.f / (right - left);
        projection_matrix[1][1] = 2.f / (bottom - top);
        projection_matrix[2][2] = 1.f / (far - near);
        projection_matrix[3][0] = -(right + left) / (right - left);
        projection_matrix[3][1] = -(bottom + top) / (bottom - top);
        projection_matrix[3][2] = -near / (far - near);
    }

    void Camera::setPerspectiveProj(float fovy, float aspect, float near, float far) {
        assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
        const float tanHalfFovy = tan(fovy / 2.f);
        projection_matrix = glm::mat4{0.0f};
        projection_matrix[0][0] = 1.f / (aspect * tanHalfFovy);
        projection_matrix[1][1] = 1.f / (tanHalfFovy);
        projection_matrix[2][2] = far / (far - near);
        projection_matrix[2][3] = 1.f;
        projection_matrix[3][2] = -(far * near) / (far - near);
        }

}