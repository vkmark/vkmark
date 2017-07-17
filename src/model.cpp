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

#include "model.h"
#include "util.h"
#include "mesh.h"

#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>

namespace
{

unsigned int const post_process_flags =
    aiProcess_Triangulate |
    aiProcess_SortByPType |
    aiProcess_GenNormals |
    aiProcess_JoinIdenticalVertices;

}

ModelAttribMap::ModelAttribMap()
    : position{-1}, color{-1}, normal{-1}
{
}

ModelAttribMap& ModelAttribMap::with_position(vk::Format format)
{
    position = formats.size();
    formats.push_back(format);
    return *this;
}

ModelAttribMap& ModelAttribMap::with_color(vk::Format format)
{
    color = formats.size();
    formats.push_back(format);
    return *this;
}

ModelAttribMap& ModelAttribMap::with_normal(vk::Format format)
{
    normal = formats.size();
    formats.push_back(format);
    return *this;
}

ModelAttribMap& ModelAttribMap::with_other(vk::Format format)
{
    formats.push_back(format);
    return *this;
}

Model::Model(std::string const& model_file)
{
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE,
                                aiPrimitiveType_LINE | aiPrimitiveType_POINT);

    auto const path = Util::get_data_file_path("models/" + model_file);
    if (!importer.ReadFile(path.c_str(), post_process_flags))
        throw std::runtime_error{"Failed to parse model file " + model_file};
}

Model::Model(std::string const& model_str, std::string const& model_type)
{
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE,
                                aiPrimitiveType_LINE | aiPrimitiveType_POINT);

    if (!importer.ReadFileFromMemory(model_str.data(), model_str.size(),
                                     post_process_flags, model_type.c_str()))
    {
        throw std::runtime_error{"Failed to parse model string of type " + model_type};
    }
}

Model::~Model() = default;

std::unique_ptr<Mesh> Model::to_mesh(ModelAttribMap const& map)
{
    auto mesh = std::make_unique<Mesh>(map.formats);

    auto const scene = importer.GetScene();

    for (auto m = 0u; m < scene->mNumMeshes; ++m)
    {
        auto const aimesh = scene->mMeshes[m];
        auto const colors = aimesh->mColors[0];

        for (auto f = 0u; f < aimesh->mNumFaces; ++f)
        {
            auto const& face = aimesh->mFaces[f];

            for (auto i = 0u; i < face.mNumIndices; ++i)
            {
                auto const vindex = face.mIndices[i];
                auto const& vertex = aimesh->mVertices[vindex];
                auto const& normal = aimesh->mNormals[vindex];

                mesh->next_vertex();

                if (map.position >= 0)
                    mesh->set_attribute(map.position, {vertex.x, -vertex.y, vertex.z});

                if (map.normal >= 0)
                    mesh->set_attribute(map.normal, {normal.x, -normal.y, normal.z});

                if (map.color >= 0)
                {
                    if (colors)
                    {
                        auto const& color = colors[vindex];
                        mesh->set_attribute(map.color, {color.r, color.g, color.b});
                    }
                    else
                    {
                        mesh->set_attribute(map.color, {1, 1, 1});
                    }
                }
            }
        }
    }

    return mesh;
}
