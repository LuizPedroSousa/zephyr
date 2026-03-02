#pragma once

#include "memory"
#include <atomic>
#include <bitset>
#include <cstdint>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/ext/scalar_uint_sized.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#define BACKEND_VULKAN 1
#define BACKEND_OPENGL 2

namespace zephyr {

class Guid {
public:
  Guid() : m_value(next_id()) {}
  Guid(uint64_t value) : m_value(value) {}
  Guid(const Guid &) = default;

  operator uint64_t() const { return m_value; }
  operator std::string() const { return std::to_string(m_value); }

private:
  static uint64_t next_id() {
    static std::atomic<uint64_t> counter{1};
    return counter++;
  }

  uint64_t m_value;
};

const constexpr uint32_t MAX_COMPONENTS = 32;

using ListenerId = Guid;
using SchedulerID = Guid;
using VertexIndice = uint32_t;
using EntityId = uint32_t;
using ComponentTypeId = uint32_t;
using ArchetypeSignature = std::bitset<MAX_COMPONENTS>;

#define ZEPH_BIND_EVENT_FN(fn)                                                 \
  [this](auto &&...args) -> decltype(auto) {                                   \
    return this->fn(std::forward<decltype(args)>(args)...);                    \
  }

template <typename T> using Scope = std::unique_ptr<T>;
template <typename T, typename... Args>
constexpr Scope<T> create_scope(Args &&...args) {
  return std::make_unique<T>(std::forward<Args>(args)...);
};

template <typename T> using Ref = std::shared_ptr<T>;
template <typename T, typename... Args>
constexpr Ref<T> create_ref(Args &&...args) {
  return std::make_shared<T>(std::forward<Args>(args)...);
};

} // namespace zephyr

namespace std {
template <typename T> struct hash;

template <> struct hash<zephyr::Guid> {
  std::size_t operator()(const zephyr::Guid &guid) const {
    return (uint64_t)guid;
  }
};
}; // namespace std
