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
#include "managed_resource.h"

#include <memory>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

class Mesh;

class DesktopScene : public Scene
{
public:
    DesktopScene();
    ~DesktopScene();

    void setup(VulkanState&, std::vector<VulkanImage> const&) override;
    void teardown() override;

    VulkanImage draw(VulkanImage const&) override;
    void update() override;

private:
    class RenderObject;

    void setup_vertex_buffer();
    void setup_render_pass();
    void setup_pipeline();
    void setup_framebuffers(std::vector<VulkanImage> const&);
    void setup_command_buffers();
    void update_uniforms(size_t index);

    VulkanState* vulkan;
    vk::Extent2D extent;
    vk::Format format;

    std::unique_ptr<Mesh> mesh;
    std::unique_ptr<RenderObject> background;
    std::vector<std::unique_ptr<RenderObject>> windows;

    ManagedResource<vk::Buffer> vertex_buffer;
    ManagedResource<vk::RenderPass> render_pass;
    ManagedResource<vk::PipelineLayout> pipeline_layout;
    ManagedResource<vk::Pipeline> pipeline_opaque;
    ManagedResource<vk::Pipeline> pipeline_blend;
    std::vector<ManagedResource<vk::ImageView>> image_views;
    std::vector<ManagedResource<vk::Framebuffer>> framebuffers;
    std::vector<vk::CommandBuffer> command_buffers;
    ManagedResource<vk::Semaphore> submit_semaphore;
};
