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

#include <memory>

#include <assimp/Importer.hpp>
#include <vulkan/vulkan.hpp>

class Mesh;

class ModelAttribMap
{
public:
    ModelAttribMap();

    ModelAttribMap& with_position(vk::Format format);
    ModelAttribMap& with_color(vk::Format format);
    ModelAttribMap& with_normal(vk::Format format);
    ModelAttribMap& with_other(vk::Format format);

    std::vector<vk::Format> formats;
    ssize_t position;
    ssize_t color;
    ssize_t normal;
};

class Model
{
public:
    Model(std::string const& model_file);
    Model(std::string const& model_str, std::string const& model_type);
    ~Model();

    std::unique_ptr<Mesh> to_mesh(ModelAttribMap const& map);

private:
    Assimp::Importer importer;
};
