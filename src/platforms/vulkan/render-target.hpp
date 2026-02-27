#pragma once

#include "entity.hpp"
#include "file.hpp"
#include "mesh.hpp"
#include "platforms/vulkan/buffer.hpp"
#include "platforms/vulkan/command-buffer.hpp"
#include "platforms/vulkan/descriptor-set.hpp"
#include "platforms/vulkan/device.hpp"
#include "platforms/vulkan/fence.hpp"
#include "platforms/vulkan/graphics-pipeline.hpp"
#include "platforms/vulkan/image.hpp"
#include "platforms/vulkan/instance.hpp"
#include "platforms/vulkan/render-pass.hpp"
#include "platforms/vulkan/semaphore.hpp"
#include "platforms/vulkan/surface.hpp"
#include "platforms/vulkan/swap-chain.hpp"
#include "window.hpp"
#include <regex>

#ifdef ENABLE_VALIDATION_LAYER
#include "platforms/vulkan/validation-layer.hpp"
#endif
#include <alloca.h>
#include <bits/types/cookie_io_functions_t.h>
#include <cstddef>
#include <fstream>
#include <glm/vector_relational.hpp>
#include <set>
#include <unistd.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <assert.hpp>
#include <base.hpp>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <log.hpp>
#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#include <stb_image/stb_image.h>

namespace zephyr {

class VulkanRenderTarget {
public:
  VulkanRenderTarget() = default;
  ~VulkanRenderTarget() = default;

  void init(Window window) {
    m_instance = VulkanInstance::create();
    m_surface = VulkanSurface::create(m_instance, window);
    m_physical_device = VulkanPhysicalDevicePicker::pick(m_instance, m_surface);
    m_logical_device = VulkanLogicalDevice::create(m_physical_device);

    m_swap_chain = VulkanSwapChain::create(window, m_physical_device,
                                           m_logical_device, m_surface);

    VulkanSwapChain::create_image_views(m_logical_device.handle, m_swap_chain);

    m_render_pass = VulkanRenderPass::create(m_swap_chain, m_logical_device);

    m_descriptor_set_layout =
        VulkanDescriptorSetLayout::create(0, 1, m_logical_device);

    m_graphics_pipeline = VulkanGraphicsPipeline::create(
        m_logical_device, m_swap_chain, m_descriptor_set_layout, m_render_pass);

    VulkanSwapChain::create_framebuffers(m_logical_device, m_swap_chain,
                                         m_render_pass);

    create_command_pool();
    create_uniform_buffers<EntityUniformBuffer>();

    m_descriptor_pool =
        VulkanDescriptorPool::create(m_uniform_buffers.size(), m_logical_device)
            .handle;

    m_descriptor_sets = VulkanDescriptorSet::create<EntityUniformBuffer>(
                            m_logical_device, m_descriptor_set_layout,
                            m_descriptor_pool, m_uniform_buffers)
                            .handles();

    create_command_buffers();
    create_sync_objects();
  }

  void cleanup_semaphores() {
    for (auto semaphore : m_render_finished_semaphores) {
      vkDestroySemaphore(m_logical_device.handle, semaphore, nullptr);
    }

    for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroySemaphore(m_logical_device.handle,
                         m_image_available_semaphores[i], nullptr);
    }
  }

  void recreate_swap_chain(Window window) {
    vkDeviceWaitIdle(m_logical_device.handle);

    for (auto framebuffer : m_swap_chain.framebuffers) {
      vkDestroyFramebuffer(m_logical_device.handle, framebuffer, nullptr);
    }

    for (auto image_view : m_swap_chain.image_views) {
      vkDestroyImageView(m_logical_device.handle, image_view, nullptr);
    }

    m_swap_chain.framebuffers.clear();
    m_swap_chain.image_views.clear();

    cleanup_semaphores();

    m_swap_chain = VulkanSwapChain::create(
        window, m_physical_device, m_logical_device, m_surface, m_swap_chain);

    VulkanSwapChain::create_framebuffers(m_logical_device, m_swap_chain,
                                         m_render_pass);

    create_semaphores();

    constexpr size_t MAX_RETIRED = 3;
    while (m_swap_chain.retired_chain_handles.size() > MAX_RETIRED) {
      VkSwapchainKHR oldest = m_swap_chain.retired_chain_handles.front();
      vkDestroySwapchainKHR(m_logical_device.handle, oldest, nullptr);
      m_swap_chain.retired_chain_handles.erase(
          m_swap_chain.retired_chain_handles.begin());
    }
  }

  void create_command_pool() {
    VkCommandPoolCreateInfo create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex =
        m_physical_device.queue_family_indices.graphics_family.value();

    ZEPH_ENSURE(vkCreateCommandPool(m_logical_device.handle, &create_info,
                                    nullptr, &m_command_pool) != VK_SUCCESS,
                "Couldn't create command pool");
  }

  void create_vertex_buffer(std::vector<Vertex> vertices) {
    VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

    auto staging_buffer = VulkanBuffer::TransientStagingRegion::make(
        m_logical_device, buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    staging_buffer.allocate(m_physical_device,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    staging_buffer.upload(vertices.data());

    staging_buffer.unmap();

    m_vertex_region = VulkanBuffer::DeviceLocalRegion::make(
        m_logical_device, buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    m_vertex_region.allocate(m_physical_device,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    m_vertex_region.copy_from(&staging_buffer, m_logical_device.graphics_queue,
                              m_command_pool);

    staging_buffer.cleanup();
  }

  void create_index_buffer(std::vector<VertexIndice> indices) {
    VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();

    auto staging_buffer = VulkanBuffer::TransientStagingRegion::make(
        m_logical_device, buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    staging_buffer.allocate(m_physical_device,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    staging_buffer.upload(indices.data());
    staging_buffer.unmap();

    m_index_region = VulkanBuffer::DeviceLocalRegion::make(
        m_logical_device, buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    m_index_region.allocate(m_physical_device,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    m_index_region.copy_from(&staging_buffer, m_logical_device.graphics_queue,
                             m_command_pool);

    staging_buffer.cleanup();
  }

  template <typename T> void create_uniform_buffers() {
    VkDeviceSize buffer_size = sizeof(T);

    m_uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      auto staging_buffer = VulkanBuffer::TransientStagingRegion::make(
          m_logical_device, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

      staging_buffer.allocate(m_physical_device,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

      staging_buffer.map();

      m_uniform_buffers[i] = staging_buffer;
    }
  }

  void create_command_buffers() {
    m_command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo alloc_info =
        VulkanCommandBuffer::declare_allocate(m_command_buffers.size(),
                                              m_command_pool);

    ZEPH_ENSURE(vkAllocateCommandBuffers(m_logical_device.handle, &alloc_info,
                                         m_command_buffers.data()) !=
                    VK_SUCCESS,
                "Couldn't allocate command buffer");
  }

  void push_command_buffer(VkCommandBuffer command_buffer, Mesh mesh,
                           uint32_t image_index, uint32_t frame_index) {
    VkCommandBufferBeginInfo begin_info = VulkanCommandBuffer::declare_begin(0);

    // begin_info.pInheritanceInfo = nullptr;

    ZEPH_ENSURE(vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS,
                "Couldn't begin to push command buffer");

    VkRenderPassBeginInfo render_pass_info = VulkanRenderPass::declare_begin(
        m_render_pass.handle, m_swap_chain.framebuffers[image_index],
        m_swap_chain.extent);

    vkCmdBeginRenderPass(command_buffer, &render_pass_info,
                         VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      m_graphics_pipeline.handle());

    VkBuffer vertex_buffers[] = {m_vertex_region.buffer};
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

    vkCmdBindIndexBuffer(command_buffer, m_index_region.buffer, 0,
                         VK_INDEX_TYPE_UINT32);

    VkViewport viewport{};

    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_swap_chain.extent.width;
    viewport.height = (float)m_swap_chain.extent.height;

    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swap_chain.extent;

    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_graphics_pipeline.layout(), 0, 1,
                            &m_descriptor_sets[frame_index], 0, nullptr);

    vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(mesh.indices.size()),
                     1, 0, 0, 0);

    vkCmdEndRenderPass(command_buffer);

    ZEPH_ENSURE(vkEndCommandBuffer(m_command_buffers[frame_index]) !=
                    VK_SUCCESS,
                "Couldn't create shader");
  }

  void create_semaphores() {
    m_render_finished_semaphores.resize(m_swap_chain.image_views.size());
    m_image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphore_info = VulkanSemaphore::declare();

    for (size_t i = 0; i < m_swap_chain.image_views.size(); i++) {
      ZEPH_ENSURE(
          vkCreateSemaphore(m_logical_device.handle, &semaphore_info, nullptr,
                            &m_render_finished_semaphores[i]) != VK_SUCCESS,
          "Couldn't create render finished semaphore");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      ZEPH_ENSURE(
          vkCreateSemaphore(m_logical_device.handle, &semaphore_info, nullptr,
                            &m_image_available_semaphores[i]) != VK_SUCCESS,
          "Couldn't create image available semaphore");
    }
  }

  void create_sync_objects() {
    m_render_finished_semaphores.resize(m_swap_chain.image_views.size());
    m_in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    create_semaphores();

    VkFenceCreateInfo fence_info = VulkanFence::declare();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      ZEPH_ENSURE(vkCreateFence(m_logical_device.handle, &fence_info, nullptr,
                                &m_in_flight_fences[i]) != VK_SUCCESS,
                  "Couldn't in_flight fence");
    }
  }

  void cleanup() {
    m_swap_chain.cleanup();

    m_vertex_region.cleanup();
    m_index_region.cleanup();

    vkDestroyDescriptorPool(m_logical_device.handle, m_descriptor_pool,
                            nullptr);

    m_descriptor_set_layout.cleanup();

    for (auto uniform_buffer : m_uniform_buffers) {
      uniform_buffer.unmap();
      uniform_buffer.cleanup();
    }

    m_graphics_pipeline.cleanup();

    cleanup_semaphores();

    for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroyFence(m_logical_device.handle, m_in_flight_fences[i], nullptr);
    }

    vkDestroyCommandPool(m_logical_device.handle, m_command_pool, nullptr);

    vkDestroyDevice(m_logical_device.handle, nullptr);

    m_surface.cleanup();
    m_instance.cleanup();
  };

  VulkanLogicalDevice logical_device() const { return m_logical_device; }
  VulkanSwapChain swap_chain() const { return m_swap_chain; }
  std::vector<VkFence> in_flight_fences() { return m_in_flight_fences; }

  std::vector<VkCommandBuffer> command_buffers() { return m_command_buffers; }

  std::vector<VkSemaphore> image_available_semaphores() {
    return m_image_available_semaphores;
  }

  std::vector<VkSemaphore> render_finished_semaphores() {
    return m_render_finished_semaphores;
  }

  uint32_t constexpr max_frames_in_flight() const noexcept {
    return MAX_FRAMES_IN_FLIGHT;
  }

  inline std::vector<VulkanBuffer::TransientStagingRegion> constexpr &
  uniform_buffers() {
    return m_uniform_buffers;
  }

private:
  VulkanInstance m_instance;
  VulkanPhysicalDevice m_physical_device;
  VulkanLogicalDevice m_logical_device;
  VulkanGraphicsPipeline m_graphics_pipeline;
  VulkanRenderPass m_render_pass;
  VulkanSurface m_surface;
  VulkanDescriptorSetLayout m_descriptor_set_layout;
  VulkanSwapChain m_swap_chain;

  VkCommandPool m_command_pool;

  std::vector<VkCommandBuffer> m_command_buffers;

  std::vector<VkSemaphore> m_image_available_semaphores;
  std::vector<VkSemaphore> m_render_finished_semaphores;
  std::vector<VkFence> m_in_flight_fences;

  const uint8_t MAX_FRAMES_IN_FLIGHT = 2;

  VulkanBuffer::DeviceLocalRegion m_vertex_region;
  VulkanBuffer::DeviceLocalRegion m_index_region;

  std::vector<VulkanBuffer::TransientStagingRegion> m_uniform_buffers;

  VkDescriptorPool m_descriptor_pool;

  std::vector<VkDescriptorSet> m_descriptor_sets;

  VkImage m_texture_image;

  VkDeviceMemory m_texture_image_memory;
};

} // namespace zephyr
