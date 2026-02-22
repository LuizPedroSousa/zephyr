#include <vulkan/vulkan_core.h>

namespace zephyr {

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

} // namespace zephyr
