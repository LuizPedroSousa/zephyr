#pragma once

#include "components.hpp"
#include "log.hpp"
#include <vector>

namespace zephyr {

struct EntityUniformBuffer {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 projection;
  alignas(16) float time;
  alignas(16) glm::vec3 view_position;
  alignas(16) glm::vec3 camera_forward;

  void update(const TransformComponent &transform,
              const CameraComponent &camera,
              const TransformComponent &camera_transform) {
    model = transform.matrix;
    view = camera.view_matrix;
    projection = camera.projection_matrix;

#if BACKEND == BACKEND_VULKAN
    projection[1][1] *= -1;
#endif

    static Timer timer;
    float elapsed_time = timer.elapsed();

    time = elapsed_time;
    view_position = camera_transform.position;
    camera_forward = camera.front;
  }
};

struct UniformTable {
  std::vector<EntityUniformBuffer> slots;
  std::unordered_map<EntityId, size_t> index;
  std::vector<size_t> free_slots;

  void allocate(EntityId id) {
    size_t slot;
    if (!free_slots.empty()) {
      slot = free_slots.back();
      free_slots.pop_back();
      slots[slot] = {};
    } else {
      slot = slots.size();
      slots.emplace_back();
    }
    index[id] = slot;
  }

  void free(EntityId id) {
    auto it = index.find(id);
    if (it == index.end())
      return;
    free_slots.push_back(it->second);
    index.erase(it);
  }

  EntityUniformBuffer *get(EntityId id) {
    auto it = index.find(id);
    return it != index.end() ? &slots[it->second] : nullptr;
  }
};
} // namespace zephyr
