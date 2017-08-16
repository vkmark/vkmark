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

#include "src/window_system.h"
#include "src/vulkan_image.h"
#include "src/vulkan_wsi.h"

class NullWindowSystem : public WindowSystem, public VulkanWSI
{
public:
    VulkanWSI& vulkan_wsi() override { return *this; }
    void init_vulkan(VulkanState&) override {}
    void deinit_vulkan() override {}

    VulkanImage next_vulkan_image() override { return {}; }
    void present_vulkan_image(VulkanImage const&) override {}
    std::vector<VulkanImage> vulkan_images() override { return {}; }

    bool should_quit() override { return false; }

    std::vector<char const*> vulkan_extensions() override { return {}; }
    bool is_physical_device_supported(vk::PhysicalDevice const&) override { return true; }
    std::vector<uint32_t> physical_device_queue_family_indices(
        vk::PhysicalDevice const&) override
    {
        return {};
    }
};
