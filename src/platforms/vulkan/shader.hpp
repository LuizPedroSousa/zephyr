#pragma once

#include "platforms/vulkan/device.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace zephyr {

struct Shader {
  static VkShaderModule create_module(const VulkanLogicalDevice device,
                                      const std::vector<char> &byte_code) {
    VkShaderModuleCreateInfo create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = byte_code.size();
    create_info.pCode = reinterpret_cast<const uint32_t *>(byte_code.data());

    VkShaderModule shader_module;

    ZEPH_ENSURE(vkCreateShaderModule(device.handle, &create_info, nullptr,
                                     &shader_module) != VK_SUCCESS,
                "Couldn't create shader");

    return shader_module;
  }
};

} // namespace zephyr
