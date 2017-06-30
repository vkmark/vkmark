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

#include "window_system.h"
#include "managed_resource.h"

#include <vulkan/vulkan.hpp>

#include <xf86drmMode.h>
#include <gbm.h>
#include <linux/vt.h>

class VTState
{
public:
    VTState();
    ~VTState();
    void restore() const;

private:
    ManagedResource<int> const vt_fd;
    vt_mode prev_vt_mode;
};

class KMSWindowSystem : public WindowSystem
{
public:
    KMSWindowSystem(
        vk::PresentModeKHR present_mode,
        vk::Format pixel_format,
        std::string const& drm_device);
    ~KMSWindowSystem();

    std::vector<char const*> vulkan_extensions() override;
    void init_vulkan(VulkanState& vulkan) override;
    void deinit_vulkan() override;

    VulkanImage next_vulkan_image() override;
    void present_vulkan_image(VulkanImage const&) override;
    std::vector<VulkanImage> vulkan_images() override;

    bool should_quit() override;

private:

    void create_gbm_bos();
    void create_drm_fbs();
    void create_vk_images();

    vk::PresentModeKHR const vk_present_mode;
    vk::Format const vk_pixel_format;

    ManagedResource<int> const drm_fd;
    ManagedResource<drmModeResPtr> const drm_resources;
    ManagedResource<drmModeConnectorPtr> const drm_connector;
    ManagedResource<drmModeCrtcPtr> const drm_prev_crtc;
    ManagedResource<drmModeCrtcPtr> const drm_crtc;
    ManagedResource<gbm_device*> const gbm;
    vk::Extent2D const vk_extent;
    VTState const vt_state;

    VulkanState* vulkan;
    vk::Format vk_image_format;
    std::vector<ManagedResource<gbm_bo*>> gbm_bos;
    std::vector<ManagedResource<uint32_t>> drm_fbs;
    std::vector<ManagedResource<vk::Image>> vk_images;
    uint32_t current_image_index;
    bool has_crtc_been_set;
};
