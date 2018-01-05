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

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

class Mesh;

class ShadingScene : public Scene
{
public:
    ShadingScene();
    ~ShadingScene();

    void setup(VulkanState&, std::vector<VulkanImage> const&) override;
    void teardown() override;

    VulkanImage draw(VulkanImage const&) override;
    void update() override;

private:
    void setup_vertex_buffer();
    void setup_uniform_buffer();
    void setup_uniform_descriptor_set();
    void setup_render_pass();
    void setup_pipeline();
    void setup_depth_image();
    void setup_framebuffers(std::vector<VulkanImage> const&);
    void setup_command_buffers();
    void update_uniforms();

    VulkanState* vulkan;
    vk::Extent2D extent;
    vk::Format format;
    vk::Format depth_format;
    float aspect;
    glm::mat4 projection;
    glm::vec3 center;
    float radius;

    std::unique_ptr<Mesh> mesh;

    ManagedResource<vk::Buffer> vertex_buffer;
    ManagedResource<vk::Buffer> uniform_buffer;
    ManagedResource<void*> uniform_buffer_map;
    ManagedResource<vk::DescriptorSet> descriptor_set;
    ManagedResource<vk::RenderPass> render_pass;
    ManagedResource<vk::PipelineLayout> pipeline_layout;
    ManagedResource<vk::Pipeline> pipeline;
    ManagedResource<vk::Image> depth_image;
    ManagedResource<vk::ImageView> depth_image_view;
    std::vector<ManagedResource<vk::ImageView>> image_views;
    std::vector<ManagedResource<vk::Framebuffer>> framebuffers;
    std::vector<vk::CommandBuffer> command_buffers;
    ManagedResource<vk::Semaphore> submit_semaphore;

    vk::DeviceMemory uniform_buffer_memory;
    vk::DescriptorSetLayout descriptor_set_layout;

    float rotation;
};
