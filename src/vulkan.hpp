#pragma once

#include "exception.hpp"
#include "mesh.hpp"
#include <bits/types/cookie_io_functions_t.h>
#include <cstddef>
#include <fstream>
#include <limits>
#include <memory>
#include <set>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <algorithm>
#include <assert.hpp>
#include <base.hpp>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <iterator>
#include <log.hpp>
#include <map>
#include <vector>
#include <vulkan-proxy.hpp>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

namespace zephyr {

static std::vector<char> read_file(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  ZEPH_ENSURE(!file.is_open(), "Couldn't open file");

  size_t file_size = (size_t)file.tellg();

  std::vector<char> buffer(file_size);

  file.seekg(0);
  file.read(buffer.data(), file_size);

  file.close();

  return buffer;
}

struct QueueFamilyIndices {
  std::optional<uint32_t> graphics_family;
  std::optional<uint32_t> present_family;

  bool exists() {
    return graphics_family.has_value() && present_family.has_value();
  }
};

struct SwapChainSupport {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> preset_modes;
};

struct PhysicalDevice {
  QueueFamilyIndices queue_family_indices;
  VkPhysicalDevice handle;
  VkPhysicalDeviceFeatures available_features;
  VkPhysicalDeviceProperties available_properties;
  SwapChainSupport swap_chain_support;

  std::vector<const char *> extensions;

  bool is_handle_valid() { return handle != VK_NULL_HANDLE; }
};

struct LogicalDevice {
  QueueFamilyIndices indices;
  VkDevice handle;
  VkPhysicalDevice phy_handle;

  VkQueue graphics_queue;
  VkQueue present_queue;

  bool is_handle_valid() { return handle != VK_NULL_HANDLE; }
};

struct SwapChain {
  VkSwapchainKHR handle;
  VkSurfaceFormatKHR format;
  VkPresentModeKHR preset_mode;
  VkExtent2D extent;

  std::vector<VkImage> images;
  std::vector<VkImageView> image_views;
  std::vector<VkFramebuffer> framebuffers;

  std::vector<VkSwapchainKHR> retired_chain_handles;
};

class Vulkan {
public:
  Vulkan() = default;

  void create_instance() {
    VkInstanceCreateInfo instance_info{};

    VkApplicationInfo app_info = declare_application();

    auto extensions = get_required_extensions();

    ZEPH_ENSURE(!ensure_extensions_support(extensions),
                " Required extensions not available");

#ifdef ENABLE_VALIDATION_LAYER
    const std::vector<const char *> validation_layers = {
        "VK_LAYER_KHRONOS_validation"};

    ZEPH_ENSURE(!ensure_validation_layers_support(validation_layers),
                "Validation Layer not available");
#endif

    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionCount =
        static_cast<uint32_t>(extensions.size());
    instance_info.ppEnabledExtensionNames = extensions.data();

#ifdef ENABLE_VALIDATION_LAYER
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info =
        declare_debug_messenger();
    instance_info.pNext = &debug_create_info;
    instance_info.enabledLayerCount = validation_layers.size();
    instance_info.ppEnabledLayerNames = validation_layers.data();
#endif

#if defined(__APPLE__)
    instance_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    int result =
        vkCreateInstance(&instance_info, nullptr, &m_instance) != VK_SUCCESS;

    ZEPH_ENSURE(result, "Coudln't create instance");
  };

  void create_surface(GLFWwindow *window) {
    ZEPH_ENSURE(glfwCreateWindowSurface(m_instance, window, nullptr,
                                        &m_surface) != VK_SUCCESS,
                "Coudln't create instance");
  }

  void pick_physical_device() {
    uint32_t device_count = 0;

    vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);

    ZEPH_ENSURE(device_count == 0,
                "Couldn't find any GPU devices with Vulkan Support!!")

    std::vector<VkPhysicalDevice> devices(device_count);

    vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

    std::multimap<int, PhysicalDevice> candidates;

    std::vector<const char *> required_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    for (auto device : devices) {
      VkPhysicalDeviceProperties device_properties;
      VkPhysicalDeviceFeatures device_features;

      vkGetPhysicalDeviceProperties(device, &device_properties);
      vkGetPhysicalDeviceFeatures(device, &device_features);

      QueueFamilyIndices indices =
          find_queue_families(device, VK_QUEUE_GRAPHICS_BIT);

      SwapChainSupport swap_chain_support = find_swap_chain_support(device);

      if (is_device_suitable_candidate(device, indices, swap_chain_support,
                                       device_features, required_extensions)) {
        PhysicalDevice physical_device;

        physical_device.queue_family_indices = indices;
        physical_device.handle = device;
        physical_device.available_features = device_features;
        physical_device.available_properties = device_properties;
        physical_device.extensions = required_extensions;
        physical_device.swap_chain_support = swap_chain_support;

        candidates.insert(std::make_pair(
            rate_suitable_device(device_properties), physical_device));
      }
    }

    if (!candidates.empty()) {
      const auto &best = *candidates.rbegin();
      if (best.first > 0) {
        m_physical_device = best.second;
      }
    }

    ZEPH_ENSURE(!m_physical_device.is_handle_valid(),
                "Couldn't find any suitable GPU device");
  }

  void create_logical_device() {

#ifdef ENABLE_VALIDATION_LAYER
    const std::vector<const char *> validation_layers = {
        "VK_LAYER_KHRONOS_validation"};
    ZEPH_ENSURE(!ensure_validation_layers_support(validation_layers),
                "Validation Layer not available");
#endif

    std::set<uint32_t> unique_queue_families = {
        m_physical_device.queue_family_indices.graphics_family.value(),
        m_physical_device.queue_family_indices.present_family.value(),
    };

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;

    float queue_priority = 1.0f;

    for (uint32_t queue_family_index : unique_queue_families) {
      VkDeviceQueueCreateInfo queue_create_info{};

      queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_create_info.queueFamilyIndex = queue_family_index;
      queue_create_info.queueCount = 1;
      queue_create_info.pQueuePriorities = &queue_priority;

      queue_create_infos.emplace_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features{};

    VkDeviceCreateInfo create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.queueCreateInfoCount =
        static_cast<uint32_t>(queue_create_infos.size());

    create_info.pEnabledFeatures = &device_features;

    create_info.enabledExtensionCount = m_physical_device.extensions.size();
    create_info.ppEnabledExtensionNames = m_physical_device.extensions.data();

#ifdef ENABLE_VALIDATION_LAYER
    create_info.enabledLayerCount = validation_layers.size();
    create_info.ppEnabledLayerNames = validation_layers.data();
#endif

    VkDevice logical_device_handle;

    ZEPH_ENSURE(vkCreateDevice(m_physical_device.handle, &create_info, nullptr,
                               &logical_device_handle) != VK_SUCCESS,
                "Couldn't create logical device")

    m_logical_device = LogicalDevice{};

    m_logical_device.handle = logical_device_handle;
    m_logical_device.phy_handle = m_physical_device.handle;

    vkGetDeviceQueue(
        m_logical_device.handle,
        m_physical_device.queue_family_indices.graphics_family.value(), 0,
        &m_logical_device.graphics_queue);

    ZEPH_ENSURE(m_logical_device.graphics_queue == VK_NULL_HANDLE,
                "Couldn't create queue for logical device");

    vkGetDeviceQueue(
        m_logical_device.handle,
        m_physical_device.queue_family_indices.present_family.value(), 0,
        &m_logical_device.present_queue);

    ZEPH_ENSURE(m_logical_device.present_queue == VK_NULL_HANDLE,
                "Couldn't create queue for logical device");
  }

  void create_swap_chain(GLFWwindow *window) {
    auto surface_format = choose_swap_surface_format(
        m_physical_device.swap_chain_support.formats);

    auto present_mode = choose_swap_present_mode(
        m_physical_device.swap_chain_support.preset_modes);

    auto extent = choose_swap_extent(
        window, m_physical_device.swap_chain_support.capabilities);

    uint32_t image_count =
        m_physical_device.swap_chain_support.capabilities.minImageCount + 1;

    if (m_physical_device.swap_chain_support.capabilities.maxImageCount > 0 &&
        image_count >
            m_physical_device.swap_chain_support.capabilities.maxImageCount) {

      image_count =
          m_physical_device.swap_chain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = m_surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto indices = m_physical_device.queue_family_indices;

    if (indices.graphics_family != indices.present_family) {
      create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      create_info.queueFamilyIndexCount = 2;

      uint32_t queue_family_indices[] = {
          indices.graphics_family.value(),
          indices.present_family.value(),
      };

      create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
      create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      create_info.queueFamilyIndexCount = 0;
      create_info.pQueueFamilyIndices = nullptr;
    }

    create_info.preTransform =
        m_physical_device.swap_chain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;

    if (m_swap_chain.handle != VK_NULL_HANDLE) {
      create_info.oldSwapchain = m_swap_chain.handle;
      m_swap_chain.retired_chain_handles.push_back(m_swap_chain.handle);
    } else {
      create_info.oldSwapchain = VK_NULL_HANDLE;
    }

    ZEPH_ENSURE(vkCreateSwapchainKHR(m_logical_device.handle, &create_info,
                                     nullptr,
                                     &m_swap_chain.handle) != VK_SUCCESS,
                "Couldn't create swap chain");

    m_swap_chain.format = surface_format;
    m_swap_chain.preset_mode = present_mode;
    m_swap_chain.extent = extent;

    uint32_t swap_chain_images_count = 0;

    vkGetSwapchainImagesKHR(m_logical_device.handle, m_swap_chain.handle,
                            &swap_chain_images_count, nullptr);

    m_swap_chain.images.resize(swap_chain_images_count);

    vkGetSwapchainImagesKHR(m_logical_device.handle, m_swap_chain.handle,
                            &swap_chain_images_count,
                            m_swap_chain.images.data());
  }

  void cleanup_swap_chain(SwapChain swap_chain,
                          bool cleanup_retired_chains = false) {
    for (auto framebuffer : swap_chain.framebuffers) {
      vkDestroyFramebuffer(m_logical_device.handle, framebuffer, nullptr);
    }

    for (auto image_view : swap_chain.image_views) {
      vkDestroyImageView(m_logical_device.handle, image_view, nullptr);
    }

    vkDestroySwapchainKHR(m_logical_device.handle, m_swap_chain.handle,
                          nullptr);

    if (cleanup_retired_chains) {
      for (auto retired_chain : swap_chain.retired_chain_handles) {
        vkDestroySwapchainKHR(m_logical_device.handle, retired_chain, nullptr);
      }

      m_swap_chain.retired_chain_handles.clear();
    }
  }

  void cleanup_semaphores() {
    for (auto semaphore : m_render_finished_semaphores) {
      vkDestroySemaphore(m_logical_device.handle, semaphore, nullptr);
    }

    for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroySemaphore(m_logical_device.handle,
                         m_image_available_semaphores[i], nullptr);
    }
  }

  void recreate_swap_chain(GLFWwindow *window) {
    vkDeviceWaitIdle(m_logical_device.handle);

    for (auto framebuffer : m_swap_chain.framebuffers) {
      vkDestroyFramebuffer(m_logical_device.handle, framebuffer, nullptr);
    }

    for (auto image_view : m_swap_chain.image_views) {
      vkDestroyImageView(m_logical_device.handle, image_view, nullptr);
    }

    m_swap_chain.framebuffers.clear();
    m_swap_chain.image_views.clear();

    cleanup_semaphores();
    create_swap_chain(window);
    create_image_views();
    create_framebuffers();
    create_semaphores();

    constexpr size_t MAX_RETIRED = 3;
    while (m_swap_chain.retired_chain_handles.size() > MAX_RETIRED) {
      VkSwapchainKHR oldest = m_swap_chain.retired_chain_handles.front();
      vkDestroySwapchainKHR(m_logical_device.handle, oldest, nullptr);
      m_swap_chain.retired_chain_handles.erase(
          m_swap_chain.retired_chain_handles.begin());
    }
  }

  void create_image_views() {
    m_swap_chain.image_views.resize(m_swap_chain.images.size());

    for (size_t i = 0; i < m_swap_chain.images.size(); i++) {
      VkImageViewCreateInfo create_info{};

      create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      create_info.image = m_swap_chain.images[i];
      create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      create_info.format = m_swap_chain.format.format;

      create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

      create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      create_info.subresourceRange.baseMipLevel = 0;
      create_info.subresourceRange.levelCount = 1;
      create_info.subresourceRange.baseArrayLayer = 0;
      create_info.subresourceRange.layerCount = 1;

      ZEPH_ENSURE(vkCreateImageView(m_logical_device.handle, &create_info,
                                    nullptr,
                                    &m_swap_chain.image_views[i]) != VK_SUCCESS,
                  "Couldn't create image view");
    };
  }

  void create_render_pass() {
    VkAttachmentDescription color_attachment{};

    color_attachment.format = m_swap_chain.format.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency subpass_dependency{};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;

    subpass_dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;

    subpass_dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &subpass_dependency;

    ZEPH_ENSURE(vkCreateRenderPass(m_logical_device.handle, &render_pass_info,
                                   nullptr, &m_render_pass) != VK_SUCCESS,
                "Couldn't create render pass");
  }

  void create_graphics_pipeline() {
    auto vertex = read_file("assets/shaders/shader.vert.spv");
    auto frag = read_file("assets/shaders/shader.frag.spv");

    VkShaderModule vertex_module = create_shader_module(vertex);
    VkShaderModule frag_module = create_shader_module(frag);

    VkPipelineShaderStageCreateInfo vertex_stage_create_info{};
    VkPipelineShaderStageCreateInfo frag_stage_create_info{};

    vertex_stage_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;

    vertex_stage_create_info.module = vertex_module;
    vertex_stage_create_info.pName = "main";

    frag_stage_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

    frag_stage_create_info.module = frag_module;
    frag_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vertex_stage_create_info,
                                                       frag_stage_create_info};

    std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT,
                                                  VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state{};

    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount =
        static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};

    auto binding_description = Vertex::get_binding_description();
    auto attribute_descriptions = Vertex::get_attribute_descriptions();

    vertex_input_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.vertexAttributeDescriptionCount =
        attribute_descriptions.size();
    vertex_input_info.pVertexAttributeDescriptions =
        attribute_descriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};

    input_assembly_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};

    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_swap_chain.extent.width;
    viewport.height = (float)m_swap_chain.extent.height;

    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swap_chain.extent;

    VkPipelineViewportStateCreateInfo viewport_state_info{};

    viewport_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

    viewport_state_info.viewportCount = 1;
    viewport_state_info.pViewports = &viewport;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_info{};

    rasterizer_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_info.depthClampEnable = VK_FALSE;
    rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_info.lineWidth = 1.0f;
    rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer_info.depthBiasEnable = VK_FALSE;
    rasterizer_info.depthBiasConstantFactor = 0.0f;
    rasterizer_info.depthBiasClamp = 0.0f;
    rasterizer_info.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling_info{};
    multisampling_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_info.sampleShadingEnable = VK_FALSE;
    multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_info.minSampleShading = 1.0f;
    multisampling_info.pSampleMask = nullptr;
    multisampling_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_info.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};

    color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blend_info{};
    color_blend_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_info.logicOpEnable = VK_FALSE;
    color_blend_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_info.attachmentCount = 1;
    color_blend_info.pAttachments = &color_blend_attachment;
    color_blend_info.blendConstants[0] = 0.0f;
    color_blend_info.blendConstants[1] = 0.0f;
    color_blend_info.blendConstants[2] = 0.0f;
    color_blend_info.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_info{};

    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pSetLayouts = nullptr;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;

    ZEPH_ENSURE(vkCreatePipelineLayout(m_logical_device.handle,
                                       &pipeline_layout_info, nullptr,
                                       &m_pipeline_layout) != VK_SUCCESS,

                "Couldn't create pipeline layout");

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;

    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pViewportState = &viewport_state_info;
    pipeline_info.pRasterizationState = &rasterizer_info;
    pipeline_info.pMultisampleState = &multisampling_info;
    pipeline_info.pColorBlendState = &color_blend_info;
    pipeline_info.pDepthStencilState = nullptr;

    pipeline_info.layout = m_pipeline_layout;
    pipeline_info.renderPass = m_render_pass;
    pipeline_info.subpass = 0;

    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    ZEPH_ENSURE(vkCreateGraphicsPipelines(
                    m_logical_device.handle, VK_NULL_HANDLE, 1, &pipeline_info,
                    nullptr, &m_graphics_pipeline) != VK_SUCCESS,
                "Couldn't create graphics pipeline");

    vkDestroyShaderModule(m_logical_device.handle, vertex_module, nullptr);
    vkDestroyShaderModule(m_logical_device.handle, frag_module, nullptr);
  }

  void create_framebuffers() {
    m_swap_chain.framebuffers.resize(m_swap_chain.image_views.size());

    for (size_t i = 0; i < m_swap_chain.framebuffers.size(); i++) {
      VkImageView attachments[] = {m_swap_chain.image_views[i]};

      VkFramebufferCreateInfo framebuffer_info{};
      framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer_info.width = m_swap_chain.extent.width;
      framebuffer_info.height = m_swap_chain.extent.height;
      framebuffer_info.renderPass = m_render_pass;
      framebuffer_info.pAttachments = attachments;
      framebuffer_info.attachmentCount = 1;
      framebuffer_info.layers = 1;

      ZEPH_ENSURE(vkCreateFramebuffer(
                      m_logical_device.handle, &framebuffer_info, nullptr,
                      &m_swap_chain.framebuffers[i]) != VK_SUCCESS,
                  "Couldn't create framebuffer");
    }
  }

  void create_command_pool() {
    VkCommandPoolCreateInfo create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex =
        m_physical_device.queue_family_indices.graphics_family.value();

    ZEPH_ENSURE(vkCreateCommandPool(m_logical_device.handle, &create_info,
                                    nullptr, &m_command_pool) != VK_SUCCESS,
                "Couldn't create command pool");
  }

  void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkBuffer &buffer,
                     VkDeviceMemory &mem_buffer) {
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    ZEPH_ENSURE(
        vkCreateBuffer(m_logical_device.handle, &buffer_info, nullptr, &buffer)

            != VK_SUCCESS,
        "Couldn't create vertex buffer")

    VkMemoryRequirements mem_requirements;

    vkGetBufferMemoryRequirements(m_logical_device.handle, buffer,
                                  &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex =
        find_memory_type(mem_requirements.memoryTypeBits, properties);

    ZEPH_ENSURE(vkAllocateMemory(m_logical_device.handle, &alloc_info, nullptr,
                                 &mem_buffer) != VK_SUCCESS,
                "Couldn't allocate vertex buffer memory");

    vkBindBufferMemory(m_logical_device.handle, buffer, mem_buffer, 0);
  }

  void create_vertex_buffer(std::vector<Vertex> vertices) {
    VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory_buffer;

    create_buffer(buffer_size,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  staging_buffer, staging_memory_buffer);

    void *data;

    vkMapMemory(m_logical_device.handle, staging_memory_buffer, 0,
                (size_t)buffer_size, 0, &data);

    memcpy(data, vertices.data(), buffer_size);

    vkUnmapMemory(m_logical_device.handle, staging_memory_buffer);

    create_buffer(buffer_size,
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertex_buffer,
                  m_vertex_buffer_memory);

    copy_buffer(staging_buffer, m_vertex_buffer, buffer_size);

    vkDestroyBuffer(m_logical_device.handle, staging_buffer, nullptr);
    vkFreeMemory(m_logical_device.handle, staging_memory_buffer, nullptr);
  }

  void create_index_buffer(std::vector<VertexIndice> indices) {
    VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();

    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory_buffer;

    create_buffer(buffer_size,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  staging_buffer, staging_memory_buffer);

    void *data;

    vkMapMemory(m_logical_device.handle, staging_memory_buffer, 0,
                (size_t)buffer_size, 0, &data);

    memcpy(data, indices.data(), buffer_size);

    vkUnmapMemory(m_logical_device.handle, staging_memory_buffer);

    create_buffer(buffer_size,
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_index_buffer,
                  m_index_buffer_memory);

    copy_buffer(staging_buffer, m_index_buffer, buffer_size);

    vkDestroyBuffer(m_logical_device.handle, staging_buffer, nullptr);
    vkFreeMemory(m_logical_device.handle, staging_memory_buffer, nullptr);
  }

  void copy_buffer(VkBuffer source_buffer, VkBuffer destination_buffer,
                   VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocate_info{};

    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandPool = m_command_pool;
    allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;

    ZEPH_ENSURE(

        vkAllocateCommandBuffers(m_logical_device.handle, &allocate_info,
                                 &command_buffer) != VK_SUCCESS,
        "Couldn't allocate command buffer");

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);

    VkBufferCopy copy_region{};

    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;

    vkCmdCopyBuffer(command_buffer, source_buffer, destination_buffer, 1,
                    &copy_region);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(m_logical_device.graphics_queue, 1, &submit_info,
                  VK_NULL_HANDLE);

    vkQueueWaitIdle(m_logical_device.graphics_queue);

    vkFreeCommandBuffers(m_logical_device.handle, m_command_pool, 1,
                         &command_buffer);
  }

  uint32_t find_memory_type(uint32_t type_filter,
                            VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memory_properties;

    vkGetPhysicalDeviceMemoryProperties(m_physical_device.handle,
                                        &memory_properties);

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

  void create_command_buffers() {
    m_command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo alloc_info{};

    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = m_command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = m_command_buffers.size();

    ZEPH_ENSURE(vkAllocateCommandBuffers(m_logical_device.handle, &alloc_info,
                                         m_command_buffers.data()) !=
                    VK_SUCCESS,
                "Couldn't allocate command buffer");
  }

  void push_command_buffer(VkCommandBuffer command_buffer, Mesh mesh,
                           uint32_t image_index, uint32_t frame_index) {
    VkCommandBufferBeginInfo begin_info{};

    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;

    ZEPH_ENSURE(vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS,
                "Couldn't begin to push command buffer");

    VkRenderPassBeginInfo render_pass_info{};

    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = m_render_pass;
    render_pass_info.framebuffer = m_swap_chain.framebuffers[image_index];

    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = m_swap_chain.extent;

    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(command_buffer, &render_pass_info,
                         VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      m_graphics_pipeline);

    VkBuffer vertex_buffers[] = {m_vertex_buffer};
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

    vkCmdBindIndexBuffer(command_buffer, m_index_buffer, 0,
                         VK_INDEX_TYPE_UINT32);

    VkViewport viewport{};

    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_swap_chain.extent.width;
    viewport.height = (float)m_swap_chain.extent.height;

    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swap_chain.extent;

    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(mesh.indices.size()),
                     1, 0, 0, 0);

    vkCmdEndRenderPass(command_buffer);

    ZEPH_ENSURE(vkEndCommandBuffer(m_command_buffers[frame_index]) !=
                    VK_SUCCESS,
                "Couldn't create shader");
  }

  void create_semaphores() {
    m_render_finished_semaphores.resize(m_swap_chain.image_views.size());
    m_image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (size_t i = 0; i < m_swap_chain.image_views.size(); i++) {
      ZEPH_ENSURE(
          vkCreateSemaphore(m_logical_device.handle, &semaphore_info, nullptr,
                            &m_render_finished_semaphores[i]) != VK_SUCCESS,
          "Couldn't render finished semaphore");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      ZEPH_ENSURE(
          vkCreateSemaphore(m_logical_device.handle, &semaphore_info, nullptr,
                            &m_image_available_semaphores[i]) != VK_SUCCESS,
          "Couldn't image available semaphore");
    }
  }

  void create_sync_objects() {
    m_render_finished_semaphores.resize(m_swap_chain.image_views.size());
    m_in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    create_semaphores();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

      ZEPH_ENSURE(vkCreateFence(m_logical_device.handle, &fence_info, nullptr,
                                &m_in_flight_fences[i]) != VK_SUCCESS,
                  "Couldn't in_flight fence");
    }
  }

  VkShaderModule create_shader_module(const std::vector<char> byte_code) {
    VkShaderModuleCreateInfo create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = byte_code.size();
    create_info.pCode = reinterpret_cast<const uint32_t *>(byte_code.data());

    VkShaderModule shader_module;

    ZEPH_ENSURE(vkCreateShaderModule(m_logical_device.handle, &create_info,
                                     nullptr, &shader_module) != VK_SUCCESS,
                "Couldn't create shader");

    return shader_module;
  }

  bool
  is_device_suitable_candidate(VkPhysicalDevice device,
                               QueueFamilyIndices queue_family_indices,
                               SwapChainSupport swap_chain_support,
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

  bool ensure_device_extensions_support(
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

  QueueFamilyIndices find_queue_families(VkPhysicalDevice device,
                                         uint32_t queues) {
    QueueFamilyIndices indices;

    uint32_t queue_family_count;

    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                             nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);

    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                             queue_families.data());

    int i = 0;
    for (auto &queue_family : queue_families) {
      if (indices.exists())
        break;

      if (queue_family.queueFlags & queues) {
        indices.graphics_family = i;
      }

      VkBool32 is_present_support_available = false;

      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface,
                                           &is_present_support_available);

      if (is_present_support_available) {
        indices.present_family = i;
      }

      i++;
    }

    return indices;
  };

  SwapChainSupport find_swap_chain_support(VkPhysicalDevice device) {
    SwapChainSupport support;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface,
                                              &support.capabilities);

    uint32_t present_mode_count;

    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface,
                                              &present_mode_count, nullptr);

    if (present_mode_count != 0) {
      support.preset_modes.resize(present_mode_count);

      vkGetPhysicalDeviceSurfacePresentModesKHR(
          device, m_surface, &present_mode_count, support.preset_modes.data());
    }

    uint32_t format_count;

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count,
                                         nullptr);

    if (format_count != 0) {
      support.formats.resize(format_count);

      vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count,
                                           support.formats.data());
    }

    return support;
  };

  VkSurfaceFormatKHR choose_swap_surface_format(
      const std::vector<VkSurfaceFormatKHR> &available_formats) {

    for (const auto available_format : available_formats) {
      if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
          available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        return available_format;
      }
    }

    return available_formats[0];
  }

  VkPresentModeKHR choose_swap_present_mode(
      const std::vector<VkPresentModeKHR> &available_present_modes) {
    for (const auto available_preset_mode : available_present_modes) {
      if (available_preset_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
        return available_preset_mode;
      }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D choose_swap_extent(GLFWwindow *window,
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

  int rate_suitable_device(VkPhysicalDeviceProperties device_properties) {
    const uint32_t HIGH_SCORE_ATTRIBUTION = 1000;

    int score = 0;

    score +=
        device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
            ? HIGH_SCORE_ATTRIBUTION
            : 0;

    score += device_properties.limits.maxImageDimension2D;

    return score;
  }

  VkApplicationInfo declare_application() {
    VkApplicationInfo app_info{};

    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "zephyr";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "zephyr";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;
    app_info.pNext = nullptr;

    return app_info;
  }

  std::vector<const char *> get_required_extensions() {
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

  bool ensure_extensions_support(std::vector<const char *> extensions) {
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

#ifdef ENABLE_VALIDATION_LAYER
  void create_debug_messenger() {
    ZEPH_ENSURE(m_instance == nullptr,
                "Instance must be created before debug_messenger");

    VkDebugUtilsMessengerCreateInfoEXT create_info = declare_debug_messenger();

    ZEPH_ENSURE(create_debug_utils_messenger_ext(m_instance, &create_info,
                                                 nullptr, &m_debug_messenger) !=
                    VK_SUCCESS,
                "Couldn't create debug messenger");
  };

  VkDebugUtilsMessengerCreateInfoEXT declare_debug_messenger() {
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

  bool
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

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                 VkDebugUtilsMessageTypeFlagsEXT message_type,
                 const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
                 void *p_user_data) {
    LOG_DEBUG("Validation layer: ", p_callback_data->pMessage);

    return VK_FALSE;
  };
#endif

  void cleanup() {
    cleanup_swap_chain(m_swap_chain, true);

    vkDestroyBuffer(m_logical_device.handle, m_vertex_buffer, nullptr);
    vkDestroyBuffer(m_logical_device.handle, m_index_buffer, nullptr);
    vkFreeMemory(m_logical_device.handle, m_vertex_buffer_memory, nullptr);
    vkFreeMemory(m_logical_device.handle, m_index_buffer_memory, nullptr);

    vkDestroyPipeline(m_logical_device.handle, m_graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(m_logical_device.handle, m_pipeline_layout,
                            nullptr);

    vkDestroyRenderPass(m_logical_device.handle, m_render_pass, nullptr);

    cleanup_semaphores();

    for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroyFence(m_logical_device.handle, m_in_flight_fences[i], nullptr);
    }

    vkDestroyCommandPool(m_logical_device.handle, m_command_pool, nullptr);

    vkDestroyDevice(m_logical_device.handle, nullptr);
#ifdef ENABLE_VALIDATION_LAYER
    destroy_debug_utils_messenger_ext(m_instance, m_debug_messenger, nullptr);
#endif
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
  };

  LogicalDevice logical_device() const { return m_logical_device; }
  SwapChain swap_chain() const { return m_swap_chain; }
  std::vector<VkFence> in_flight_fences() { return m_in_flight_fences; }

  std::vector<VkCommandBuffer> command_buffers() { return m_command_buffers; }

  std::vector<VkSemaphore> image_available_semaphores() {
    return m_image_available_semaphores;
  }

  std::vector<VkSemaphore> render_finished_semaphores() {
    return m_render_finished_semaphores;
  }

  uint32_t constexpr max_frames_in_flight() const noexcept {
    return MAX_FRAMES_IN_FLIGHT;
  }

private:
  VkInstance m_instance;
#ifdef ENABLE_VALIDATION_LAYER
  VkDebugUtilsMessengerEXT m_debug_messenger;
#endif
  PhysicalDevice m_physical_device;
  LogicalDevice m_logical_device;

  VkSurfaceKHR m_surface;

  VkRenderPass m_render_pass;
  VkPipelineLayout m_pipeline_layout;
  VkPipeline m_graphics_pipeline;

  VkCommandPool m_command_pool;
  std::vector<VkCommandBuffer> m_command_buffers;

  SwapChain m_swap_chain;

  std::vector<VkSemaphore> m_image_available_semaphores;
  std::vector<VkSemaphore> m_render_finished_semaphores;
  std::vector<VkFence> m_in_flight_fences;

  const uint8_t MAX_FRAMES_IN_FLIGHT = 2;

  VkBuffer m_vertex_buffer;
  VkDeviceMemory m_vertex_buffer_memory;

  VkBuffer m_index_buffer;
  VkDeviceMemory m_index_buffer_memory;
};

} // namespace zephyr
