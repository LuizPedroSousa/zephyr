#pragma once

#include "assert.hpp"
#include "base.hpp" #include "components.hpp"
#include "entity.hpp"
#include "event-dispatcher.hpp"
#include "event-scheduler.hpp" #include "keyboard.hpp"
#include "log.hpp"
#include "mesh.hpp"
#include "platforms/vulkan/queue.hpp"
#include "platforms/vulkan/render-target.hpp"
#include "time.hpp"
#include "window.hpp"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <cstring>
#include <glm/ext/vector_float3.hpp>
#include <glm/trigonometric.hpp>
#include <vulkan/vulkan_core.h>

namespace zephyr {

class Application {
public:
  Application() {}

  void init() {
    EventScheduler::init();
    EventDispatcher::init();
    Time::init();
    Window::init();

    auto window = Window::get();

    window->open("Zephyr", 1920, 1080);

    m_vulkan_render_target = create_scope<VulkanRenderTarget>();
    m_vulkan_render_target->init(window);
  }

  void run() {
    auto scheduler = EventScheduler::get();

    Time *time = Time::get();
    Window *window = Window::get();

    make_entity(m_world)
        .with_position({0.0f, 0.0f, -3.0f})
        .with_component(CameraComponent{})
        .with_component(CameraTagComponent{})
        .spawn();

    make_entity(m_world)
        .with_component(MeshComponent{.mesh = Mesh::cube()})
        .spawn();

    for (size_t x = 0; x < 4; x++) {
      for (size_t z = 0; z < 4; z++) {
        make_entity(m_world)
            .with_component(MeshComponent{.mesh = Mesh::cube()})
            .with_position(glm::vec3(x * 2, 0, z * 2))
            .spawn();
      }
    }

    m_vulkan_render_target->setup_uniform_buffer_slots<EntityUniformBuffer>(
        m_world.uniforms.slots.size());

    m_vulkan_render_target->create_descriptor_pool();

    m_vulkan_render_target->create_texture_image(
        "../src/assets/textures/stone_albedo.jpg");
    m_vulkan_render_target->setup_descriptor_sets<EntityUniformBuffer>();

    auto cube = Mesh::cube();

    m_vulkan_render_target->create_vertex_buffer(cube.vertices);
    m_vulkan_render_target->create_index_buffer(cube.indices);

    m_vulkan_render_target->setup_grid_pipeline();

    m_vulkan_render_target->create_command_buffers();
    m_vulkan_render_target->create_sync_objects();

    while (window->is_open()) {
      time->update();
      window->update();
      scheduler->bind(SchedulerType::POST_FRAME);

      draw();

      window->swap();
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
    auto available_command_buffers = m_vulkan_render_target->command_buffers();
    Window *window = Window::get();

    vkWaitForFences(logical_device.handle, 1,
                    &in_flight_fences[m_current_frame], VK_TRUE, UINT64_MAX);

    uint32_t image_index;

    VkResult result = vkAcquireNextImageKHR(
        logical_device.handle, swap_chain.handle, UINT64_MAX,
        image_available_semaphores[m_current_frame], VK_NULL_HANDLE,
        &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        window->is_resized()) {
      window->set_is_resized(false);
      m_vulkan_render_target->recreate_swap_chain(window);
      return;
    } else {
      ZEPH_ENSURE(result != VK_SUCCESS, "Couldn't acquire swap chain image");
    }

    m_world.query<TransformComponent, CameraComponent, CameraTagComponent>(
        [&](EntityId id, TransformComponent &transform_component,
            CameraComponent &camera_component, CameraTagComponent &_tag) {
          auto extent = m_vulkan_render_target->swap_chain().extent;
          camera_component.movement(transform_component, extent);
        });

    static Timer timer;
    float time = timer.elapsed();

    m_world.query<TransformComponent, ObjectTagComponent>(
        [&](EntityId id, TransformComponent &transform_component,
            ObjectTagComponent &_tag) {
          transform_component.rotation = glm::angleAxis(
              time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

          transform_component.is_dirty = true;
          transform_component.recalculate();
        });

    vkResetFences(logical_device.handle, 1, &in_flight_fences[m_current_frame]);

    std::vector<VkCommandBuffer> frame_command_buffers = {
        available_command_buffers[m_current_frame]};

    vkResetCommandBuffer(frame_command_buffers[0], 0);

    m_world.update_uniforms();

    uint32_t camera_slot = 0;
    m_world.query<TransformComponent, CameraComponent, CameraTagComponent>(
        [&](EntityId id, TransformComponent &, CameraComponent &,
            CameraTagComponent &) {
          if (auto *uniform = m_world.uniforms.get(id)) {
            camera_slot = m_world.uniforms.index.at(id);
            m_vulkan_render_target->dispatch_uniform_buffer(
                *uniform, camera_slot, m_current_frame);
          }
        });

    m_world.query<MeshComponent, ObjectTagComponent>(
        [&](EntityId id, MeshComponent &, ObjectTagComponent &) {
          if (auto *uniform = m_world.uniforms.get(id)) {
            uint32_t slot = m_world.uniforms.index.at(id);
            m_vulkan_render_target->dispatch_uniform_buffer(*uniform, slot,
                                                            m_current_frame);
          }
        });

    m_vulkan_render_target->begin_frame(frame_command_buffers[0], image_index);

    m_vulkan_render_target->draw(frame_command_buffers[0], m_current_frame,
                                 camera_slot);

    m_world.query<MeshComponent, ObjectTagComponent>(
        [&](EntityId id, MeshComponent &mesh_component,
            ObjectTagComponent &_tag) {
          uint32_t slot = m_world.uniforms.index.at(id);
          m_vulkan_render_target->draw_indexed(frame_command_buffers[0],
                                               mesh_component.mesh,
                                               m_current_frame, slot);
        });

    m_vulkan_render_target->end_frame(frame_command_buffers[0]);

    VkSemaphore wait_semaphores[] = {
        image_available_semaphores[m_current_frame]};
    VkSemaphore signal_semaphores[] = {render_finished_semaphores[image_index]};

    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit_info =
        VulkanQueue::declare_submit(frame_command_buffers);

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
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

  ~Application() {
    m_vulkan_render_target->cleanup();
    Window::get()->cleanup();
  }

private:
  Scope<VulkanRenderTarget> m_vulkan_render_target;

  uint32_t m_current_frame = 0;
  World m_world;
};

} // namespace zephyr
