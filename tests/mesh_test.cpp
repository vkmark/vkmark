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

#include "src/mesh.h"

#include "catch.hpp"

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <numeric>

using namespace Catch::Matchers;

SCENARIO("mesh object construction", "")
{
    GIVEN("Unsupported formats")
    {
        std::vector<vk::Format> const unsupported_formats{
            vk::Format::eR32G32B32Sint,
            vk::Format::eR64G64B64Sfloat};

        WHEN("constructing a mesh")
        {
            THEN("the construction throws")
            {
                for (auto const f : unsupported_formats)
                {
                    std::vector<vk::Format> formats{vk::Format::eR32G32B32A32Sfloat, f};
                    REQUIRE_THROWS(Mesh{formats});
                }
            }
        }
    }

    GIVEN("Supported formats")
    {
        std::vector<vk::Format> const supported_formats{
            vk::Format::eR32G32Sfloat,
            vk::Format::eR32G32B32Sfloat,
            vk::Format::eR32G32B32A32Sfloat};

        WHEN("constructing a mesh")
        {
            THEN("the construction succeeeds")
            {
                Mesh mesh{supported_formats};
            }
        }
    }
}

SCENARIO("mesh creation", "")
{
    std::vector<vk::Format> const formats{
        vk::Format::eR32Sfloat,
        vk::Format::eR32G32Sfloat,
        vk::Format::eR32G32B32Sfloat,
        vk::Format::eR32G32B32A32Sfloat};
    size_t const num_vertex_floats = 10;

    Mesh mesh{formats};

    GIVEN("A mesh")
    {
        WHEN("setting vertex attribute with incorrectly sized data")
        {
            mesh.next_vertex();

            THEN("setting the attributes throws")
            {
                REQUIRE_THROWS(mesh.set_attribute(0, glm::vec2{1, 2}));
                REQUIRE_THROWS(mesh.set_attribute(1, glm::vec3{1, 2, 3}));
                REQUIRE_THROWS(mesh.set_attribute(2, glm::vec2{1, 2}));
                REQUIRE_THROWS(mesh.set_attribute(3, glm::vec3{1, 2, 3}));
            }
        }

        WHEN("setting vertex attributes with correctly sized data")
        {
            mesh.next_vertex();

            THEN("setting the attributes works")
            {
                mesh.set_attribute(0, 0);
                mesh.set_attribute(1, glm::vec2{1, 2});
                mesh.set_attribute(2, glm::vec3{1, 2, 3});
                mesh.set_attribute(3, glm::vec4{1, 2, 3, 4});
            }
        }
    }

    GIVEN("A mesh with vertices")
    {
        for (int i = 0; i < 5; ++i)
        {
            auto const v = i * num_vertex_floats;
            mesh.next_vertex();
            mesh.set_attribute(0, v);
            mesh.set_attribute(1, glm::vec2{v + 1, v + 2});
            mesh.set_attribute(2, glm::vec3{v + 3, v + 4, v + 5});
            mesh.set_attribute(3, glm::vec4{v + 6, v + 7, v + 8, v + 9});
        }

        WHEN("interleave is true")
        {
            mesh.set_interleave(true);

            THEN("the copied vertex data is interleaved")
            {
                std::vector<float> data(mesh.vertex_data_size() / sizeof(float));
                mesh.copy_vertex_data_to(data.data());

                std::vector<float> expected(num_vertex_floats * 5);
                std::iota(expected.begin(), expected.end(), 0);

                REQUIRE_THAT(data, Equals(expected));
            }

            THEN("binding descriptions are interleaved")
            {
                auto const binding_descs = mesh.binding_descriptions();

                REQUIRE(binding_descs.size() == 1);
                REQUIRE(binding_descs[0].binding == 0);
                REQUIRE(binding_descs[0].stride == num_vertex_floats * sizeof(float));
                REQUIRE(binding_descs[0].inputRate == vk::VertexInputRate::eVertex);
            }

            THEN("attribute descriptions are interleaved")
            {
                auto const attrib_descs = mesh.attribute_descriptions();

                REQUIRE(attrib_descs.size() == formats.size());
                for (size_t i = 0; i < attrib_descs.size(); ++i)
                {
                    REQUIRE(attrib_descs[i].binding == 0);
                    REQUIRE(attrib_descs[i].location == i);
                }
                REQUIRE(attrib_descs[0].offset == 0 * sizeof(float));
                REQUIRE(attrib_descs[1].offset == 1 * sizeof(float));
                REQUIRE(attrib_descs[2].offset == 3 * sizeof(float));
                REQUIRE(attrib_descs[3].offset == 6 * sizeof(float));
            }
        }

        WHEN("interleave is false")
        {
            mesh.set_interleave(false);

            THEN("the copied vertex data is not interleaved")
            {
                std::vector<float> data(mesh.vertex_data_size() / sizeof(float));
                mesh.copy_vertex_data_to(data.data());

                std::vector<float> expected;
                for (int i = 0; i < 5; ++i)
                {
                    expected.push_back(i * num_vertex_floats);
                }
                for (int i = 0; i < 5; ++i)
                {
                    for (int j = 0; j < 2; ++j)
                        expected.push_back(i * num_vertex_floats + 1 + j);
                }
                for (int i = 0; i < 5; ++i)
                {
                    for (int j = 0; j < 3; ++j)
                        expected.push_back(i * num_vertex_floats + 3 + j);
                }
                for (int i = 0; i < 5; ++i)
                {
                    for (int j = 0; j < 4; ++j)
                        expected.push_back(i * num_vertex_floats + 6 + j);
                }

                REQUIRE_THAT(data, Equals(expected));
            }

            THEN("binding descriptions are not interleaved")
            {
                auto const binding_descs = mesh.binding_descriptions();

                REQUIRE(binding_descs.size() == formats.size());
                for (size_t i = 0; i < binding_descs.size(); ++i)
                {
                    REQUIRE(binding_descs[i].binding == i);
                    // format[i] corresponds to a float vector of size i + 1
                    REQUIRE(binding_descs[i].stride == (i + 1) * sizeof(float));
                    REQUIRE(binding_descs[i].inputRate == vk::VertexInputRate::eVertex);
                }
            }

            THEN("attribute descriptions are not interleaved")
            {
                auto const attrib_descs = mesh.attribute_descriptions();

                REQUIRE(attrib_descs.size() == formats.size());
                for (size_t i = 0; i < attrib_descs.size(); ++i)
                {
                    REQUIRE(attrib_descs[i].binding == i);
                    REQUIRE(attrib_descs[i].location == i);
                    REQUIRE(attrib_descs[i].offset == 0);
                }
            }
        }
    }
}
