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

#include "window_system_plugin.h"
#include "kms_window_system.h"
#include "atomic_kms_window_system.h"

#include "options.h"
#include "log.h"

#include <fcntl.h>
#include <unistd.h>
#include <xf86drm.h>

#define VKMARK_KMS_WINDOW_SYSTEM_PRIORITY 2

namespace
{

std::string const drm_device_opt{"kms-drm-device"};
std::string const atomic_opt{"kms-atomic"};

std::string get_drm_device_option(Options const& options)
{
    auto const& winsys_options = options.window_system_options;
    std::string drm_device;

    for (auto const& opt : winsys_options)
    {
        if (opt.name == drm_device_opt)
            drm_device = opt.value;
    }

    return drm_device;
}

}

void vkmark_window_system_load_options(Options& options)
{
    options.add_window_system_help(
        "KMS window system options (pass in --winsys-options)\n"
        "  kms-drm-device=DEV          The drm device to use (default: /dev/dri/card0)\n"
        "  kms-atomic=auto|yes|no      Whether to use atomic modesetting (default: auto)\n"
        );
}

int vkmark_window_system_probe(Options const& options)
{
    auto drm_device = get_drm_device_option(options);
    bool device_from_option = !drm_device.empty();
    int score = 0;

    if (!device_from_option)
        drm_device = "/dev/dri/card0";

    int const drm_fd = open(drm_device.c_str(), O_RDWR);
    if (drm_fd >= 0)
    {
        if (drmSetMaster(drm_fd) >= 0)
        {
            drmDropMaster(drm_fd);
            score = device_from_option ? VKMARK_WINDOW_SYSTEM_PROBE_GOOD :
                                         VKMARK_WINDOW_SYSTEM_PROBE_OK;
        }
        close(drm_fd);
    }

    if (score)
        score += VKMARK_KMS_WINDOW_SYSTEM_PRIORITY;

    return score;
}

std::unique_ptr<WindowSystem> vkmark_window_system_create(Options const& options)
{
    auto const& winsys_options = options.window_system_options;
    std::string drm_device{"/dev/dri/card0"};
    std::string atomic{"auto"};

    for (auto const& opt : winsys_options)
    {
        if (opt.name == drm_device_opt)
        {
            drm_device = opt.value;
        }
        else if (opt.name == atomic_opt)
        {
            if (opt.value != "auto" && opt.value != "yes" && opt.value != "no")
            {
                Log::info("KMSWindowSystemPlugin: Ignoring unknown value '%s'"
                          " for window system option '%s'\n",
                          opt.value.c_str(), opt.name.c_str());
            }
            else
            {
                atomic = opt.value;
            }
        }
        else
        {
            Log::info("KMSWindowSystemPlugin: Ignoring unknown window system option '%s'\n",
                      opt.name.c_str());
        }
    }

    if (atomic == "yes" ||
        (atomic == "auto" && AtomicKMSWindowSystem::is_supported_on(drm_device)))
    {
        Log::debug("KMSWindowSystemPlugin: Using atomic modesetting\n");
        return std::make_unique<AtomicKMSWindowSystem>(drm_device);
    }
    else
    {
        Log::debug("KMSWindowSystemPlugin: Using legacy modesetting\n");
        return std::make_unique<KMSWindowSystem>(drm_device);
    }
}
