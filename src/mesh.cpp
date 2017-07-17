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

#include "mesh.h"

#include <numeric>
#include <stdexcept>

namespace
{

std::vector<size_t> vk_formats_to_float_formats(
    std::vector<vk::Format> const& formats)
{
    std::vector<size_t> ret;

    for (auto const f : formats)
    {
        switch (f)
        {
            case vk::Format::eR32Sfloat:
                ret.push_back(1);
                break;
            case vk::Format::eR32G32Sfloat:
                ret.push_back(2);
                break;
            case vk::Format::eR32G32B32Sfloat:
                ret.push_back(3);
                break;
            case vk::Format::eR32G32B32A32Sfloat:
                ret.push_back(4);
                break;
            default:
                throw std::runtime_error{"Unsupported vertex format " + to_string(f)};
        };
    }

    return ret;
}

size_t calc_vertex_num_floats(std::vector<size_t> const& formats)
{
    return std::accumulate(formats.begin(), formats.end(), 0);
}

}

Mesh::Mesh(std::vector<vk::Format> const& vk_formats)
    : vk_formats{vk_formats},
      formats{vk_formats_to_float_formats(vk_formats)},
      vertex_num_floats{calc_vertex_num_floats(formats)},
      interleave{false}
{

}

void Mesh::set_interleave(bool interleave_)
{
    interleave = interleave_;
}

void Mesh::next_vertex()
{
    vertices.push_back(std::vector<float>(vertex_num_floats));
}

size_t Mesh::num_vertices() const
{
    return vertices.size();
}

void Mesh::set_attribute(size_t pos, float data)
{
    if (formats[pos] != 1)
        throw std::logic_error{"Trying to set vertex attribute with incorrectly sized data"};

    auto const offset = std::accumulate(formats.begin(), formats.begin() + pos, 0);
    auto& vertex = vertices.back();

    vertex[offset] = data;
}

void Mesh::set_attribute(size_t pos, glm::vec2 const& data)
{
    if (formats[pos] != 2)
        throw std::logic_error{"Trying to set vertex attribute with incorrectly sized data"};

    auto const offset = std::accumulate(formats.begin(), formats.begin() + pos, 0);
    auto& vertex = vertices.back();

    vertex[offset] = data.x;
    vertex[offset + 1] = data.y;
}

void Mesh::set_attribute(size_t pos, glm::vec3 const& data)
{
    if (formats[pos] != 3)
        throw std::logic_error{"Trying to set vertex attribute with incorrectly sized data"};

    auto const offset = std::accumulate(formats.begin(), formats.begin() + pos, 0);
    auto& vertex = vertices.back();

    vertex[offset] = data.x;
    vertex[offset + 1] = data.y;
    vertex[offset + 2] = data.z;
}

void Mesh::set_attribute(size_t pos, glm::vec4 const& data)
{
    if (formats[pos] != 4)
        throw std::logic_error{"Trying to set vertex attribute with incorrectly sized data"};

    auto const offset = std::accumulate(formats.begin(), formats.begin() + pos, 0);
    auto& vertex = vertices.back();

    vertex[offset] = data.x;
    vertex[offset + 1] = data.y;
    vertex[offset + 2] = data.z;
    vertex[offset + 3] = data.w;
}

std::vector<vk::VertexInputBindingDescription>
Mesh::binding_descriptions() const
{
    std::vector<vk::VertexInputBindingDescription> ret;

    if (interleave)
    {
        ret.push_back(
            vk::VertexInputBindingDescription{}
                .setBinding(0)
                .setStride(sizeof(float) * vertex_num_floats)
                .setInputRate(vk::VertexInputRate::eVertex));
    }
    else
    {
        int binding = 0;

        for (auto const& f : formats)
        {
            ret.push_back(
                vk::VertexInputBindingDescription{}
                    .setBinding(binding)
                    .setStride(sizeof(float) * f)
                    .setInputRate(vk::VertexInputRate::eVertex));

            ++binding;
        }
    }

    return ret;
}

std::vector<vk::VertexInputAttributeDescription>
Mesh::attribute_descriptions() const
{
    std::vector<vk::VertexInputAttributeDescription> ret;

    int binding = 0;
    int i = 0;

    for (auto const& vf : vk_formats)
    {
        auto const offset =
            interleave ?
            sizeof(float) * std::accumulate(formats.begin(), formats.begin() + i, 0) :
            0;

        ret.push_back(
            vk::VertexInputAttributeDescription{}
                .setBinding(binding)
                .setLocation(i)
                .setFormat(vf)
                .setOffset(offset));

        if (!interleave)
            ++binding;
        ++i;
    }

    return ret;
}

void Mesh::copy_vertex_data_to(void* dst) const
{
    auto const dst_c = static_cast<char*>(dst);

    if (interleave)
    {
        auto current = dst_c;

        for (auto const& vertex : vertices)
        {
            auto const nbytes = vertex.size() * sizeof(float);
            memcpy(current, vertex.data(), nbytes);
            current += nbytes;
        }
    }
    else
    {
        auto current = dst_c;

        for (size_t i = 0; i < formats.size(); ++i)
        {
            auto const offset = std::accumulate(formats.begin(), formats.begin() + i, 0);
            auto const nbytes = formats[i] * sizeof(float);

            for (auto const& vertex : vertices)
            {
                memcpy(current, &vertex[offset], nbytes);
                current += nbytes;
            }
        }
    }
}

std::vector<vk::DeviceSize> Mesh::vertex_data_binding_offsets() const
{
    std::vector<vk::DeviceSize> ret;

    if (interleave)
    {
        ret.push_back(0);
    }
    else
    {
        for (size_t i = 0; i < formats.size(); ++i)
        {
            auto attrib_offset = std::accumulate(formats.begin(), formats.begin() + i, 0);
            ret.push_back(attrib_offset * sizeof(float) * vertices.size());
        }
    }

    return ret;
}

size_t Mesh::vertex_data_size() const
{
    return vertices.size() * vertex_num_floats * sizeof(float);
}
