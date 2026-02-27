#pragma once

#include "assert.hpp"
#include "platforms/vulkan/instance.hpp"
#include "window.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

namespace zephyr {

class VulkanSurface {
public:
  VulkanSurface() = default;
  VulkanSurface(VkInstance instance, VkSurfaceKHR surface)
      : m_instance(instance), m_handle(surface) {}

  static VulkanSurface create(const VulkanInstance &instance, Window window) {
    VkSurfaceKHR handle;

    ZEPH_ENSURE(glfwCreateWindowSurface(instance.handle(), window.handle(),
                                        nullptr, &handle) != VK_SUCCESS,
                "Coudln't create instance");

    return VulkanSurface(instance.handle(), handle);
  }

  void cleanup() { vkDestroySurfaceKHR(m_instance, m_handle, nullptr); }

  inline constexpr VkSurfaceKHR handle() const { return m_handle; }

private:
  VkInstance m_instance;
  VkSurfaceKHR m_handle;
};

} // namespace zephyr
