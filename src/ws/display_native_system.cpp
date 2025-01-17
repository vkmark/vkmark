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

#include "display_native_system.h"

#include "vulkan_state.h"

vk::DisplaySurfaceCreateInfoKHR DisplayNativeSystem::get_display_surface_create_info(
    vk::PhysicalDevice const& physical_device)
{
    auto const displays = physical_device.getDisplayPropertiesKHR();
    if (displays.empty())
        throw std::runtime_error{"Failed to find any Vulkan displays"};
    // TODO: Allow the user to specify the display they want to use.
    auto const display = displays[0].display;

    auto const planes = physical_device.getDisplayPlanePropertiesKHR();
    if (planes.empty())
        throw std::runtime_error{"Failed to find any Vulkan planes"};

    auto const modes = physical_device.getDisplayModePropertiesKHR(display);
    if (modes.empty())
        throw std::runtime_error{"Failed to find any Vulkan modes"};

    uint32_t plane_index;

    for (plane_index = 0; plane_index < planes.size(); ++plane_index)
    {
        // We want to target the "primary" plane, but since there is no
        // such concept in Vulkan use stack index 0 as a proxy for this.
        if (planes[plane_index].currentStackIndex != 0)
            continue;
        // If a plane is already associated with our display, use it
        if (planes[plane_index].currentDisplay == display)
            break;
        // Otherwise check to see if this plane can be associated with our display
        auto supported_displays =
            physical_device.getDisplayPlaneSupportedDisplaysKHR(plane_index);
        if (std::find(supported_displays.begin(), supported_displays.end(), display) !=
            supported_displays.end())
        {
            break;
        }
    }

    if (plane_index == planes.size())
        throw std::runtime_error{"Failed to find any planes supported by display"};

    // Find the mode with the largest refresh rate that matches the physical
    // resolution of the displya.
    vk::DisplayModePropertiesKHR mode;
    for (auto& m : modes)
    {
        if (m.parameters.visibleRegion == displays[0].physicalResolution &&
            (!mode.displayMode ||
             m.parameters.refreshRate > mode.parameters.refreshRate))
        {
            mode = m;
        }
    }

    if (!mode.displayMode)
        throw std::runtime_error{"Failed to find any modes matching the physical resolution"};

    return vk::DisplaySurfaceCreateInfoKHR{}
        .setPlaneIndex(plane_index)
        .setPlaneStackIndex(0)
        .setDisplayMode(mode.displayMode)
        .setImageExtent(displays[0].physicalResolution);
}

VulkanWSI::Extensions DisplayNativeSystem::required_extensions()
{
    return {{VK_KHR_SURFACE_EXTENSION_NAME,
             VK_KHR_DISPLAY_EXTENSION_NAME},
            {}};
}

uint32_t DisplayNativeSystem::get_presentation_queue_family_index(vk::PhysicalDevice const& pd)
{
    auto const queue_families = pd.getQueueFamilyProperties();

    for (auto i = 0u; i < queue_families.size(); ++i)
    {
        if (queue_families[i].queueCount > 0)
            return i;
    }

    return invalid_queue_family_index;
}

bool DisplayNativeSystem::should_quit()
{
    return false;
}

vk::Extent2D DisplayNativeSystem::get_vk_extent()
{
    return vk_extent;
}

ManagedResource<vk::SurfaceKHR> DisplayNativeSystem::create_vk_surface(VulkanState& vulkan)
{
    auto const create_info = get_display_surface_create_info(vulkan.physical_device());
    vk_extent = create_info.imageExtent;

    return ManagedResource<vk::SurfaceKHR>{
        vulkan.instance().createDisplayPlaneSurfaceKHR(create_info),
        [vptr=&vulkan] (vk::SurfaceKHR& s) { vptr->instance().destroySurfaceKHR(s); }};
}
