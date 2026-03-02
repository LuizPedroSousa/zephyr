#pragma once

#include "glm/gtx/quaternion.hpp"
#include "keyboard.hpp"
#include "mesh.hpp"
#include "time.hpp"
#include "window.hpp"
#include <glm/ext/vector_float3.hpp>

namespace zephyr {

struct TransformComponent {
  glm::vec3 position;
  glm::vec3 scale;
  glm::quat rotation;
  bool is_dirty;
  glm::mat4 matrix;

  void recalculate() {
    if (!is_dirty) {
      return;
    }

    if (glm::length(rotation) == 0.0f) {
      rotation = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), scale);
    glm::mat4 rotation_matrix = glm::toMat4(rotation);

    matrix = translation_matrix * rotation_matrix * scale_matrix;
    is_dirty = false;
  }

  void translate(glm::vec3 p_position) {
    this->matrix = glm::translate(this->matrix, p_position);
    this->position = p_position;

    is_dirty = true;
  }

  void set_scale(glm::vec3 p_scale) {
    this->matrix = glm::scale(this->matrix, p_scale);
    this->scale = p_scale;

    is_dirty = true;
  }

  glm::vec3 forward() { return glm::normalize(glm::vec3(matrix[2])); }
};

struct CameraComponent {
  glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 front = glm::vec3(0.0f, 0.0f, 1.0f);

  glm::mat4 view_matrix;
  glm::mat4 projection_matrix;
  glm::vec3 rotation;
  glm::vec3 direction = glm::vec3(0.0f, 0.0f, 1.0f);

  float yaw = 90.0f;
  float pitch = 0.0f;
  float speed = 4.0f;

  float sensitivity = 0.1f;

  void recalculate(TransformComponent &transform, VkExtent2D extent) {
    auto matrix = glm::mat4(1.0f);

    matrix = glm::lookAt(transform.position, transform.position + front, up);

    view_matrix = matrix;

    projection_matrix = glm::perspective(
        45.0f, (float)extent.width / (float)extent.height, 0.1f, 100.0f);
  }

  void movement(TransformComponent &transform, VkExtent2D extent) {
    using namespace zephyr::input;

    if (IS_KEY_DOWN(KeyCode::W)) {
      transform.position += direction * speed * Time::get()->deltatime();
      transform.is_dirty = true;
    }

    if (IS_KEY_DOWN(KeyCode::S)) {
      transform.position -= direction * speed * Time::get()->deltatime();
      transform.is_dirty = true;
    }

    if (IS_KEY_DOWN(KeyCode::A)) {
      transform.position -= glm::normalize(glm::cross(front, up)) * speed *
                            Time::get()->deltatime();
      transform.is_dirty = true;
    }

    if (IS_KEY_DOWN(KeyCode::D)) {
      transform.position += glm::normalize(glm::cross(front, up)) * speed *
                            Time::get()->deltatime();
      transform.is_dirty = true;
    }

    if (IS_KEY_DOWN(KeyCode::Space)) {
      transform.position.y += speed * Time::get()->deltatime();
      transform.is_dirty = true;
    }

    if (IS_KEY_DOWN(KeyCode::LeftControl)) {
      transform.position.y -= speed * Time::get()->deltatime();
      transform.is_dirty = true;
    }

    auto mouse = MOUSE_DELTA();

    if (mouse.x != 0 || mouse.y != 0) {
      yaw += mouse.x * sensitivity;
      pitch -= mouse.y * sensitivity;

      if (pitch > 89.0f)
        pitch = 89.0f;
      if (pitch < -89.0f)
        pitch = -89.0f;

      direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
      direction.y = sin(glm::radians(pitch));
      direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

      front = glm::normalize(direction);
    }

    recalculate(transform, extent);
    transform.recalculate();
  }
};

struct MeshComponent {
  Mesh mesh = Mesh::cube();
};

struct CameraTagComponent {};
struct ObjectTagComponent {};

} // namespace zephyr
