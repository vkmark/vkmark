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

vkutil::DescriptorSetBuilder::Info::Info()
    : descriptor_type{vk::DescriptorType::eSampler},
      buffer{nullptr},
      offset{0},
      range{0},
      image_view{nullptr},
      sampler{nullptr}
{
}

vkutil::DescriptorSetBuilder::DescriptorSetBuilder(VulkanState& vulkan)
    : vulkan{vulkan},
      info{1},
      layout_out_ptr{nullptr}
{
}

vkutil::DescriptorSetBuilder& vkutil::DescriptorSetBuilder::set_type(
    vk::DescriptorType type)
{
    info.back().descriptor_type = type;
    return *this;
}

vkutil::DescriptorSetBuilder& vkutil::DescriptorSetBuilder::set_stage_flags(
    vk::ShaderStageFlags stage_flags_)
{
    info.back().stage_flags = stage_flags_;
    return *this;
}

vkutil::DescriptorSetBuilder& vkutil::DescriptorSetBuilder::set_buffer(
    vk::Buffer& buffer_, size_t offset_, size_t range_)
{
    info.back().buffer = &buffer_;
    info.back().offset = offset_;
    info.back().range = range_;
    return *this;
}

vkutil::DescriptorSetBuilder& vkutil::DescriptorSetBuilder::set_image_view(
    vk::ImageView& image_view, vk::Sampler& sampler)
{
    info.back().image_view = &image_view;
    info.back().sampler = &sampler;
    return *this;
}

vkutil::DescriptorSetBuilder& vkutil::DescriptorSetBuilder::set_layout_out(
    vk::DescriptorSetLayout& layout_out)
{
    layout_out_ptr = &layout_out;
    return *this;
}

vkutil::DescriptorSetBuilder& vkutil::DescriptorSetBuilder::next_binding()
{
    info.push_back({});
    return *this;
}

ManagedResource<vk::DescriptorSet> vkutil::DescriptorSetBuilder::build()
{
    // Layout
    std::vector<vk::DescriptorSetLayoutBinding> bindings;

    for (auto i = 0u; i < info.size(); ++i)
    {
        bindings.push_back(
            vk::DescriptorSetLayoutBinding{}
                .setBinding(i)
                .setDescriptorType(info[i].descriptor_type)
                .setDescriptorCount(1)
                .setStageFlags(info[i].stage_flags));
    }

    auto const descriptor_set_layout_create_info = vk::DescriptorSetLayoutCreateInfo{}
        .setBindingCount(bindings.size())
        .setPBindings(bindings.data());

    auto descriptor_set_layout = ManagedResource<vk::DescriptorSetLayout>{
        vulkan.device().createDescriptorSetLayout(descriptor_set_layout_create_info),
        [vptr=&vulkan] (auto const& dsl) { vptr->device().destroyDescriptorSetLayout(dsl); }};

    // Descriptor pool and sets
    std::vector<vk::DescriptorPoolSize> pool_sizes;

    for (auto i = 0u; i < info.size(); ++i)
    {
        pool_sizes.push_back(vk::DescriptorPoolSize{}
            .setDescriptorCount(1)
            .setType(info[i].descriptor_type));
    }

    auto const descriptor_pool_create_info = vk::DescriptorPoolCreateInfo{}
        .setPoolSizeCount(pool_sizes.size())
        .setPPoolSizes(pool_sizes.data())
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
    std::vector<vk::WriteDescriptorSet> write_descriptor_sets;

    for (auto i = 0u; i < info.size(); ++i)
    {
        auto write_descriptor_set = vk::WriteDescriptorSet{}
            .setDstSet(descriptor_set[0])
            .setDstBinding(i)
            .setDstArrayElement(0)
            .setDescriptorType(info[i].descriptor_type)
            .setDescriptorCount(1);

        if (info[i].buffer)
        {
            auto const descriptor_buffer_info = vk::DescriptorBufferInfo{}
                .setBuffer(*info[i].buffer)
                .setOffset(info[i].offset)
                .setRange(info[i].range);

            write_descriptor_set.setPBufferInfo(&descriptor_buffer_info);
        }
        else if (info[i].image_view)
        {
            auto const descriptor_image_info = vk::DescriptorImageInfo{}
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setImageView(*info[i].image_view)
                .setSampler(*info[i].sampler);

            write_descriptor_set.setPImageInfo(&descriptor_image_info);
        }

        write_descriptor_sets.push_back(write_descriptor_set);
    }

    vulkan.device().updateDescriptorSets(write_descriptor_sets, {});

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
