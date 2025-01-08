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

#include "headless_native_system.h"

#include "vulkan_state.h"

HeadlessNativeSystem::HeadlessNativeSystem(vk::Extent2D vk_extent)
    : vk_extent{vk_extent}
{
}

std::vector<char const*> HeadlessNativeSystem::instance_extensions()
{
    return {VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME};
}

uint32_t HeadlessNativeSystem::get_presentation_queue_family_index(vk::PhysicalDevice const& pd)
{
    auto const queue_families = pd.getQueueFamilyProperties();

    for (auto i = 0u; i < queue_families.size(); ++i)
    {
        if (queue_families[i].queueCount > 0)
            return i;
    }

    return invalid_queue_family_index;
}

bool HeadlessNativeSystem::should_quit()
{
    return false;
}

vk::Extent2D HeadlessNativeSystem::get_vk_extent()
{
    return vk_extent;
}

ManagedResource<vk::SurfaceKHR> HeadlessNativeSystem::create_vk_surface(VulkanState& vulkan)
{
    return ManagedResource<vk::SurfaceKHR>{
        vulkan.instance().createHeadlessSurfaceEXT(vk::HeadlessSurfaceCreateInfoEXT{}),
        [vptr=&vulkan] (vk::SurfaceKHR& s) { vptr->instance().destroySurfaceKHR(s); }};
}
