#pragma once
#include "assert.hpp"
#include "base.hpp"
#include "platforms/vulkan/validation-layer.hpp"
#include <cstring>
#define GLFW_INCLUDE_VULKAN
#include "platforms/vulkan/application.hpp"
#include <GLFW/glfw3.h>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace zephyr {

class VulkanInstance {
public:
  VulkanInstance() = default;

#ifdef ENABLE_VALIDATION_LAYER
  VulkanInstance(VkInstance handle, ValidationLayer validation_layer)
      : m_handle(handle), m_validation_layer(validation_layer) {}
#else
  VulkanInstance(VkInstance handle) : m_handle(handle) {}
#endif

  static bool ensure_extensions_support(std::vector<const char *> extensions) {
    uint32_t available_extension_count = 0;

    vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count,
                                           nullptr);

    std::vector<VkExtensionProperties> available_extensions(
        available_extension_count);

    vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count,
                                           available_extensions.data());

    for (auto extension : extensions) {
      bool extension_exists = false;

      for (auto available_extension : available_extensions) {
        if (strcmp(extension, available_extension.extensionName) == 0) {
          extension_exists = true;
          break;
        }
      }

      if (!extension_exists) {
        return false;
      }
    }

    return true;
  }

  static VulkanInstance create() {
    VkInstanceCreateInfo instance_info{};

    VkApplicationInfo app_info = VulkanApplication::declare();

    auto extensions = get_required_extensions();

    ZEPH_ENSURE(!ensure_extensions_support(extensions),
                " Required extensions not available");

#ifdef ENABLE_VALIDATION_LAYER
    const std::vector<const char *> validation_layers = {
        "VK_LAYER_KHRONOS_validation"};

    ZEPH_ENSURE(
        !ValidationLayer::ensure_validation_layers_support(validation_layers),
        "Validation Layer not available");
#endif

    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionCount =
        static_cast<uint32_t>(extensions.size());
    instance_info.ppEnabledExtensionNames = extensions.data();

#ifdef ENABLE_VALIDATION_LAYER
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info =
        ValidationLayer::declare_debug_messenger();
    instance_info.pNext = &debug_create_info;
    instance_info.enabledLayerCount = validation_layers.size();
    instance_info.ppEnabledLayerNames = validation_layers.data();
#endif

#if defined(__APPLE__)
    instance_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    VkInstance handle;

    int result =
        vkCreateInstance(&instance_info, nullptr, &handle) != VK_SUCCESS;

    ZEPH_ENSURE(result, "Coudln't create instance");

#ifdef ENABLE_VALIDATION_LAYER
    auto validation_layer = ValidationLayer::create(handle);

    return VulkanInstance(handle, validation_layer);
#else
    return VulkanInstance(handle);
#endif
  };

  static std::vector<const char *> get_required_extensions() {
    uint32_t glfw_extension_count = 0;

    const char **glfw_extensions =
        glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    ZEPH_ENSURE(glfw_extensions == nullptr,
                "Couldn't get GLFW required extensions");

    std::vector<const char *> required_extensions(
        glfw_extensions, glfw_extensions + glfw_extension_count);

#if defined(__APPLE__)
    required_extensions.emplace_back(
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

#ifdef ENABLE_VALIDATION_LAYER
    required_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return required_extensions;
  }

  void cleanup() {
#ifdef ENABLE_VALIDATION_LAYER
    m_validation_layer.cleanup();
#endif
    vkDestroyInstance(m_handle, nullptr);
  }

  inline constexpr VkInstance handle() const { return m_handle; }

private:
  VkInstance m_handle;

#ifdef ENABLE_VALIDATION_LAYER
  ValidationLayer m_validation_layer;
#endif
};

} // namespace zephyr
