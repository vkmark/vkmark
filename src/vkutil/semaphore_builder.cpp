/*
 * Copyright © 2018 Collabora Ltd.
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

#include "semaphore_builder.h"

#include "vulkan_state.h"

vkutil::SemaphoreBuilder::SemaphoreBuilder(VulkanState& vulkan)
    : vulkan{vulkan}
{
}

ManagedResource<vk::Semaphore> vkutil::SemaphoreBuilder::build()
{
    auto const export_info = vk::ExportSemaphoreCreateInfo{}
        .setHandleTypes(vk::ExternalSemaphoreHandleTypeFlagBits::eSyncFdKHR);
    auto create_info = vk::SemaphoreCreateInfo{}
        .setPNext(&export_info);

    return ManagedResource<vk::Semaphore>{
        vulkan.device().createSemaphore(create_info),
        [vptr=&vulkan] (auto const& s) { vptr->device().destroySemaphore(s); }};
}
