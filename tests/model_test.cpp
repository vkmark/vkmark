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

#include "src/model.h"
#include "src/mesh.h"

#include "catch.hpp"

using namespace Catch::Matchers;

namespace
{

std::vector<float> mesh_vertex_data(Mesh& mesh)
{
    std::vector<float> ret(mesh.vertex_data_size() / sizeof(float));

    mesh.set_interleave(true);
    mesh.copy_vertex_data_to(ret.data());

    return ret;
}

}

SCENARIO("model to mesh", "")
{
    GIVEN("A model with positions")
    {
        std::string const model_str =
            "v  0  1  1\n"
            "v -1 -1  1\n"
            "v  1 -1  1\n"
            "f  1  2  3\n";

        Model model{model_str, "obj"};

        WHEN("converting to a mesh")
        {
            THEN("positions are converted to the vulkan coordinate system")
            {
                auto const mesh = model.to_mesh(
                    ModelAttribMap{}.with_position(vk::Format::eR32G32B32Sfloat));

                auto const vertex_data = mesh_vertex_data(*mesh);
                REQUIRE_THAT(vertex_data, Equals(
                    std::vector<float>{
                         0, -1,  1,
                        -1,  1,  1,
                         1,  1,  1}));
            }

            THEN("normals are automatically calculated")
            {
                auto const mesh = model.to_mesh(
                    ModelAttribMap{}.with_normal(vk::Format::eR32G32B32Sfloat));

                auto const vertex_data = mesh_vertex_data(*mesh);
                REQUIRE_THAT(vertex_data, Equals(
                    std::vector<float>{
                         0,  0,  1,
                         0,  0,  1,
                         0,  0,  1}));
            }
        }
    }

    GIVEN("A model with rectangle faces")
    {
        std::string const model_str =
            "ply\n"
            "format ascii 1.0\n"
            "element vertex 4\n"
            "property float x\n"
            "property float y\n"
            "property float z\n"
            "element face 1\n"
            "property list uchar int vertex_index\n"
            "end_header\n"
            "-1.0 +1.0 +1.0\n"
            "-1.0 -1.0 +1.0\n"
            "+1.0 -1.0 +1.0\n"
            "+1.0 +1.0 +1.0\n"
            "4 0 1 2 3\n";

        Model model{model_str, "ply"};

        WHEN("converting to a mesh")
        {
            auto const mesh = model.to_mesh(
                ModelAttribMap{}.with_position(vk::Format::eR32G32B32Sfloat));

            THEN("faces are triangulated")
            {
                auto const vertex_data = mesh_vertex_data(*mesh);
                REQUIRE_THAT(vertex_data, Equals(
                    std::vector<float>{
                        -1, -1,  1,
                        -1,  1,  1,
                         1,  1,  1,
                        -1, -1,  1,
                         1,  1,  1,
                         1, -1,  1}));

            }
        }
    }

    GIVEN("A model with normals")
    {
        std::string const model_str =
            "v  0  1  1\n"
            "v -1 -1  1\n"
            "v  1 -1  1\n"
            "vn -1  1  1\n"
            "vn  1  0 -1\n"
            "vn  0 -1  0\n"
            "f 1//1 2//2 3//3\n";

        Model model{model_str, "obj"};

        WHEN("converting to a mesh")
        {
            auto const mesh = model.to_mesh(
                ModelAttribMap{}.with_normal(vk::Format::eR32G32B32Sfloat));

            THEN("the normals from the model are converted to the vulkan coordinate system")
            {
                auto const vertex_data = mesh_vertex_data(*mesh);
                REQUIRE_THAT(vertex_data, Equals(
                    std::vector<float>{
                        -1, -1,  1,
                         1,  0, -1,
                         0, +1,  0}));
            }
        }
    }

    GIVEN("A model with colors")
    {
        std::string const model_str =
            "ply\n"
            "format ascii 1.0\n"
            "element vertex 3\n"
            "property float x\n"
            "property float y\n"
            "property float z\n"
            "property float red\n"
            "property float green\n"
            "property float blue\n"
            "element face 1\n"
            "property list uchar int vertex_index\n"
            "end_header\n"
            "-1.0 +1.0 +1.0 1 0 0\n"
            "-1.0 -1.0 +1.0 0 1 0\n"
            "+1.0 -1.0 +1.0 0 0 1\n"
            "3 0 1 2\n";

        Model model{model_str, "ply"};

        WHEN("converting to a mesh")
        {
            auto const mesh = model.to_mesh(
                ModelAttribMap{}
                    .with_color(vk::Format::eR32G32B32Sfloat));

            THEN("the colors are read")
            {
                auto const vertex_data = mesh_vertex_data(*mesh);
                REQUIRE_THAT(vertex_data, Equals(
                    std::vector<float>{
                        1, 0, 0,
                        0, 1, 0,
                        0, 0, 1}));
            }
        }
    }

    GIVEN("A model with multiple vertex attributes")
    {
        std::string const model_str =
            "ply\n"
            "format ascii 1.0\n"
            "element vertex 3\n"
            "property float x\n"
            "property float y\n"
            "property float z\n"
            "property float red\n"
            "property float green\n"
            "property float blue\n"
            "property float nx\n"
            "property float ny\n"
            "property float nz\n"
            "element face 1\n"
            "property list uchar int vertex_index\n"
            "end_header\n"
            "-1.0 +1.0 +1.0 1 0 0 -1  0 -1\n"
            "-1.0 -1.0 +1.0 0 1 0  1  1  0\n"
            "+1.0 -1.0 +1.0 0 0 1  0 -1  1\n"
            "3 0 1 2\n";

        Model model{model_str, "ply"};

        WHEN("converting to a mesh with a specific attribute order")
        {
            auto const mesh = model.to_mesh(
                ModelAttribMap{}
                    .with_color(vk::Format::eR32G32B32Sfloat)
                    .with_normal(vk::Format::eR32G32B32Sfloat)
                    .with_position(vk::Format::eR32G32B32Sfloat));

            THEN("the attributes are in the correct order")
            {
                auto const vertex_data = mesh_vertex_data(*mesh);
                REQUIRE_THAT(vertex_data, Equals(
                    std::vector<float>{
                         1,  0,  0, -1,  0, -1, -1, -1,  1,
                         0,  1,  0,  1, -1,  0, -1,  1,  1,
                         0,  0,  1,  0,  1,  1,  1,  1,  1}));
            }
        }
    }
}
