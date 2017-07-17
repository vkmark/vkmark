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
#include <glm/glm.hpp>

class VertexData;

class Mesh
{
public:
    Mesh(std::vector<vk::Format> const& formats);

    void set_interleave(bool interleave_);

    void next_vertex();
    size_t num_vertices() const;
    void set_attribute(size_t pos, float data);
    void set_attribute(size_t pos, glm::vec2 const& data);
    void set_attribute(size_t pos, glm::vec3 const& data);
    void set_attribute(size_t pos, glm::vec4 const& data);

    // Vulkan related
    std::vector<vk::VertexInputBindingDescription> binding_descriptions() const;
    std::vector<vk::VertexInputAttributeDescription> attribute_descriptions() const;

    size_t vertex_data_size() const;
    void copy_vertex_data_to(void* dst) const;
    std::vector<vk::DeviceSize> vertex_data_binding_offsets() const;

private:
    std::vector<vk::Format> const vk_formats;
    std::vector<size_t> const formats;
    size_t const vertex_num_floats;

    bool interleave;
    std::vector<std::vector<float>> vertices;
};
