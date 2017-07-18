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

#include <vector>

class VulkanState;

namespace vkutil
{

class FramebufferBuilder
{
public:
    FramebufferBuilder(VulkanState& vulkan);

    FramebufferBuilder& set_render_pass(vk::RenderPass render_pass);
    FramebufferBuilder& set_image_views(std::vector<vk::ImageView> const& image_view);
    FramebufferBuilder& set_extent(vk::Extent2D extent);

    ManagedResource<vk::Framebuffer> build();

private:
    VulkanState& vulkan;
    vk::RenderPass render_pass;
    std::vector<vk::ImageView> image_views;
    vk::Extent2D extent;
};

}
