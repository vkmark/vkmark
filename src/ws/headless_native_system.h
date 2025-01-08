/*
 * Copyright © 2025 Collabora Ltd.
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

#define VK_USE_PLATFORM_HEADLESS_KHR
#include "native_system.h"

class HeadlessNativeSystem : public NativeSystem
{
public:
    HeadlessNativeSystem(vk::Extent2D vk_extent);

    std::vector<char const*> instance_extensions() override;
    uint32_t get_presentation_queue_family_index(vk::PhysicalDevice const& pd) override;
    bool should_quit() override;
    vk::Extent2D get_vk_extent() override;
    ManagedResource<vk::SurfaceKHR> create_vk_surface(VulkanState& vulkan) override;

private:
    vk::Extent2D vk_extent;
};
