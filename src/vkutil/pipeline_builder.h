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

#include <vulkan/vulkan.hpp>

#include "managed_resource.h"

class VulkanState;

namespace vkutil
{

class PipelineBuilder
{
public:
    PipelineBuilder(VulkanState& vulkan);

    PipelineBuilder& set_vertex_input(
        std::vector<vk::VertexInputBindingDescription> const& binding_descriptions,
        std::vector<vk::VertexInputAttributeDescription> const& attribute_descriptions);
    PipelineBuilder& set_vertex_shader(std::vector<char> const& spirv);
    PipelineBuilder& set_fragment_shader(std::vector<char> const& spirv);
    PipelineBuilder& set_depth_test(bool depth_test);
    PipelineBuilder& set_extent(vk::Extent2D extent);
    PipelineBuilder& set_layout(vk::PipelineLayout layout);
    PipelineBuilder& set_render_pass(vk::RenderPass render_pass);

    ManagedResource<vk::Pipeline> build();

private:
    VulkanState& vulkan;
    std::vector<vk::VertexInputBindingDescription> binding_descriptions;
    std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;
    std::vector<char> vertex_shader_spirv;
    std::vector<char> fragment_shader_spirv;
    bool depth_test;
    vk::Extent2D extent;
    vk::PipelineLayout layout;
    vk::RenderPass render_pass;
};

}
