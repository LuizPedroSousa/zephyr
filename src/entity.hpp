#pragma once
#include "base.hpp"
#include "components.hpp"
#include "log.hpp"
#include "mesh.hpp"
#include "platforms/vulkan/swap-chain.hpp"
#include "time.hpp"
#include "uniforms.hpp"
#include "window.hpp"
#include <unordered_map>
#include <vulkan/vulkan_core.h>

namespace std {
template <> struct hash<std::bitset<zephyr::MAX_COMPONENTS>> {
  size_t operator()(const std::bitset<zephyr::MAX_COMPONENTS> &sig) const {
    return hash<unsigned long>()(sig.to_ulong());
  }
};
} // namespace std

namespace zephyr {

namespace internal {
inline ComponentTypeId next_component_id() {
  static ComponentTypeId counter = 0;
  return counter++;
}
} // namespace internal

template <typename T> inline ComponentTypeId component_type_id() {
  static ComponentTypeId id = internal::next_component_id();
  return id;
}

struct Column {
  size_t stride = 0;
  std::vector<std::byte> data;
  size_t count() const { return stride ? data.size() / stride : 0; }
};

struct ArchetypeStorage {
  ArchetypeSignature signature;
  std::vector<EntityId> entity_ids;
  std::unordered_map<ComponentTypeId, Column> columns;
};

struct EntityRecord {
  ArchetypeStorage *archetype = nullptr;
  size_t row = 0;
};

struct World {
  std::unordered_map<ArchetypeSignature, ArchetypeStorage> archetypes;
  std::unordered_map<EntityId, EntityRecord> entity_records;
  UniformTable uniforms;
  EntityId m_next_id = 0;

  EntityId spawn() {
    EntityId id = m_next_id++;

    entity_records[id] = {nullptr, 0};
    uniforms.allocate(id);

    return id;
  }

  void despawn(EntityId id) {
    auto it = entity_records.find(id);
    if (it == entity_records.end())
      return;
    auto &[arch, row] = it->second;
    if (arch)
      swap_remove(*arch, id, row);
    uniforms.free(id);
    entity_records.erase(id);
  }

  template <typename T> void add_component(EntityId id, T component) {
    ComponentTypeId tid = component_type_id<T>();
    auto &record = entity_records[id];
    ArchetypeStorage *old_arch = record.archetype;
    size_t old_row = record.row;

    ArchetypeSignature new_sig =
        old_arch ? old_arch->signature : ArchetypeSignature{};
    new_sig.set(tid);

    ArchetypeStorage &new_arch = archetypes[new_sig];
    new_arch.signature = new_sig;
    size_t new_row = new_arch.entity_ids.size();

    if (old_arch) {
      for (auto &[ct, old_col] : old_arch->columns) {
        Column &new_col = new_arch.columns[ct];
        new_col.stride = old_col.stride;
        new_col.data.insert(
            new_col.data.end(), old_col.data.begin() + old_row * old_col.stride,
            old_col.data.begin() + (old_row + 1) * old_col.stride);
      }
      swap_remove(*old_arch, id, old_row);
    }

    Column &col = new_arch.columns[tid];
    col.stride = sizeof(T);

    const auto *bytes = reinterpret_cast<const std::byte *>(&component);
    col.data.insert(col.data.end(), bytes, bytes + sizeof(T));

    new_arch.entity_ids.push_back(id);
    entity_records[id] = {&new_arch, new_row};
  }

  template <typename... Ts> void add_components(EntityId id, Ts... components) {
    auto &record = entity_records[id];
    ArchetypeStorage *old_arch = record.archetype;
    size_t old_row = record.row;

    ArchetypeSignature new_sig =
        old_arch ? old_arch->signature : ArchetypeSignature{};
    (new_sig.set(component_type_id<Ts>()), ...);

    ArchetypeStorage &new_arch = archetypes[new_sig];
    new_arch.signature = new_sig;
    size_t new_row = new_arch.entity_ids.size();

    if (old_arch) {
      for (auto &[ct, old_col] : old_arch->columns) {
        Column &new_col = new_arch.columns[ct];
        new_col.stride = old_col.stride;
        new_col.data.insert(
            new_col.data.end(), old_col.data.begin() + old_row * old_col.stride,
            old_col.data.begin() + (old_row + 1) * old_col.stride);
      }
      swap_remove(*old_arch, id, old_row);
    }

    (
        [&](auto component) {
          using T = decltype(component);
          Column &col = new_arch.columns[component_type_id<T>()];
          col.stride = sizeof(T);
          const auto *bytes = reinterpret_cast<const std::byte *>(&component);
          col.data.insert(col.data.end(), bytes, bytes + sizeof(T));
        }(components),
        ...);

    new_arch.entity_ids.push_back(id);
    entity_records[id] = {&new_arch, new_row};
  }

  template <typename T> T *get_component(EntityId id) {
    auto it = entity_records.find(id);
    if (it == entity_records.end() || !it->second.archetype)
      return nullptr;
    auto &[arch, row] = it->second;
    auto col_it = arch->columns.find(component_type_id<T>());
    if (col_it == arch->columns.end())
      return nullptr;
    return reinterpret_cast<T *>(col_it->second.data.data() + row * sizeof(T));
  }

  void place(EntityId id, glm::vec3 position = glm::vec3(0.0f)) {
    auto *existent_transform = get_component<TransformComponent>(id);

    if (existent_transform) {
      existent_transform->translate(position);
    } else {
      TransformComponent transform{};
      transform.translate(position);
      add_component(id, transform);
    }
  }

  void scale(EntityId id, glm::vec3 s) {
    auto *existent_transform = get_component<TransformComponent>(id);
    if (existent_transform) {
      existent_transform->set_scale(s);
    } else {
      TransformComponent transform{};
      transform.set_scale(s);
      add_component(id, transform);
    }
  }

  void rotate(EntityId id, glm::quat rotation) {
    auto *existent_transform = get_component<TransformComponent>(id);
    if (existent_transform) {
      existent_transform->rotation = rotation;
      existent_transform->is_dirty = true;
    } else {
      TransformComponent transform{};
      transform.rotation = rotation;
      transform.is_dirty = true;
      add_component(id, transform);
    }
  }

  void update_uniforms() {
    CameraComponent *camera_component = nullptr;
    TransformComponent *camera_transform = nullptr;

    query<TransformComponent, CameraComponent>(
        [&](EntityId, TransformComponent &transform, CameraComponent &camera) {
          camera_component = &camera;
          camera_transform = &transform;
        });

    if (!camera_component && !camera_transform)
      return;

    query<TransformComponent>([&](EntityId id, TransformComponent &transform) {
      if (auto *uniform = uniforms.get(id))
        uniform->update(transform, *camera_component, *camera_transform);
    });
  }

  template <typename... Ts, typename Fn> void query(Fn &&fn) {
    ArchetypeSignature required;
    (required.set(component_type_id<Ts>()), ...);

    for (auto &[sig, arch] : archetypes) {
      if ((sig & required) != required || arch.entity_ids.empty())
        continue;
      size_t count = arch.entity_ids.size();
      for (size_t i = 0; i < count; ++i) {
        fn(arch.entity_ids[i],
           *reinterpret_cast<Ts *>(
               arch.columns[component_type_id<Ts>()].data.data() +
               i * sizeof(Ts))...);
      }
    }
  }

private:
  void swap_remove(ArchetypeStorage &arch, EntityId id, size_t row) {
    size_t last_row = arch.entity_ids.size() - 1;
    EntityId last_id = arch.entity_ids[last_row];

    for (auto &[ct, col] : arch.columns) {
      if (row != last_row) {
        std::copy(col.data.begin() + last_row * col.stride,
                  col.data.begin() + last_row * col.stride + col.stride,
                  col.data.begin() + row * col.stride);
      }
      col.data.resize(col.data.size() - col.stride);
    }

    arch.entity_ids[row] = last_id;
    arch.entity_ids.pop_back();

    if (last_id != id)
      entity_records[last_id].row = row;
  }
};

template <typename... Ts> struct EntityBuilder {
  World &m_world;
  glm::vec3 m_position = glm::vec3(0.0f);
  glm::vec3 m_scale = glm::vec3(1.0f);
  glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

  std::tuple<Ts...> m_components;

  EntityBuilder(World &world) : m_world(world) {}

  EntityBuilder(World &world, glm::vec3 position, glm::vec3 scale,
                glm::quat rotation, std::tuple<Ts...> components)
      : m_world(world), m_position(position), m_scale(scale),
        m_rotation(rotation), m_components(components) {}

  EntityBuilder &with_position(glm::vec3 position) {
    m_position = position;
    return *this;
  }

  EntityBuilder &with_scale(glm::vec3 scale) {
    m_scale = scale;
    return *this;
  }

  EntityBuilder &with_rotation(glm::quat rotation) {
    m_rotation = rotation;
    return *this;
  }

  template <typename T> EntityBuilder<Ts..., T> with_component(T component) {
    return EntityBuilder<Ts..., T>(
        m_world, m_position, m_scale, m_rotation,
        std::tuple_cat(m_components, std::make_tuple(component)));
  }

  EntityId spawn() {
    EntityId id = m_world.spawn();

    TransformComponent transform{};
    transform.translate(m_position);
    transform.set_scale(m_scale);
    transform.rotation = m_rotation;
    transform.is_dirty = true;
    transform.recalculate();

    constexpr bool has_tag = (std::is_same_v<Ts, ObjectTagComponent> || ...) ||
                             (std::is_same_v<Ts, CameraTagComponent> || ...);

    std::apply(
        [&](auto... components) {
          if constexpr (has_tag) {
            m_world.add_components(id, transform, components...);
          } else {
            m_world.add_components(id, transform, components...,
                                   ObjectTagComponent{});
          }
        },
        m_components);

    return id;
  }
};

inline EntityBuilder<> make_entity(World &world) {
  return EntityBuilder<>(world);
}

} // namespace zephyr
