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

#include "framebuffer_builder.h"

#include "vulkan_state.h"

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
