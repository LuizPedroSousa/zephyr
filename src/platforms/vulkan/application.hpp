#pragma once

#include <vulkan/vulkan_core.h>

static constexpr const char *APP_NAME = "zephyr";
static constexpr const char *ENGINE_NAME = "zephyr";

struct VulkanApplication {
  static VkApplicationInfo declare() {
    VkApplicationInfo app_info{};

    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = APP_NAME;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = ENGINE_NAME;
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;
    app_info.pNext = nullptr;

    return app_info;
  }
};
