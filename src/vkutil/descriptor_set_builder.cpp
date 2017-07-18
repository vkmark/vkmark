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

#include "descriptor_set_builder.h"

#include "vulkan_state.h"

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
