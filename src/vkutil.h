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

#pragma once

#include <vulkan/vulkan.hpp>

#include "managed_resource.h"

class VulkanState;

namespace vkutil
{

class RenderPassBuilder
{
public:
    RenderPassBuilder(VulkanState& vulkan);

    RenderPassBuilder& set_format(vk::Format format);

    ManagedResource<vk::RenderPass> build();

private:
    VulkanState& vulkan;
    vk::Format format;
};

class PipelineBuilder
{
public:
    PipelineBuilder(VulkanState& vulkan);

    PipelineBuilder& set_vertex_input(
        std::vector<vk::VertexInputBindingDescription> const& binding_descriptions,
        std::vector<vk::VertexInputAttributeDescription> const& attribute_descriptions);
    PipelineBuilder& set_vertex_shader(std::vector<char> const& spirv);
    PipelineBuilder& set_fragment_shader(std::vector<char> const& spirv);
    PipelineBuilder& set_extent(vk::Extent2D extent);
    PipelineBuilder& set_layout(vk::PipelineLayout layout);
    PipelineBuilder& set_render_pass(vk::RenderPass render_pass);

    ManagedResource<vk::Pipeline> build();

private:
    VulkanState& vulkan;
    std::vector<vk::VertexInputBindingDescription> binding_descriptions;
    std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;
    std::vector<char> vertex_shader_spirv;
    std::vector<char> fragment_shader_spirv;
    vk::Extent2D extent;
    vk::PipelineLayout layout;
    vk::RenderPass render_pass;
};

class ImageViewBuilder
{
public:
    ImageViewBuilder(VulkanState& vulkan);

    ImageViewBuilder& set_image(vk::Image image);
    ImageViewBuilder& set_format(vk::Format image);

    ManagedResource<vk::ImageView> build();

private:
    VulkanState& vulkan;
    vk::Image image;
    vk::Format format;
};

class FramebufferBuilder
{
public:
    FramebufferBuilder(VulkanState& vulkan);

    FramebufferBuilder& set_render_pass(vk::RenderPass render_pass);
    FramebufferBuilder& set_image_view(vk::ImageView image_view);
    FramebufferBuilder& set_extent(vk::Extent2D extent);

    ManagedResource<vk::Framebuffer> build();

private:
    VulkanState& vulkan;
    vk::RenderPass render_pass;
    vk::ImageView image_view;
    vk::Extent2D extent;
};

class BufferBuilder
{
public:
    BufferBuilder(VulkanState& vulkan);

    BufferBuilder& set_size(size_t size);
    BufferBuilder& set_usage(vk::BufferUsageFlags usage);
    BufferBuilder& set_memory_properties(vk::MemoryPropertyFlags memory_properties);
    BufferBuilder& set_memory_out(vk::DeviceMemory& memory_out);

    ManagedResource<vk::Buffer> build();

private:
    uint32_t find_matching_memory_type_for(vk::MemoryRequirements const& requirements);

    VulkanState& vulkan;
    size_t size;
    vk::BufferUsageFlags usage;
    vk::MemoryPropertyFlags memory_properties;
    vk::DeviceMemory* memory_out_ptr;
};

class DescriptorSetBuilder
{
public:
    DescriptorSetBuilder(VulkanState& vulkan);

    DescriptorSetBuilder& set_type(vk::DescriptorType type);
    DescriptorSetBuilder& set_stage_flags(vk::ShaderStageFlags stage_flags);
    DescriptorSetBuilder& set_buffer(vk::Buffer& buffer_, size_t offset_, size_t range_);
    DescriptorSetBuilder& set_layout_out(vk::DescriptorSetLayout& layout_out);

    ManagedResource<vk::DescriptorSet> build();

private:
    VulkanState& vulkan;

    vk::DescriptorType descriptor_type;
    vk::ShaderStageFlags stage_flags;
    vk::Buffer* buffer;
    size_t offset;
    size_t range;
    vk::DescriptorSetLayout* layout_out_ptr;
};
}
