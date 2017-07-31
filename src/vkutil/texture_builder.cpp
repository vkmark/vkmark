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

#include "texture_builder.h"
#include "texture.h"
#include "util.h"
#include "vulkan_state.h"

#include "buffer_builder.h"
#include "copy_buffer.h"
#include "image_builder.h"
#include "image_view_builder.h"
#include "transition_image_layout.h"

namespace
{

void texture_setup_image(VulkanState& vulkan,
                         vkutil::Texture& texture,
                         Util::Image const& image)
{
    auto const texture_format = vk::Format::eR8G8B8A8Srgb;
    vk::DeviceMemory staging_buffer_memory;

    auto const image_extent = vk::Extent2D{
        static_cast<uint32_t>(image.width),
        static_cast<uint32_t>(image.height)};

    auto staging_buffer = vkutil::BufferBuilder{vulkan}
        .set_size(image.size)
        .set_usage(vk::BufferUsageFlagBits::eTransferSrc)
        .set_memory_properties(
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent)
        .set_memory_out(staging_buffer_memory)
        .build();

    auto const staging_buffer_map = vulkan.device().mapMemory(
        staging_buffer_memory, 0, image.size);
    memcpy(staging_buffer_map, image.data, image.size);
    vulkan.device().unmapMemory(staging_buffer_memory);

    texture.image = vkutil::ImageBuilder{vulkan}
        .set_extent(image_extent)
        .set_format(texture_format)
        .set_tiling(vk::ImageTiling::eOptimal)
        .set_usage(vk::ImageUsageFlagBits::eTransferDst |
                   vk::ImageUsageFlagBits::eSampled)
        .set_memory_properties(vk::MemoryPropertyFlagBits::eDeviceLocal)
        .set_initial_layout(vk::ImageLayout::ePreinitialized)
        .build();

    vkutil::transition_image_layout(
        vulkan,
        texture.image,
        vk::ImageLayout::ePreinitialized,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageAspectFlagBits::eColor);

    vkutil::copy_buffer_to_image(vulkan, staging_buffer, texture.image, image_extent);

    texture.image_view = vkutil::ImageViewBuilder{vulkan}
        .set_image(texture.image)
        .set_format(texture_format)
        .set_aspect_mask(vk::ImageAspectFlagBits::eColor)
        .build();
}

void texture_setup_sampler(VulkanState& vulkan,
                           vkutil::Texture& texture,
                           vk::Filter filter,
                           float anisotropy)
{
    auto const sampler_create_info = vk::SamplerCreateInfo{}
        .setMagFilter(filter)
        .setMinFilter(filter)
        .setAddressModeU(vk::SamplerAddressMode::eRepeat)
        .setAddressModeV(vk::SamplerAddressMode::eRepeat)
        .setAddressModeW(vk::SamplerAddressMode::eRepeat)
        .setAnisotropyEnable(anisotropy > 0.0f)
        .setMaxAnisotropy(anisotropy)
        .setUnnormalizedCoordinates(false)
        .setCompareEnable(false)
        .setMinLod(0.0f)
        .setMaxLod(0.25f)
        .setMipmapMode(vk::SamplerMipmapMode::eNearest);

    texture.sampler = ManagedResource<vk::Sampler>{
        vulkan.device().createSampler(sampler_create_info),
        [vptr=&vulkan] (auto const& s) { vptr->device().destroySampler(s); }};
}

}

vkutil::TextureBuilder::TextureBuilder(VulkanState& vulkan)
    : vulkan{vulkan}
{
}

vkutil::TextureBuilder& vkutil::TextureBuilder::set_file(std::string const& file_)
{
    file = file_;
    return *this;
}

vkutil::TextureBuilder& vkutil::TextureBuilder::set_filter(vk::Filter filter_)
{
    filter = filter_;
    return *this;
}

vkutil::TextureBuilder& vkutil::TextureBuilder::set_anisotropy(float a)
{
    anisotropy = a;
    return *this;
}

vkutil::Texture vkutil::TextureBuilder::build()
{
    Texture texture;

    auto const image = Util::read_image_file(file);

    texture_setup_image(vulkan, texture, image);
    texture_setup_sampler(vulkan, texture, filter, anisotropy);

    return texture;
}
