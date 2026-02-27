#pragma once

#include "assert.hpp"
#include "log.hpp"
#include <cstring>
#include <vulkan/vulkan_core.h>

namespace zephyr {

class ValidationLayer {
public:
  ValidationLayer() = default;

  explicit ValidationLayer(VkDebugUtilsMessengerEXT debug_messenger,
                           VkInstance instance)
      : m_debug_messenger(debug_messenger), m_instance(instance) {}

  static ValidationLayer create(VkInstance instance) {
    VkDebugUtilsMessengerEXT debug_messenger;

    ZEPH_ENSURE(instance == nullptr,
                "Instance must be created before debug_messenger");

    VkDebugUtilsMessengerCreateInfoEXT create_info =
        ValidationLayer::declare_debug_messenger();

    ZEPH_ENSURE(ValidationLayer::create_debug_utils_messenger_ext(
                    instance, &create_info, nullptr, &debug_messenger) !=
                    VK_SUCCESS,
                "Couldn't create debug messenger");

    return ValidationLayer(debug_messenger, instance);
  }

  static VkResult create_debug_utils_messenger_ext(
      VkInstance instance,
      const VkDebugUtilsMessengerCreateInfoEXT *p_create_info,
      const VkAllocationCallbacks *p_allocator,
      VkDebugUtilsMessengerEXT *p_debug_messenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)(vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT"));

    if (func != nullptr) {
      return func(instance, p_create_info, p_allocator, p_debug_messenger);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  static void
  destroy_debug_utils_messenger_ext(VkInstance instance,
                                    VkDebugUtilsMessengerEXT debug_messenger,
                                    const VkAllocationCallbacks *p_allocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)(vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT"));

    if (func != nullptr) {
      func(instance, debug_messenger, p_allocator);
    }
  }

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                 VkDebugUtilsMessageTypeFlagsEXT message_type,
                 const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
                 void *p_user_data) {
    LOG_DEBUG("Validation layer: ", p_callback_data->pMessage);

    return VK_FALSE;
  };

  static VkDebugUtilsMessengerCreateInfoEXT declare_debug_messenger() {
    VkDebugUtilsMessengerCreateInfoEXT create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    create_info.pfnUserCallback = debug_callback;
    create_info.pUserData = nullptr;

    return create_info;
  };

  static bool
  ensure_validation_layers_support(const std::vector<const char *> &layers) {
    uint32_t available_layer_count;

    vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr);

    std::vector<VkLayerProperties> available_layers(available_layer_count);

    vkEnumerateInstanceLayerProperties(&available_layer_count,
                                       available_layers.data());

    for (auto layer : layers) {
      bool layer_exists = false;

      for (auto available_layer : available_layers) {
        if (strcmp(layer, available_layer.layerName) == 0) {
          layer_exists = true;
          break;
        }
      }

      if (!layer_exists) {
        return false;
      }
    }

    return true;
  }

  void cleanup() {
    ValidationLayer::destroy_debug_utils_messenger_ext(
        m_instance, m_debug_messenger, nullptr);
  }

private:
  VkInstance m_instance;
  VkDebugUtilsMessengerEXT m_debug_messenger;
};

} // namespace zephyr
