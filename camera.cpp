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


    void Camera::setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) { 
        const glm::vec3 w{glm::normalize(direction)};
        const glm::vec3 u{glm::normalize(glm::cross(w, up))};
        const glm::vec3 v{glm::cross(w, u)};

        view_matrix = glm::mat4{1.f};
        view_matrix[0][0] = u.x;
        view_matrix[1][0] = u.y;
        view_matrix[2][0] = u.z;
        view_matrix[0][1] = v.x;
        view_matrix[1][1] = v.y;
        view_matrix[2][1] = v.z;
        view_matrix[0][2] = w.x;
        view_matrix[1][2] = w.y;
        view_matrix[2][2] = w.z;
        view_matrix[3][0] = -glm::dot(u, position);
        view_matrix[3][1] = -glm::dot(v, position);
        view_matrix[3][2] = -glm::dot(w, position);
    }

    void Camera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) { // for looking at something
        setViewDirection(position, target - position, up);
    }

    void Camera::setViewYXZ(glm::vec3 position, glm::vec3 rotation) { // Euler
        const float c3 = glm::cos(rotation.z);
        const float s3 = glm::sin(rotation.z);
        const float c2 = glm::cos(rotation.x);
        const float s2 = glm::sin(rotation.x);
        const float c1 = glm::cos(rotation.y);
        const float s1 = glm::sin(rotation.y);
        const glm::vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
        const glm::vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
        const glm::vec3 w{(c2 * s1), (-s2), (c1 * c2)};
        view_matrix = glm::mat4{1.f};
        view_matrix[0][0] = u.x;
        view_matrix[1][0] = u.y;
        view_matrix[2][0] = u.z;
        view_matrix[0][1] = v.x;
        view_matrix[1][1] = v.y;
        view_matrix[2][1] = v.z;
        view_matrix[0][2] = w.x;
        view_matrix[1][2] = w.y;
        view_matrix[2][2] = w.z;
        view_matrix[3][0] = -glm::dot(u, position);
        view_matrix[3][1] = -glm::dot(v, position);
        view_matrix[3][2] = -glm::dot(w, position);
        
    }

  struct KeyMappings {
    int moveLeft = GLFW_KEY_A;
    int moveRight = GLFW_KEY_D;
    int moveForward = GLFW_KEY_W;
    int moveBackward = GLFW_KEY_S;
    int moveUp = GLFW_KEY_E;
    int moveDown = GLFW_KEY_Q;
    int lookLeft = GLFW_KEY_LEFT;
    int lookRight = GLFW_KEY_RIGHT;
    int lookUp = GLFW_KEY_UP;
    int lookDown = GLFW_KEY_DOWN;
  };

  KeyMappings keys{};
  float moveSpeed{3.f};
  float lookSpeed{1.5f};

    // should it probably be somewhere else? yes. todo
void Camera::updatePosRotKeys(GLFWwindow* window, float dt) {
  glm::vec3 rotate{0};
  if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotate.y += 1.f;
  if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotate.y -= 1.f;
  if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotate.x += 1.f;
  if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotate.x -= 1.f;

  if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
    rot += lookSpeed * dt * glm::normalize(rotate);
  }

  // limit pitch values between about +/- 85ish degrees
  rot.x = glm::clamp(rot.x, -1.5f, 1.5f);
  rot.y = glm::mod(rot.y, glm::two_pi<float>());

  float yaw = rot.y;
  const glm::vec3 forwardDir{sin(yaw), 0.f, cos(yaw)};
  const glm::vec3 rightDir{forwardDir.z, 0.f, -forwardDir.x};
  const glm::vec3 upDir{0.f, -1.f, 0.f};

  glm::vec3 moveDir{0.f};
  if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
  if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
  if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
  if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
  if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
  if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;

  if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
    pos += moveSpeed * dt * glm::normalize(moveDir);
  }

  setViewYXZ(pos, rot);
}

    // void Camera::changeViewYXZ(glm::vec3 position_delta, glm::vec3 rotation_delta) { // Euler
    //     const float c3 = glm::cos(rotation_delta.z);
    //     const float s3 = glm::sin(rotation_delta.z);
    //     const float c2 = glm::cos(rotation_delta.x);
    //     const float s2 = glm::sin(rotation_delta.x);
    //     const float c1 = glm::cos(rotation_delta.y);
    //     const float s1 = glm::sin(rotation_delta.y);
    //     const glm::vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
    //     const glm::vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
    //     const glm::vec3 w{(c2 * s1), (-s2), (c1 * c2)};
    //     view_matrix = glm::mat4{1.f};
    //     view_matrix[0][0] = u.x;
    //     view_matrix[1][0] = u.y;
    //     view_matrix[2][0] = u.z;
    //     view_matrix[0][1] = v.x;
    //     view_matrix[1][1] = v.y;
    //     view_matrix[2][1] = v.z;
    //     view_matrix[0][2] = w.x;
    //     view_matrix[1][2] = w.y;
    //     view_matrix[2][2] = w.z;
    //     view_matrix[3][0] = -glm::dot(u, position);
    //     view_matrix[3][1] = -glm::dot(v, position);
    //     view_matrix[3][2] = -glm::dot(w, position);
        
    // }

}