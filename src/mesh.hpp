#pragma once
#include "base.hpp"
#include "log.hpp"
#include <array>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace zephyr {

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texture_coordinates;

  static VkVertexInputBindingDescription get_binding_description() {
    VkVertexInputBindingDescription binding_description{};

    binding_description.binding = 0;
    binding_description.stride = sizeof(Vertex);
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return binding_description;
  }

  static std::array<VkVertexInputAttributeDescription, 3>
  get_attribute_descriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions{};

    attribute_descriptions[0].binding = 0;
    attribute_descriptions[0].location = 0;
    attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[0].offset = offsetof(Vertex, position);

    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[1].offset = offsetof(Vertex, normal);

    attribute_descriptions[2].binding = 0;
    attribute_descriptions[2].location = 2;
    attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions[2].offset = offsetof(Vertex, texture_coordinates);

    return attribute_descriptions;
  }
};

class Mesh {
public:
  std::vector<Vertex> vertices;
  std::vector<VertexIndice> indices;

  Mesh(std::vector<Vertex> p_vertices, std::vector<VertexIndice> p_indices)
      : indices(p_indices) {

#if BACKEND == BACKEND_VULKAN
    for (auto &v : vertices)
      v.position.y = -v.position.y;
#endif

    vertices = p_vertices;
  }

  static Mesh capsule(float radius = 0.5f, float height = 1.0f,
                      int segments = 16, int rings = 8);

  static Mesh cube(float size = 1.0f) {
    float half_length = size / 2.0f;

    std::vector<Vertex> vertices = {
        // Front face
        {glm::vec3(-half_length, -half_length, half_length),
         glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)},
        {glm::vec3(half_length, -half_length, half_length),
         glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f)},
        {glm::vec3(half_length, half_length, half_length),
         glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)},
        {glm::vec3(-half_length, half_length, half_length),
         glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)},

        // Back face
        {glm::vec3(half_length, -half_length, -half_length),
         glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 0.0f)},
        {glm::vec3(-half_length, -half_length, -half_length),
         glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 0.0f)},
        {glm::vec3(-half_length, half_length, -half_length),
         glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 1.0f)},
        {glm::vec3(half_length, half_length, -half_length),
         glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 1.0f)},

        // Left face
        {glm::vec3(-half_length, -half_length, -half_length),
         glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
        {glm::vec3(-half_length, -half_length, half_length),
         glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)},
        {glm::vec3(-half_length, half_length, half_length),
         glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f)},
        {glm::vec3(-half_length, half_length, -half_length),
         glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f)},

        // Right face
        {glm::vec3(half_length, -half_length, half_length),
         glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
        {glm::vec3(half_length, -half_length, -half_length),
         glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)},
        {glm::vec3(half_length, half_length, -half_length),
         glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f)},
        {glm::vec3(half_length, half_length, half_length),
         glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f)},

        // Top face
        {glm::vec3(-half_length, half_length, half_length),
         glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
        {glm::vec3(half_length, half_length, half_length),
         glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f)},
        {glm::vec3(half_length, half_length, -half_length),
         glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f)},
        {glm::vec3(-half_length, half_length, -half_length),
         glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f)},

        // Bottom face
        {glm::vec3(-half_length, -half_length, -half_length),
         glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
        {glm::vec3(half_length, -half_length, -half_length),
         glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 0.0f)},
        {glm::vec3(half_length, -half_length, half_length),
         glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 1.0f)},
        {glm::vec3(-half_length, -half_length, half_length),
         glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 1.0f)}};

    std::vector<unsigned int> indices = {// Front face
                                         0, 1, 2, 2, 3, 0,

                                         // Back face
                                         4, 5, 6, 6, 7, 4,

                                         // Left face
                                         8, 9, 10, 10, 11, 8,

                                         // Right face
                                         12, 13, 14, 14, 15, 12,

                                         // Top face
                                         16, 17, 18, 18, 19, 16,

                                         // Bottom face
                                         20, 21, 22, 22, 23, 20};

    return Mesh(vertices, indices);
  }

  static Mesh plane(float size) {
    float half_length = size / 2.0f;

    std::vector<Vertex> vertices = {
        // Front face
        {glm::vec3(-half_length, -half_length, half_length),
         glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)},
        {glm::vec3(half_length, -half_length, half_length),
         glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f)},
        {glm::vec3(half_length, half_length, half_length),
         glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)},
        {glm::vec3(-half_length, half_length, half_length),
         glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)}};

    std::vector<unsigned int> indices = {
        // Front face
        0, 1, 2, 2, 3, 0,
    };

    return Mesh(vertices, indices);
  }

  static Mesh sphere() {
    const int segments = 32;
    const int rings = 16;
    const float radius = 0.5f;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float pi = std::numbers::pi_v<float>;

    for (int ring = 0; ring <= rings; ++ring) {
      float phi = ring * pi / rings;
      float y = radius * std::cos(phi);

      for (int segment = 0; segment <= segments; ++segment) {
        float theta = segment * 2 * pi / segments;
        float x = radius * std::sin(phi) * std::cos(theta);
        float z = radius * std::sin(phi) * std::sin(theta);

        Vertex vertex;
        vertex.position = glm::vec3(x, y, z);
        vertex.normal = glm::normalize(vertex.position);
        vertex.texture_coordinates =
            glm::vec2(static_cast<float>(segment) / segments,
                      static_cast<float>(ring) / rings);

        vertices.push_back(vertex);
      }
    }

    for (int ring = 0; ring < rings; ++ring) {
      for (int segment = 0; segment < segments; ++segment) {
        int current_ring = ring * (segments + 1);
        int next_ring = (ring + 1) * (segments + 1);

        indices.push_back(current_ring + segment);
        indices.push_back(next_ring + segment);
        indices.push_back(next_ring + segment + 1);

        indices.push_back(current_ring + segment);
        indices.push_back(next_ring + segment + 1);
        indices.push_back(current_ring + segment + 1);
      }
    }

    return Mesh(vertices, indices);
  }

  static Mesh quad(float size = 1.0f) {
    float half_length = size / 2.0f;

    std::vector<Vertex> vertices = {
        {glm::vec3(-half_length, half_length, 0.0f), glm::vec3(0.0f),
         glm::vec2(0.0f, 1.0f)},
        {glm::vec3(-half_length, -half_length, 0.0f), glm::vec3(0.0f),
         glm::vec2(0.0f, 0.0f)},
        {glm::vec3(half_length, -half_length, 0.0f), glm::vec3(0.0f),
         glm::vec2(1.0f, 0.0f)},
        {glm::vec3(half_length, half_length, 0.0f), glm::vec3(0.0f),
         glm::vec2(1.0f, 1.0f)},
    };

    std::vector<unsigned int> indices = {0, 1, 2, 2,
                                         3, 0

    };

    return Mesh(vertices, indices);
  }

  ~Mesh() = default;
};
} // namespace zephyr
