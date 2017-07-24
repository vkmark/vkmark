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

class ImageBuilder
{
public:
    ImageBuilder(VulkanState& vulkan);

    ImageBuilder& set_extent(vk::Extent2D extent);
    ImageBuilder& set_format(vk::Format format);
    ImageBuilder& set_tiling(vk::ImageTiling tiling);
    ImageBuilder& set_usage(vk::ImageUsageFlags usage);
    ImageBuilder& set_memory_properties(vk::MemoryPropertyFlags memory_properties);
    ImageBuilder& set_initial_layout(vk::ImageLayout initial_layout);

    ManagedResource<vk::Image> build();

private:
    VulkanState& vulkan;
    vk::Extent2D extent;
    vk::Format format;
    vk::ImageTiling tiling;
    vk::ImageUsageFlags usage;
    vk::MemoryPropertyFlags memory_properties;
    vk::ImageLayout initial_layout;
};

}

