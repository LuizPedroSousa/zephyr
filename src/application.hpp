#pragma once

#include "assert.hpp"
#include "base.hpp"
#include "entity.hpp"
#include "mesh.hpp"
#include "platforms/vulkan/render-target.hpp"
#include "window.hpp"
#include <cstdint>
#include <cstring>
#include <vulkan/vulkan_core.h>

namespace zephyr {

class Application {
public:
  Application() : m_window(Window()) {}

  void init() {
    m_window.open("Zephyr", 1920, 1080);

    m_vulkan_render_target = create_scope<VulkanRenderTarget>();
    m_vulkan_render_target->init(m_window);
    m_vulkan_render_target->create_vertex_buffer(m_mesh.vertices);
    m_vulkan_render_target->create_index_buffer(m_mesh.indices);
  }

  void run() {
    while (m_window.is_open()) {
      m_window.update();
      draw();
    }

    vkDeviceWaitIdle(m_vulkan_render_target->logical_device().handle);
  }

  void draw() {
    auto logical_device = m_vulkan_render_target->logical_device();
    auto swap_chain = m_vulkan_render_target->swap_chain();

    auto image_available_semaphores =
        m_vulkan_render_target->image_available_semaphores();
    auto render_finished_semaphores =
        m_vulkan_render_target->render_finished_semaphores();
    auto in_flight_fences = m_vulkan_render_target->in_flight_fences();
    auto command_buffers = m_vulkan_render_target->command_buffers();

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
      m_vulkan_render_target->recreate_swap_chain(m_window);
      return;
    } else {
      ZEPH_ENSURE(result != VK_SUCCESS, "Couldn't acquire swap chain image");
    }

    vkResetFences(logical_device.handle, 1, &in_flight_fences[m_current_frame]);

    vkResetCommandBuffer(command_buffers[m_current_frame], 0);
    update_uniform_buffer(swap_chain);

    m_vulkan_render_target->push_command_buffer(
        command_buffers[m_current_frame], m_mesh, image_index, m_current_frame);

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

    m_current_frame =
        (m_current_frame + 1) % m_vulkan_render_target->max_frames_in_flight();
  }

  void update_uniform_buffer(VulkanSwapChain swap_chain) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime)
                     .count();

    EntityUniformBuffer uniform;

    uniform.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
                                glm::vec3(0.0, 0.0, 1.0f));

    uniform.view = glm::lookAt(glm::vec3(2.0f), glm::vec3(0.0f),
                               glm::vec3(0.0f, 0.0f, 1.0f));

    uniform.projection = glm::perspective(
        glm::radians(45.0f),
        swap_chain.extent.width / (float)swap_chain.extent.height, 0.1f, 10.0f);

#if BACKEND == BACKEND_VULKAN
    uniform.projection[1][1] *= -1;
#endif

    auto uniform_buffers = m_vulkan_render_target->uniform_buffers();

    memcpy(uniform_buffers[m_current_frame].mapped, &uniform, sizeof(uniform));
  }

  ~Application() {
    m_vulkan_render_target->cleanup();
    m_window.cleanup();
  }

private:
  Window m_window;
  Scope<VulkanRenderTarget> m_vulkan_render_target;
  uint32_t m_current_frame = 0;

  Mesh m_mesh = Mesh::cube();
};

} // namespace zephyr
