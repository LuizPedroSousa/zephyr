#pragma once

#include "assert.hpp"
#include "platforms/vulkan/instance.hpp"
#include "platforms/vulkan/queue.hpp"
#include "platforms/vulkan/surface.hpp"
#include "platforms/vulkan/swap-chain.hpp"
#include <map>
#include <set>

namespace zephyr {

struct VulkanPhysicalDevice {
  VulkanQueueFamilyIndices queue_family_indices;

  VkPhysicalDevice handle;
  VkPhysicalDeviceFeatures available_features;
  VkPhysicalDeviceProperties available_properties;

  VulkanSwapChainSupport swap_chain_support;

  std::vector<const char *> extensions;

  VulkanPhysicalDevice() = default;

  VulkanPhysicalDevice(VulkanQueueFamilyIndices queue_family_indices,
                       VkPhysicalDevice handle,
                       VkPhysicalDeviceFeatures available_features,
                       VkPhysicalDeviceProperties available_properties,
                       VulkanSwapChainSupport swap_chain_support,
                       std::vector<const char *> extensions)
      : queue_family_indices(queue_family_indices), handle(handle),
        available_features(available_features),
        available_properties(available_properties),
        swap_chain_support(swap_chain_support),
        extensions(std::move(extensions)) {}

  bool is_handle_valid() { return handle != VK_NULL_HANDLE; }

  static uint32_t find_memory_type(VkPhysicalDevice device_handle,
                                   uint32_t type_filter,
                                   VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memory_properties;

    vkGetPhysicalDeviceMemoryProperties(device_handle, &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
      auto mem_type_property_flags =
          memory_properties.memoryTypes[i].propertyFlags;

      auto has_flags = (mem_type_property_flags & properties) == properties;

      if (type_filter & (1 << i) && has_flags) {
        return i;
      }
    }

    ZEPH_EXCEPTION("Couldn't find suitable memory type");
  }
};

struct VulkanPhysicalDevicePicker {
  static VulkanPhysicalDevice pick(const VulkanInstance &instance,
                                   const VulkanSurface &surface) {
    VulkanPhysicalDevice physical_device;
    uint32_t device_count = 0;

    vkEnumeratePhysicalDevices(instance.handle(), &device_count, nullptr);

    ZEPH_ENSURE(device_count == 0,
                "Couldn't find any GPU devices with Vulkan Support!!")

    std::vector<VkPhysicalDevice> devices(device_count);

    vkEnumeratePhysicalDevices(instance.handle(), &device_count,
                               devices.data());

    std::multimap<int, VulkanPhysicalDevice> candidates;

    std::vector<const char *> required_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    for (auto device : devices) {
      VkPhysicalDeviceProperties device_properties;
      VkPhysicalDeviceFeatures device_features;

      vkGetPhysicalDeviceProperties(device, &device_properties);
      vkGetPhysicalDeviceFeatures(device, &device_features);

      VulkanQueueFamilyIndices indices =
          VulkanQueueFamilyIndices::find_queue_families(
              surface.handle(), device, VK_QUEUE_GRAPHICS_BIT);

      VulkanSwapChainSupport swap_chain_support =
          VulkanSwapChain::find_support(surface.handle(), device);

      if (VulkanPhysicalDevicePicker::is_suitable_candidate(
              device, indices, swap_chain_support, device_features,
              required_extensions)) {
        candidates.insert(std::make_pair(
            rate_suitable_device(device_properties),
            VulkanPhysicalDevice(indices, device, device_features,
                                 device_properties, swap_chain_support,
                                 required_extensions)));
      }
    }

    if (!candidates.empty()) {
      const auto &best = *candidates.rbegin();
      if (best.first > 0) {
        physical_device = best.second;
      }
    }

    ZEPH_ENSURE(!physical_device.is_handle_valid(),
                "Couldn't find any suitable GPU device");

    return physical_device;
  }

  static int
  rate_suitable_device(VkPhysicalDeviceProperties device_properties) {
    const uint32_t HIGH_SCORE_ATTRIBUTION = 1000;

    int score = 0;

    score +=
        device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
            ? HIGH_SCORE_ATTRIBUTION
            : 0;

    score += device_properties.limits.maxImageDimension2D;

    return score;
  }

  static bool ensure_device_extensions_support(
      VkPhysicalDevice device, std::vector<const char *> required_extensions) {
    uint32_t available_extension_count;

    vkEnumerateDeviceExtensionProperties(device, nullptr,
                                         &available_extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(
        available_extension_count);

    vkEnumerateDeviceExtensionProperties(device, nullptr,
                                         &available_extension_count,
                                         available_extensions.data());

    std::set<std::string> extensions_set(required_extensions.begin(),
                                         required_extensions.end());

    for (const auto &available_extension : available_extensions) {
      extensions_set.erase(available_extension.extensionName);
    }

    return extensions_set.empty();
  }

  static bool
  is_suitable_candidate(VkPhysicalDevice device,
                        VulkanQueueFamilyIndices queue_family_indices,
                        VulkanSwapChainSupport swap_chain_support,
                        VkPhysicalDeviceFeatures device_features,
                        std::vector<const char *> required_extensions) {

    bool is_extensions_supported =
        ensure_device_extensions_support(device, required_extensions);

    if (!is_extensions_supported) {
      return false;
    }

    bool is_swap_chain_valid = !swap_chain_support.formats.empty() &&
                               !swap_chain_support.preset_modes.empty();

    return queue_family_indices.exists() && device_features.geometryShader &&
           is_swap_chain_valid;
  }
};

class VulkanLogicalDevice {
public:
  VulkanQueueFamilyIndices indices;
  VkDevice handle;
  VkPhysicalDevice phy_handle;

  VkQueue graphics_queue;
  VkQueue present_queue;

  bool is_handle_valid() { return handle != VK_NULL_HANDLE; }

  static VulkanLogicalDevice create(const VulkanPhysicalDevice &physical_device);
};

}; // namespace zephyr
