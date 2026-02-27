#pragma once

#include "file.hpp"
#include "mesh.hpp"
#include "platforms/vulkan/descriptor-set.hpp"
#include "platforms/vulkan/device.hpp"
#include "platforms/vulkan/render-pass.hpp"
#include "platforms/vulkan/shader.hpp"
#include "platforms/vulkan/swap-chain.hpp"
#include <vulkan/vulkan_core.h>

namespace zephyr {

class VulkanGraphicsPipeline {
public:
  VulkanGraphicsPipeline() = default;
  VulkanGraphicsPipeline(VkDevice ld_handle, VkPipeline handle,
                         VkPipelineLayout pipeline_layout,
                         VulkanRenderPass render_pass)
      : m_handle(handle), m_ld_handle(ld_handle),
        m_pipeline_layout(pipeline_layout), m_render_pass(render_pass) {}

  static VulkanGraphicsPipeline
  create(VulkanLogicalDevice logical_device, VulkanSwapChain swap_chain,
         VulkanDescriptorSetLayout descriptor_set_layout,
         VulkanRenderPass render_pass) {

    auto vertex = read_file("assets/shaders/shader.vert.spv");
    auto frag = read_file("assets/shaders/shader.frag.spv");

    VkShaderModule vertex_module =
        Shader::create_module(logical_device, vertex);
    VkShaderModule frag_module = Shader::create_module(logical_device, frag);

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
    viewport.width = (float)swap_chain.extent.width;
    viewport.height = (float)swap_chain.extent.height;

    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swap_chain.extent;

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
    rasterizer_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

    VkDescriptorSetLayout set_layout = descriptor_set_layout.handle();

    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &set_layout;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;

    VkPipeline handle;
    VkPipelineLayout pipeline_layout;

    ZEPH_ENSURE(vkCreatePipelineLayout(logical_device.handle,
                                       &pipeline_layout_info, nullptr,
                                       &pipeline_layout) != VK_SUCCESS,

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

    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = render_pass.handle;
    pipeline_info.subpass = 0;

    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    ZEPH_ENSURE(vkCreateGraphicsPipelines(logical_device.handle, VK_NULL_HANDLE,
                                          1, &pipeline_info, nullptr,
                                          &handle) != VK_SUCCESS,
                "Couldn't create graphics pipeline");

    vkDestroyShaderModule(logical_device.handle, vertex_module, nullptr);
    vkDestroyShaderModule(logical_device.handle, frag_module, nullptr);

    return VulkanGraphicsPipeline(logical_device.handle, handle, pipeline_layout, render_pass);
  }

  void cleanup() {
    vkDestroyPipeline(m_ld_handle, m_handle, nullptr);
    vkDestroyPipelineLayout(m_ld_handle, m_pipeline_layout, nullptr);

    vkDestroyRenderPass(m_ld_handle, m_render_pass.handle, nullptr);
  }

  inline constexpr VkPipeline handle() const noexcept { return m_handle; }
  inline constexpr VkPipelineLayout layout() const noexcept {
    return m_pipeline_layout;
  }

private:
  VkPipeline m_handle;
  VkDevice m_ld_handle;
  VkPipelineLayout m_pipeline_layout;
  VulkanRenderPass m_render_pass;
};

} // namespace zephyr
