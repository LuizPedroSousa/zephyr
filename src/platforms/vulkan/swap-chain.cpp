#include "platforms/vulkan/swap-chain.hpp"
#include "platforms/vulkan/device.hpp"
#include "platforms/vulkan/render-pass.hpp"
#include <utility>

namespace zephyr {

VulkanSwapChain VulkanSwapChain::create(
    const Window window, VulkanPhysicalDevice physical_device,
    VulkanLogicalDevice logical_device, const VulkanSurface surface,
    std::optional<VulkanSwapChain> existent_chain) {
  auto surface_format = VulkanSwapChainPicker::choose_surface_format(
      physical_device.swap_chain_support.formats);

  auto present_mode = VulkanSwapChainPicker::choose_present_mode(
      physical_device.swap_chain_support.preset_modes);

  auto extent = VulkanSwapChainPicker::choose_extent(
      window.handle(), physical_device.swap_chain_support.capabilities);

  uint32_t image_count =
      physical_device.swap_chain_support.capabilities.minImageCount + 1;

  if (physical_device.swap_chain_support.capabilities.maxImageCount > 0 &&
      image_count >
          physical_device.swap_chain_support.capabilities.maxImageCount) {

    image_count = physical_device.swap_chain_support.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create_info{};

  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = surface.handle();
  create_info.minImageCount = image_count;
  create_info.imageFormat = surface_format.format;
  create_info.imageColorSpace = surface_format.colorSpace;
  create_info.imageExtent = extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  auto indices = physical_device.queue_family_indices;

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
      physical_device.swap_chain_support.capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = present_mode;
  create_info.clipped = VK_TRUE;

  std::vector<VkSwapchainKHR> retired_chain_handles;

  std::vector<VkImage> images;
  std::vector<VkImageView> image_views;
  std::vector<VkFramebuffer> framebuffers;

  if (existent_chain.has_value() && existent_chain.value().is_handle_valid()) {
    create_info.oldSwapchain = existent_chain.value().handle;
    retired_chain_handles.push_back(existent_chain.value().handle);
  } else {
    create_info.oldSwapchain = VK_NULL_HANDLE;
  }

  VkSwapchainKHR handle;

  ZEPH_ENSURE(vkCreateSwapchainKHR(logical_device.handle, &create_info, nullptr,
                                   &handle) != VK_SUCCESS,
              "Couldn't create swap chain");

  uint32_t swap_chain_images_count = 0;

  vkGetSwapchainImagesKHR(logical_device.handle, handle,
                          &swap_chain_images_count, nullptr);

  images.resize(swap_chain_images_count);

  vkGetSwapchainImagesKHR(logical_device.handle, handle,
                          &swap_chain_images_count, images.data());

  return VulkanSwapChain(handle, surface_format, present_mode, extent,
                         logical_device.handle, images, image_views,
                         framebuffers, retired_chain_handles);
}

void VulkanSwapChain::create_image_views(VkDevice logical_device_handle,
                                         VulkanSwapChain &swap_chain) {
  swap_chain.image_views.resize(swap_chain.images.size());

  for (size_t i = 0; i < swap_chain.images.size(); i++) {
    VkImageViewCreateInfo create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = swap_chain.images[i];
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = swap_chain.surface_format.format;

    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    ZEPH_ENSURE(vkCreateImageView(logical_device_handle, &create_info, nullptr,
                                  &swap_chain.image_views[i]) != VK_SUCCESS,
                "Couldn't create image view");
  };
}

void VulkanSwapChain::create_framebuffers(VulkanLogicalDevice logical_device,
                                          VulkanSwapChain &swap_chain,
                                          VulkanRenderPass render_pass) {
  swap_chain.framebuffers.resize(swap_chain.image_views.size());

  for (size_t i = 0; i < swap_chain.framebuffers.size(); i++) {
    VkImageView attachments[] = {swap_chain.image_views[i]};

    VkFramebufferCreateInfo framebuffer_info{};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.width = swap_chain.extent.width;
    framebuffer_info.height = swap_chain.extent.height;
    framebuffer_info.renderPass = render_pass.handle;
    framebuffer_info.pAttachments = attachments;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.layers = 1;

    ZEPH_ENSURE(vkCreateFramebuffer(logical_device.handle, &framebuffer_info,
                                    nullptr,
                                    &swap_chain.framebuffers[i]) != VK_SUCCESS,
                "Couldn't create framebuffer");
  }
}

void VulkanSwapChain::cleanup() {
  for (auto framebuffer : framebuffers) {
    vkDestroyFramebuffer(logical_device_handle, framebuffer, nullptr);
  }

  for (auto image_view : image_views) {
    vkDestroyImageView(logical_device_handle, image_view, nullptr);
  }

  vkDestroySwapchainKHR(logical_device_handle, handle, nullptr);

  for (auto retired_chain : retired_chain_handles) {
    vkDestroySwapchainKHR(logical_device_handle, retired_chain, nullptr);
  }

  retired_chain_handles.clear();
}

} // namespace zephyr
