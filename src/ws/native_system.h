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

#include "managed_resource.h"

#include <vulkan/vulkan.hpp>

class VulkanState;

class NativeSystem
{
public:
    virtual ~NativeSystem() = default;

    virtual std::vector<char const*> vulkan_extensions() = 0;
    virtual bool should_quit() = 0;
    virtual vk::Extent2D get_vk_extent() = 0;
    virtual ManagedResource<vk::SurfaceKHR> create_vk_surface(VulkanState& vulkan) = 0;

protected:
    NativeSystem() = default;
    NativeSystem(NativeSystem const&) = delete;
    NativeSystem& operator=(NativeSystem const&) = delete;
};
