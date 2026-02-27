#pragma once
#include "platforms/vulkan/device.hpp"
#include "platforms/vulkan/swap-chain.hpp"
#include <vulkan/vulkan_core.h>

namespace zephyr {

static const VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

class VulkanRenderPass {
public:
  VulkanRenderPass() = default;
  VulkanRenderPass(VkRenderPass handle) : handle(handle) {}

  static VulkanRenderPass create(const VulkanSwapChain swap_chain,
                                 const VulkanLogicalDevice logical_device) {
    VkRenderPass render_pass_handle;

    VkAttachmentDescription color_attachment{};

    color_attachment.format = swap_chain.surface_format.format;
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

    ZEPH_ENSURE(vkCreateRenderPass(logical_device.handle, &render_pass_info,
                                   nullptr, &render_pass_handle) != VK_SUCCESS,
                "Couldn't create render pass");

    return VulkanRenderPass(render_pass_handle);
  }

  static VkRenderPassBeginInfo declare_begin(VkRenderPass render_pass,
                                             VkFramebuffer framebuffer,
                                             VkExtent2D render_area_extent) {
    VkRenderPassBeginInfo render_pass_info{};

    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass;
    render_pass_info.framebuffer = framebuffer;

    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = render_area_extent;

    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    return render_pass_info;
  }

  VkRenderPass handle;
};

} // namespace zephyr
