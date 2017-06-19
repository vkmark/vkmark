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

#include <functional>

template<typename T>
struct ManagedResource
{
    using Destructor = std::function<void(T&)>;

    ManagedResource() = default;

    ManagedResource(T&& raw, Destructor&& destructor)
        : raw{raw},
          destructor{std::move(destructor)}
    {
    }

    ManagedResource(ManagedResource&& rhs)
        : raw{std::move(rhs)},
          destructor{std::move(rhs.destructor)}
    {
        rhs.raw = T{};
        rhs.destructor = [](T&){};
    }

    ~ManagedResource()
    {
        destructor(raw);
    }

    ManagedResource& operator=(ManagedResource&& rhs)
    {
        destructor(raw);

        raw = std::move(rhs.raw);
        destructor = std::move(rhs.destructor);
        rhs.raw = T{};
        rhs.destructor = [](T&){};

        return *this;
    }

    ManagedResource& operator=(ManagedResource& rhs) = delete;

    operator T const& () const { return raw; }
    operator T&() { return raw; }

    T steal()
    {
        auto ret = raw;
        raw = T{};
        destructor = [](T&){};
        return ret;
    }

    T raw = T{};
    std::function<void(T&)> destructor = [](T&){};
};
