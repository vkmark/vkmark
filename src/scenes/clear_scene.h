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

#include "scene.h"

#include <vulkan/vulkan.hpp>

class ClearScene : public Scene
{
public:
    ClearScene();

    void setup(VulkanState&, std::vector<VulkanImage> const&) override;
    void teardown() override;

    VulkanImage draw(VulkanImage const&) override;
    void update() override;

private:
    void prepare_command_buffer(VulkanImage const& image);

    VulkanState* vulkan;
    std::vector<vk::CommandBuffer> command_buffers;
    std::vector<vk::Fence> command_buffer_fences;
    vk::Semaphore submit_semaphore;
    vk::ClearColorValue clear_color;
    bool cycle;
};
