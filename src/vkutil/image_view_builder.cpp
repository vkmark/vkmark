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

#include "image_view_builder.h"

#include "vulkan_state.h"

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
