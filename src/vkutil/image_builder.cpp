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

#include "image_builder.h"
#include "find_matching_memory_type.h"

#include "vulkan_state.h"

vkutil::ImageBuilder::ImageBuilder(VulkanState& vulkan)
    : vulkan{vulkan},
      format{vk::Format::eUndefined},
      tiling{vk::ImageTiling::eOptimal},
      initial_layout{vk::ImageLayout::eUndefined}
{
}

vkutil::ImageBuilder& vkutil::ImageBuilder::set_extent(vk::Extent2D extent_)
{
    extent = extent_;
    return *this;
}

vkutil::ImageBuilder& vkutil::ImageBuilder::set_format(vk::Format format_)
{
    format = format_;
    return *this;
}

vkutil::ImageBuilder& vkutil::ImageBuilder::set_tiling(vk::ImageTiling tiling_)
{
    tiling = tiling_;
    return *this;
}

vkutil::ImageBuilder& vkutil::ImageBuilder::set_usage(vk::ImageUsageFlags usage_)
{
    usage = usage_;
    return *this;
}

vkutil::ImageBuilder& vkutil::ImageBuilder::set_memory_properties(
    vk::MemoryPropertyFlags memory_properties_)
{
    memory_properties = memory_properties_;
    return *this;
}

vkutil::ImageBuilder& vkutil::ImageBuilder::set_initial_layout(vk::ImageLayout initial_layout_)
{
    initial_layout = initial_layout_;
    return *this;
}

ManagedResource<vk::Image> vkutil::ImageBuilder::build()
{
    auto const image_create_info = vk::ImageCreateInfo{}
        .setImageType(vk::ImageType::e2D)
        .setExtent({extent.width, extent.height, 1})
        .setMipLevels(1)
        .setArrayLayers(1)
        .setFormat(format)
        .setTiling(tiling)
        .setInitialLayout(initial_layout)
        .setUsage(usage)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setSharingMode(vk::SharingMode::eExclusive);

    auto vk_image = ManagedResource<vk::Image>{
        vulkan.device().createImage(image_create_info),
        [vptr=&vulkan] (auto const& i) { vptr->device().destroyImage(i); }};

    auto const req = vulkan.device().getImageMemoryRequirements(vk_image);
    auto const memory_type_index = vkutil::find_matching_memory_type(
        vulkan, req, memory_properties);

    auto const memory_allocate_info = vk::MemoryAllocateInfo{}
        .setAllocationSize(req.size)
        .setMemoryTypeIndex(memory_type_index);

    auto vk_mem = ManagedResource<vk::DeviceMemory>{
        vulkan.device().allocateMemory(memory_allocate_info),
        [vptr=&vulkan] (auto const& m) { vptr->device().freeMemory(m); }};

    vulkan.device().bindImageMemory(vk_image, vk_mem, 0);

    return ManagedResource<vk::Image>{
        vk_image.steal(),
        [vptr=&vulkan, mem=vk_mem.steal()]
        (auto const& i)
        {
            vptr->device().freeMemory(mem);
            vptr->device().destroyImage(i);
        }};
}

