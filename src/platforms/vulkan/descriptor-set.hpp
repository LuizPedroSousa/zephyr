#pragma once
#include "platforms/vulkan/buffer.hpp"
#include "platforms/vulkan/device.hpp"
namespace zephyr {

class VulkanDescriptorPool {
public:
  VulkanDescriptorPool(VkDescriptorPool handle) : handle(handle) {};

  static VulkanDescriptorPool create(uint32_t size,
                                     VulkanLogicalDevice logical_device) {
    VkDescriptorPoolSize pool_size{};

    VkDescriptorPool handle;

    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = static_cast<uint32_t>(size);

    VkDescriptorPoolCreateInfo create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.poolSizeCount = 1;
    create_info.pPoolSizes = &pool_size;

    create_info.maxSets = static_cast<uint32_t>(size);

    ZEPH_ENSURE(vkCreateDescriptorPool(logical_device.handle, &create_info,
                                       nullptr, &handle),
                "Couldn't create descriptor pool");

    return VulkanDescriptorPool(handle);
  }

  VkDescriptorPool handle;
};

class VulkanDescriptorSetLayout {
public:
  VulkanDescriptorSetLayout() = default;
  VulkanDescriptorSetLayout(VkDescriptorSetLayout handle, VkDevice ld_handle)
      : m_handle(handle), m_ld_handle(ld_handle) {}

  static VulkanDescriptorSetLayout create(uint32_t binding, uint32_t size,
                                          VulkanLogicalDevice logical_device) {
    VkDescriptorSetLayoutBinding uniform_buffer_layout_binding{};

    uniform_buffer_layout_binding.binding = binding;
    uniform_buffer_layout_binding.descriptorType =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniform_buffer_layout_binding.descriptorCount = size;
    uniform_buffer_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uniform_buffer_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount = 1;
    create_info.pBindings = &uniform_buffer_layout_binding;

    VkDescriptorSetLayout desc_handle;

    ZEPH_ENSURE(vkCreateDescriptorSetLayout(logical_device.handle, &create_info,
                                            nullptr, &desc_handle) != VK_SUCCESS,
                "Coudln't create descriptor set layout");

    return VulkanDescriptorSetLayout(desc_handle, logical_device.handle);
  }

  void cleanup() {
    vkDestroyDescriptorSetLayout(m_ld_handle, m_handle, nullptr);
  }

  inline constexpr VkDescriptorSetLayout &handle() noexcept {
    return m_handle;
  };

private:
  VkDescriptorSetLayout m_handle;
  VkDevice m_ld_handle;
};

class VulkanDescriptorSet {
public:
  explicit VulkanDescriptorSet(std::vector<VkDescriptorSet> m_descriptor_sets)
      : m_descriptor_sets(std::move(m_descriptor_sets)) {}

  const std::vector<VkDescriptorSet> &handles() const {
    return m_descriptor_sets;
  }

  template <typename T>
  static VulkanDescriptorSet
  create(VulkanLogicalDevice logical_device,
         VulkanDescriptorSetLayout descriptor_set_layout,
         VulkanDescriptorPool descriptor_pool,
         std::vector<VulkanBuffer::TransientStagingRegion> uniform_buffers) {
    std::vector<VkDescriptorSet> descriptor_sets;
    descriptor_sets.resize(uniform_buffers.size());

    std::vector<VkDescriptorSetLayout> layouts(uniform_buffers.size(),
                                               descriptor_set_layout.handle());

    VkDescriptorSetAllocateInfo allocate_info{};

    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = descriptor_pool.handle;
    allocate_info.descriptorSetCount =
        static_cast<uint32_t>(uniform_buffers.size());
    allocate_info.pSetLayouts = layouts.data();

    ZEPH_ENSURE(vkAllocateDescriptorSets(logical_device.handle, &allocate_info,
                                         descriptor_sets.data()) != VK_SUCCESS,
                "Couldn't allocate descriptor sets");

    for (size_t i = 0; i < uniform_buffers.size(); i++) {
      VkDescriptorBufferInfo buffer_info{};

      buffer_info.buffer = uniform_buffers[i].buffer;
      buffer_info.offset = 0;
      buffer_info.range = sizeof(T);

      VkWriteDescriptorSet descriptor_write{};
      descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptor_write.dstSet = descriptor_sets[i];
      descriptor_write.dstBinding = 0;
      descriptor_write.dstArrayElement = 0;
      descriptor_write.pBufferInfo = &buffer_info;
      descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptor_write.descriptorCount = 1;

      vkUpdateDescriptorSets(logical_device.handle, 1, &descriptor_write, 0,
                             nullptr);
    }

    return VulkanDescriptorSet(descriptor_sets);
  }

private:
  std::vector<VkDescriptorSet> m_descriptor_sets;
};

} // namespace zephyr
