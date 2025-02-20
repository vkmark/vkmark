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

#include "kms_window_system.h"
#include "managed_resource.h"

#include <xf86drmMode.h>

struct PropertyIds
{
    PropertyIds(int drm_fd,
                drmModeCrtcPtr crtc,
                drmModeConnectorPtr connector,
                drmModePlanePtr plane);

    struct
    {
        int mode_id;
        int active;
    } crtc;

    struct
    {
        int crtc_id;
    } connector;

    struct
    {
        int fb_id;
        int crtc_id;
        int src_x;
        int src_y;
        int src_w;
        int src_h;
        int crtc_x;
        int crtc_y;
        int crtc_w;
        int crtc_h;
    } plane;
};

class AtomicKMSWindowSystem : public KMSWindowSystem
{
public:
    static bool is_supported_on(std::string const& drm_device);

    AtomicKMSWindowSystem(std::string const& drm_device,
                          std::string const& tty,
                          vk::PresentModeKHR present_mode);

protected:
    void flip(uint32_t image_index) override;

    bool const supports_atomic;
    PropertyIds const property_ids;
};
