#pragma once

#include "assert.hpp"
#include "vulkan.hpp"
#include "window.hpp"
#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

namespace zephyr {

class Application {
public:
  Application() : m_window(Window()), m_vulkan(Vulkan()) {}

  void init() {
    m_window.open("Zephyr", 1920, 1080);

    m_vulkan.create_instance();
    m_vulkan.create_debug_messenger();
    m_vulkan.create_surface(m_window.handle());
    m_vulkan.pick_physical_device();
    m_vulkan.create_logical_device();
    m_vulkan.create_swap_chain(m_window.handle());
    m_vulkan.create_image_views();
    m_vulkan.create_render_pass();
    m_vulkan.create_graphics_pipeline();
    m_vulkan.create_framebuffers();
    m_vulkan.create_command_pool();
    m_vulkan.create_vertex_buffer(m_vertices);
    m_vulkan.create_command_buffers();
    m_vulkan.create_sync_objects();
  }

  void run() {
    while (m_window.is_open()) {
      m_window.update();
      draw();
    }

    vkDeviceWaitIdle(m_vulkan.logical_device().handle);
  }

  void draw() {
    auto logical_device = m_vulkan.logical_device();
    auto swap_chain = m_vulkan.swap_chain();

    auto image_available_semaphores = m_vulkan.image_available_semaphores();
    auto render_finished_semaphores = m_vulkan.render_finished_semaphores();
    auto in_flight_fences = m_vulkan.in_flight_fences();
    auto command_buffers = m_vulkan.command_buffers();

    vkWaitForFences(logical_device.handle, 1,
                    &in_flight_fences[m_current_frame], VK_TRUE, UINT64_MAX);

    uint32_t image_index;

    VkResult result = vkAcquireNextImageKHR(
        logical_device.handle, swap_chain.handle, UINT64_MAX,
        image_available_semaphores[m_current_frame], VK_NULL_HANDLE,
        &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        m_window.is_resized()) {
      m_window.set_is_resized(false);
      m_vulkan.recreate_swap_chain(m_window.handle());
      return;
    } else {
      ZEPH_ENSURE(result != VK_SUCCESS, "Couldn't acquire swap chain image");
    }

    vkResetFences(logical_device.handle, 1, &in_flight_fences[m_current_frame]);

    vkResetCommandBuffer(command_buffers[m_current_frame], 0);
    m_vulkan.push_command_buffer(command_buffers[m_current_frame], m_vertices,
                                 image_index, m_current_frame);

    VkSubmitInfo submit_info{};

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {
        image_available_semaphores[m_current_frame]};
    VkSemaphore signal_semaphores[] = {render_finished_semaphores[image_index]};

    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[m_current_frame];

    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    ZEPH_ENSURE(vkQueueSubmit(logical_device.graphics_queue, 1, &submit_info,
                              in_flight_fences[m_current_frame]) != VK_SUCCESS,
                "Couldn't submit draw command buffer");

    VkSwapchainKHR swap_chains[] = {swap_chain.handle};

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr;

    vkQueuePresentKHR(logical_device.present_queue, &present_info);

    m_current_frame = (m_current_frame + 1) % m_vulkan.max_frames_in_flight();
  }

  ~Application() {
    m_vulkan.cleanup();
    m_window.cleanup();
  }

private:
  Window m_window;
  Vulkan m_vulkan;
  uint32_t m_current_frame = 0;

  const std::vector<Vertex> m_vertices = {{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                          {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                                          {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};
};

} // namespace zephyr
