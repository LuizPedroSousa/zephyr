#pragma once
#include "platforms/vulkan/surface.hpp"
#include "window.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <unistd.h>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace zephyr {

struct VulkanPhysicalDevice;
class VulkanLogicalDevice;
class VulkanRenderPass;

struct VulkanSwapChainSupport {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> preset_modes;
};

class VulkanSwapChain {
public:
  VulkanSwapChain() = default;

  VulkanSwapChain(VkSwapchainKHR handle, VkSurfaceFormatKHR format,
                  VkPresentModeKHR preset_mode, VkExtent2D extent,
                  VkDevice logical_device_handle, std::vector<VkImage> images,
                  std::vector<VkImageView> image_views,
                  std::vector<VkFramebuffer> framebuffers,
                  std::vector<VkSwapchainKHR> retired_chain_handles)
      : handle(handle), surface_format(format), preset_mode(preset_mode),
        extent(extent), logical_device_handle(logical_device_handle),
        images(std::move(images)), image_views(std::move(image_views)),
        framebuffers(std::move(framebuffers)),
        retired_chain_handles(std::move(retired_chain_handles)) {}

  VkSwapchainKHR handle;

  VkSurfaceFormatKHR surface_format;
  VkPresentModeKHR preset_mode;
  VkExtent2D extent;
  VkDevice logical_device_handle;

  std::vector<VkImage> images;
  std::vector<VkImageView> image_views;
  std::vector<VkFramebuffer> framebuffers;

  std::vector<VkSwapchainKHR> retired_chain_handles;

  bool is_handle_valid() { return handle != VK_NULL_HANDLE; }

  static VulkanSwapChainSupport find_support(VkSurfaceKHR surface,
                                             VkPhysicalDevice device) {
    VulkanSwapChainSupport support;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                              &support.capabilities);

    uint32_t present_mode_count;

    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
                                              &present_mode_count, nullptr);

    if (present_mode_count != 0) {
      support.preset_modes.resize(present_mode_count);

      vkGetPhysicalDeviceSurfacePresentModesKHR(
          device, surface, &present_mode_count, support.preset_modes.data());
    }

    uint32_t format_count;

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count,
                                         nullptr);

    if (format_count != 0) {
      support.formats.resize(format_count);

      vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count,
                                           support.formats.data());
    }

    return support;
  };

  static VulkanSwapChain
  create(const Window window, VulkanPhysicalDevice physical_device,
         VulkanLogicalDevice logical_device, const VulkanSurface surface,
         std::optional<VulkanSwapChain> existent_chain = std::nullopt);

  static void create_image_views(VkDevice logical_device_handle,
                                 VulkanSwapChain &swap_chain);

  static void create_framebuffers(VulkanLogicalDevice logical_device,
                                  VulkanSwapChain &swap_chain,
                                  VulkanRenderPass render_pass);

  void cleanup();
};

struct VulkanSwapChainPicker {
  static VkSurfaceFormatKHR choose_surface_format(
      const std::vector<VkSurfaceFormatKHR> &available_formats) {

    for (const auto available_format : available_formats) {
      if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
          available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        return available_format;
      }
    }

    return available_formats[0];
  }

  static VkPresentModeKHR choose_present_mode(
      const std::vector<VkPresentModeKHR> &available_present_modes) {
    for (const auto available_preset_mode : available_present_modes) {
      if (available_preset_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
        return available_preset_mode;
      }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
  }

  static VkExtent2D
  choose_extent(GLFWwindow *window,
                const VkSurfaceCapabilitiesKHR &capabilities) {

    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
    }

    int width, height;

    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D extent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
    };

    extent.width = std::clamp(extent.width, capabilities.minImageExtent.width,
                              capabilities.maxImageExtent.width);
    extent.height =
        std::clamp(extent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return extent;
  }
};

} // namespace zephyr
