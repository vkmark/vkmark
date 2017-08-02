/*
 * Copyright © 2017 Collabora Ltd.
 *
 * This file is part of vkmark.
 *
 * vkmark is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * vkmark is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with vkmark. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Alexandros Frantzis <alexandros.frantzis@collabora.com>
 */

#include "pipeline_builder.h"

#include "vulkan_state.h"

namespace
{

ManagedResource<vk::ShaderModule> create_shader_module(
    vk::Device const& device, std::vector<char> const& code)
{
    std::vector<uint32_t> code_aligned(code.size() / 4 + 1);
    memcpy(code_aligned.data(), code.data(), code.size());

    auto const shader_module_create_info = vk::ShaderModuleCreateInfo{}
        .setCodeSize(code.size())
        .setPCode(code_aligned.data());

    return ManagedResource<vk::ShaderModule>{
        device.createShaderModule(shader_module_create_info),
        [dptr=&device] (auto const& sm) { dptr->destroyShaderModule(sm); }};
}

}

vkutil::PipelineBuilder::PipelineBuilder(VulkanState& vulkan)
    : vulkan{vulkan},
      depth_test{false},
      blend{false}
{
}

vkutil::PipelineBuilder& vkutil::PipelineBuilder::set_vertex_input(
    std::vector<vk::VertexInputBindingDescription> const& binding_descriptions_,
    std::vector<vk::VertexInputAttributeDescription> const& attribute_descriptions_)
{
    binding_descriptions = binding_descriptions_;
    attribute_descriptions = attribute_descriptions_;
    return *this;
}

vkutil::PipelineBuilder& vkutil::PipelineBuilder::set_vertex_shader(
    std::vector<char> const& spirv)
{
    vertex_shader_spirv = spirv;
    return *this;
}

vkutil::PipelineBuilder& vkutil::PipelineBuilder::set_fragment_shader(
    std::vector<char> const& spirv)
{
    fragment_shader_spirv = spirv;
    return *this;
}

vkutil::PipelineBuilder& vkutil::PipelineBuilder::set_depth_test(bool depth_test_)
{
    depth_test = depth_test_;
    return *this;
}

vkutil::PipelineBuilder& vkutil::PipelineBuilder::set_extent(vk::Extent2D extent_)
{
    extent = extent_;
    return *this;
}

vkutil::PipelineBuilder& vkutil::PipelineBuilder::set_layout(vk::PipelineLayout layout_)
{
    layout = layout_;
    return *this;
}

vkutil::PipelineBuilder& vkutil::PipelineBuilder::set_render_pass(vk::RenderPass render_pass_)
{
    render_pass = render_pass_;
    return *this;
}

vkutil::PipelineBuilder& vkutil::PipelineBuilder::set_blend(bool blend_)
{
    blend = blend_;
    return *this;
}

ManagedResource<vk::Pipeline> vkutil::PipelineBuilder::build()
{
    auto const vertex_shader = create_shader_module(vulkan.device(), vertex_shader_spirv);
    auto const fragment_shader = create_shader_module(vulkan.device(), fragment_shader_spirv);

    auto const vertex_shader_stage_create_info = vk::PipelineShaderStageCreateInfo{}
        .setStage(vk::ShaderStageFlagBits::eVertex)
        .setModule(vertex_shader)
        .setPName("main");
    auto const fragment_shader_stage_create_info = vk::PipelineShaderStageCreateInfo{}
        .setStage(vk::ShaderStageFlagBits::eFragment)
        .setModule(fragment_shader)
        .setPName("main");

    vk::PipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_create_info,
        fragment_shader_stage_create_info};

    auto const vertex_input_state_create_info = vk::PipelineVertexInputStateCreateInfo{}
        .setVertexBindingDescriptionCount(binding_descriptions.size())
        .setPVertexBindingDescriptions(binding_descriptions.data())
        .setVertexAttributeDescriptionCount(attribute_descriptions.size())
        .setPVertexAttributeDescriptions(attribute_descriptions.data());

    auto const input_assembly_state_create_info = vk::PipelineInputAssemblyStateCreateInfo{}
        .setTopology(vk::PrimitiveTopology::eTriangleList)
        .setPrimitiveRestartEnable(false);

    auto const viewport = vk::Viewport{}
        .setWidth(extent.width)
        .setHeight(extent.height)
        .setMinDepth(0.0f)
        .setMaxDepth(1.0f);

    auto const scissor = vk::Rect2D{}
        .setOffset({0, 0})
        .setExtent(extent);

    auto const viewport_state_create_info = vk::PipelineViewportStateCreateInfo{}
        .setViewportCount(1)
        .setPViewports(&viewport)
        .setScissorCount(1)
        .setPScissors(&scissor);

    auto const rasterization_state_create_info = vk::PipelineRasterizationStateCreateInfo{}
        .setDepthClampEnable(false)
        .setRasterizerDiscardEnable(false)
        .setPolygonMode(vk::PolygonMode::eFill)
        .setLineWidth(1.0f)
        .setCullMode(vk::CullModeFlagBits::eBack)
        .setFrontFace(vk::FrontFace::eCounterClockwise)
        .setDepthBiasEnable(false);

    auto const multisample_state_create_info = vk::PipelineMultisampleStateCreateInfo{}
        .setSampleShadingEnable(false)
        .setRasterizationSamples(vk::SampleCountFlagBits::e1);

    auto const blend_attach = vk::PipelineColorBlendAttachmentState{}
        .setColorWriteMask(
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA)
        .setBlendEnable(blend)
        .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
        .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
        .setAlphaBlendOp(vk::BlendOp::eAdd);

    auto const color_blend_state_create_info = vk::PipelineColorBlendStateCreateInfo{}
        .setLogicOpEnable(false)
        .setAttachmentCount(1)
        .setPAttachments(&blend_attach);

    auto const depth_stencil_state_create_info = vk::PipelineDepthStencilStateCreateInfo{}
        .setDepthTestEnable(depth_test)
        .setDepthWriteEnable(depth_test)
        .setDepthCompareOp(vk::CompareOp::eLess)
        .setDepthBoundsTestEnable(false)
        .setStencilTestEnable(false);

    auto pipeline_create_info = vk::GraphicsPipelineCreateInfo{}
        .setStageCount(2)
        .setPStages(shader_stages)
        .setPVertexInputState(&vertex_input_state_create_info)
        .setPInputAssemblyState(&input_assembly_state_create_info)
        .setPViewportState(&viewport_state_create_info)
        .setPRasterizationState(&rasterization_state_create_info)
        .setPMultisampleState(&multisample_state_create_info)
        .setPColorBlendState(&color_blend_state_create_info)
        .setPDepthStencilState(&depth_stencil_state_create_info)
        .setLayout(layout)
        .setRenderPass(render_pass)
        .setSubpass(0);

    return ManagedResource<vk::Pipeline>{
        vulkan.device().createGraphicsPipeline({}, pipeline_create_info),
        [vptr=&vulkan] (auto const& p) { vptr->device().destroyPipeline(p); }};
}
