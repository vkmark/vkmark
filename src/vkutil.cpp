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

#include "vkutil.h"
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

vkutil::RenderPassBuilder::RenderPassBuilder(VulkanState& vulkan)
    : vulkan{vulkan},
      format{vk::Format::eUndefined}
{
}

vkutil::RenderPassBuilder& vkutil::RenderPassBuilder::set_format(vk::Format format_)
{
    format = format_;
    return *this;
}

ManagedResource<vk::RenderPass> vkutil::RenderPassBuilder::build()
{
    auto const color_attachment = vk::AttachmentDescription{}
        .setFormat(format)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    auto const color_attachment_ref = vk::AttachmentReference{}
        .setAttachment(0)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    auto const subpass = vk::SubpassDescription{}
        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachmentCount(1)
        .setPColorAttachments(&color_attachment_ref);

    auto const render_pass_create_info = vk::RenderPassCreateInfo{}
        .setAttachmentCount(1)
        .setPAttachments(&color_attachment)
        .setSubpassCount(1)
        .setPSubpasses(&subpass);

    return ManagedResource<vk::RenderPass>{
        vulkan.device().createRenderPass(render_pass_create_info),
        [vptr=&vulkan] (auto const& rp) { vptr->device().destroyRenderPass(rp); }};
}

vkutil::PipelineBuilder::PipelineBuilder(VulkanState& vulkan)
    : vulkan{vulkan}
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
        .setTopology(vk::PrimitiveTopology::eTriangleStrip)
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
        .setFrontFace(vk::FrontFace::eClockwise)
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
        .setBlendEnable(false);

    auto const color_blend_state_create_info = vk::PipelineColorBlendStateCreateInfo{}
        .setLogicOpEnable(false)
        .setAttachmentCount(1)
        .setPAttachments(&blend_attach);

    auto pipeline_create_info = vk::GraphicsPipelineCreateInfo{}
        .setStageCount(2)
        .setPStages(shader_stages)
        .setPVertexInputState(&vertex_input_state_create_info)
        .setPInputAssemblyState(&input_assembly_state_create_info)
        .setPViewportState(&viewport_state_create_info)
        .setPRasterizationState(&rasterization_state_create_info)
        .setPMultisampleState(&multisample_state_create_info)
        .setPColorBlendState(&color_blend_state_create_info)
        .setLayout(layout)
        .setRenderPass(render_pass)
        .setSubpass(0);

    return ManagedResource<vk::Pipeline>{
        vulkan.device().createGraphicsPipeline({}, pipeline_create_info),
        [vptr=&vulkan] (auto const& p) { vptr->device().destroyPipeline(p); }};
}

vkutil::ImageViewBuilder::ImageViewBuilder(VulkanState& vulkan)
    : vulkan{vulkan}
{
}

vkutil::ImageViewBuilder& vkutil::ImageViewBuilder::set_image(vk::Image image_)
{
    image = image_;
    return *this;
}

vkutil::ImageViewBuilder& vkutil::ImageViewBuilder::set_format(vk::Format format_)
{
    format = format_;
    return *this;
}

ManagedResource<vk::ImageView> vkutil::ImageViewBuilder::build()
{
    auto const image_subresource_range = vk::ImageSubresourceRange{}
        .setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setBaseMipLevel(0)
        .setLevelCount(1)
        .setBaseArrayLayer(0)
        .setLayerCount(1);

    auto const image_view_create_info = vk::ImageViewCreateInfo{}
        .setImage(image)
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(format)
        .setSubresourceRange(image_subresource_range);

    return ManagedResource<vk::ImageView>{
        vulkan.device().createImageView(image_view_create_info),
        [vptr=&vulkan] (auto const& iv) { vptr->device().destroyImageView(iv); }};
}

vkutil::FramebufferBuilder::FramebufferBuilder(VulkanState& vulkan)
    : vulkan{vulkan}
{
}

vkutil::FramebufferBuilder& vkutil::FramebufferBuilder::set_render_pass(
    vk::RenderPass render_pass_)
{
    render_pass = render_pass_;
    return *this;
}

vkutil::FramebufferBuilder& vkutil::FramebufferBuilder::set_image_view(
    vk::ImageView image_view_)
{
    image_view = image_view_;
    return *this;
}

vkutil::FramebufferBuilder& vkutil::FramebufferBuilder::set_extent(
    vk::Extent2D extent_)
{
    extent = extent_;
    return *this;
}

ManagedResource<vk::Framebuffer> vkutil::FramebufferBuilder::build()
{
    auto const framebuffer_create_info = vk::FramebufferCreateInfo{}
        .setRenderPass(render_pass)
        .setAttachmentCount(1)
        .setPAttachments(&image_view)
        .setWidth(extent.width)
        .setHeight(extent.height)
        .setLayers(1);

    return ManagedResource<vk::Framebuffer>{
        vulkan.device().createFramebuffer(framebuffer_create_info),
        [vptr=&vulkan] (auto const& fb) { vptr->device().destroyFramebuffer(fb); }};
}

vkutil::BufferBuilder::BufferBuilder(VulkanState& vulkan)
    : vulkan{vulkan},
      size{0},
      memory_out_ptr{nullptr}
{
}

vkutil::BufferBuilder& vkutil::BufferBuilder::set_size(size_t size_)
{
    size = size_;
    return *this;
}

vkutil::BufferBuilder& vkutil::BufferBuilder::set_usage(vk::BufferUsageFlags usage_)
{
    usage = usage_;
    return *this;
}

vkutil::BufferBuilder& vkutil::BufferBuilder::set_memory_properties(
    vk::MemoryPropertyFlags memory_properties_)
{
    memory_properties = memory_properties_;
    return *this;
}

vkutil::BufferBuilder& vkutil::BufferBuilder::set_memory_out(
    vk::DeviceMemory& memory_out)
{
    memory_out_ptr = &memory_out;
    return *this;
}

ManagedResource<vk::Buffer> vkutil::BufferBuilder::build()
{
    auto const vertex_buffer_create_info = vk::BufferCreateInfo{}
        .setSize(size)
        .setUsage(usage)
        .setSharingMode(vk::SharingMode::eExclusive);

    auto vk_buffer = ManagedResource<vk::Buffer>{
        vulkan.device().createBuffer(vertex_buffer_create_info),
        [vptr=&vulkan] (auto const& b) { vptr->device().destroyBuffer(b); }};

    auto const mem_requirements = vulkan.device().getBufferMemoryRequirements(vk_buffer);
    auto const mem_type = find_matching_memory_type_for(mem_requirements);

    auto const memory_allocate_info = vk::MemoryAllocateInfo{}
        .setAllocationSize(mem_requirements.size)
        .setMemoryTypeIndex(mem_type);

    auto vk_mem = ManagedResource<vk::DeviceMemory>{
        vulkan.device().allocateMemory(memory_allocate_info),
        [vptr=&vulkan] (auto const& m) { vptr->device().freeMemory(m); }};

    vulkan.device().bindBufferMemory(vk_buffer, vk_mem, 0);

    if (memory_out_ptr)
        *memory_out_ptr = vk_mem.raw;

    return ManagedResource<vk::Buffer>{
        vk_buffer.steal(),
        [vptr=&vulkan, mem=vk_mem.steal()]
        (auto const& b)
        {
            vptr->device().freeMemory(mem);
            vptr->device().destroyBuffer(b);
        }};
}

uint32_t vkutil::BufferBuilder::find_matching_memory_type_for(
    vk::MemoryRequirements const& requirements)
{
    auto const properties = vulkan.physical_device().getMemoryProperties();

    for (uint32_t i = 0; i < properties.memoryTypeCount; i++)
    {
        if ((requirements.memoryTypeBits & (1 << i)) &&
            (properties.memoryTypes[i].propertyFlags & memory_properties) == memory_properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Couldn't find mathcing memory type for buffer");
}

vkutil::DescriptorSetBuilder::DescriptorSetBuilder(VulkanState& vulkan)
    : vulkan{vulkan},
      buffer{nullptr},
      offset{0},
      range{0},
      layout_out_ptr{nullptr}
{
}

vkutil::DescriptorSetBuilder& vkutil::DescriptorSetBuilder::set_type(
    vk::DescriptorType type)
{
    descriptor_type = type;
    return *this;
}

vkutil::DescriptorSetBuilder& vkutil::DescriptorSetBuilder::set_stage_flags(
    vk::ShaderStageFlags stage_flags_)
{
    stage_flags = stage_flags_;
    return *this;
}

vkutil::DescriptorSetBuilder& vkutil::DescriptorSetBuilder::set_buffer(
    vk::Buffer& buffer_, size_t offset_, size_t range_)
{
    buffer = &buffer_;
    offset = offset_;
    range = range_;
    return *this;
}

vkutil::DescriptorSetBuilder& vkutil::DescriptorSetBuilder::set_layout_out(
    vk::DescriptorSetLayout& layout_out)
{
    layout_out_ptr = &layout_out;
    return *this;
}

ManagedResource<vk::DescriptorSet> vkutil::DescriptorSetBuilder::build()
{
    // Layout
    auto const uniform_layout_binding = vk::DescriptorSetLayoutBinding{}
        .setBinding(0)
        .setDescriptorType(descriptor_type)
        .setDescriptorCount(1)
        .setStageFlags(stage_flags);

    auto const descriptor_set_layout_create_info = vk::DescriptorSetLayoutCreateInfo{}
        .setBindingCount(1)
        .setPBindings(&uniform_layout_binding);

    auto descriptor_set_layout = ManagedResource<vk::DescriptorSetLayout>{
        vulkan.device().createDescriptorSetLayout(descriptor_set_layout_create_info),
        [vptr=&vulkan] (auto const& dsl) { vptr->device().destroyDescriptorSetLayout(dsl); }};

    // Descriptor pool and sets
    auto const descriptor_pool_size = vk::DescriptorPoolSize{}
        .setDescriptorCount(1)
        .setType(vk::DescriptorType::eUniformBuffer);

    auto const descriptor_pool_create_info = vk::DescriptorPoolCreateInfo{}
        .setPoolSizeCount(1)
        .setPPoolSizes(&descriptor_pool_size)
        .setMaxSets(1);

    auto descriptor_pool = ManagedResource<vk::DescriptorPool>{
        vulkan.device().createDescriptorPool(descriptor_pool_create_info),
        [vptr=&vulkan] (auto const& p) { vptr->device().destroyDescriptorPool(p); }};

    auto const descriptor_set_allocate_info = vk::DescriptorSetAllocateInfo{}
        .setDescriptorPool(descriptor_pool)
        .setDescriptorSetCount(1)
        .setPSetLayouts(&descriptor_set_layout.raw);

    auto descriptor_set = vulkan.device().allocateDescriptorSets(descriptor_set_allocate_info);

    // Update descriptor set
    auto const descriptor_buffer_info = vk::DescriptorBufferInfo{}
        .setBuffer(*buffer)
        .setOffset(offset)
        .setRange(range);

    auto const write_descriptor_set = vk::WriteDescriptorSet{}
        .setDstSet(descriptor_set[0])
        .setDstBinding(0)
        .setDstArrayElement(0)
        .setDescriptorType(descriptor_type)
        .setDescriptorCount(1)
        .setPBufferInfo(&descriptor_buffer_info);

    vulkan.device().updateDescriptorSets(write_descriptor_set, {});

    if (layout_out_ptr)
        *layout_out_ptr = descriptor_set_layout.raw;

    return ManagedResource<vk::DescriptorSet>{
        std::move(descriptor_set[0]),
        [vptr=&vulkan, layout=descriptor_set_layout.steal(), pool=descriptor_pool.steal()]
        (auto const&)
        {
            vptr->device().destroyDescriptorPool(pool);
            vptr->device().destroyDescriptorSetLayout(layout);
        }};
}
