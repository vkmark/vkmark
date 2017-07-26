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

#include "shading_scene.h"

#include "mesh.h"
#include "model.h"
#include "util.h"
#include "vulkan_state.h"
#include "vulkan_image.h"
#include "vkutil/vkutil.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <cmath>

namespace
{

struct Uniforms
{
    glm::mat4 modelviewprojection;
    glm::mat4 normal;
    glm::vec4 material_diffuse;
    glm::mat4 modelview;
};

}

ShadingScene::ShadingScene() : Scene{"shading"}
{
    options_["shading"] =
        SceneOption("shading", "gouraud", "Which shading method to use",
                    "gouraud,blinn-phong-inf,phong,cel");
}

ShadingScene::~ShadingScene() = default;

void ShadingScene::setup(
    VulkanState& vulkan_,
    std::vector<VulkanImage> const& vulkan_images)
{
    Scene::setup(vulkan_, vulkan_images);

    vulkan = &vulkan_;
    extent = vulkan_images[0].extent;
    format = vulkan_images[0].format;
    depth_format = vk::Format::eD32Sfloat;
    aspect = static_cast<float>(extent.height) / extent.width;

    mesh = Model{"cat.3ds"}.to_mesh(
        ModelAttribMap{}
            .with_position(vk::Format::eR32G32B32Sfloat)
            .with_normal(vk::Format::eR32G32B32Sfloat));

    mesh->set_interleave(true);

    // Model projection
    auto const min_bound = mesh->min_attribute_bound(0);
    auto const max_bound = mesh->max_attribute_bound(0);
    auto const diameter = glm::length(max_bound - min_bound);
    auto const aspect = static_cast<float>(extent.width)/static_cast<float>(extent.height);
    center = (max_bound + min_bound) / 2.0f;
    radius = diameter / 2.0f;
    auto const fovy = 2.0f * atanf(radius / (2.0f + radius));
    projection = glm::perspective(fovy, aspect, 2.0f, 2.0f + diameter);

    setup_vertex_buffer();
    setup_uniform_buffer();
    setup_uniform_descriptor_set();
    setup_render_pass();
    setup_pipeline();
    setup_depth_image();
    setup_framebuffers(vulkan_images);
    setup_command_buffers();

    submit_semaphore = vulkan->device().createSemaphore(vk::SemaphoreCreateInfo());
    rotation = 0.0;
}

void ShadingScene::teardown()
{
    vulkan->device().waitIdle();

    vulkan->device().destroySemaphore(submit_semaphore);
    vulkan->device().unmapMemory(uniform_buffer_memory);
    vulkan->device().freeCommandBuffers(vulkan->command_pool(), command_buffers);
    framebuffers.clear();
    image_views.clear();
    depth_image_view = {};
    depth_image = {};
    pipeline = {};
    pipeline_layout = {};
    render_pass = {};
    descriptor_set = {};
    uniform_buffer = {};
    vertex_buffer = {};

    Scene::teardown();
}

VulkanImage ShadingScene::draw(VulkanImage const& image)
{
    update_uniforms();

    vk::PipelineStageFlags const mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    auto const submit_info = vk::SubmitInfo{}
        .setCommandBufferCount(1)
        .setPCommandBuffers(&command_buffers[image.index])
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&image.semaphore)
        .setPWaitDstStageMask(&mask)
        .setSignalSemaphoreCount(1)
        .setPSignalSemaphores(&submit_semaphore);

    vulkan->graphics_queue().submit(submit_info, {});

    return image.copy_with_semaphore(submit_semaphore);
}

void ShadingScene::update()
{
    auto const t = (Util::get_timestamp_us() - start_time) / 1000000.0f;

    rotation = 36.0f * t;

    Scene::update();
}

void ShadingScene::setup_vertex_buffer()
{
    vk::DeviceMemory staging_buffer_memory;
    vk::BufferUsageFlags staging_usage_flags =
        vk::BufferUsageFlagBits::eVertexBuffer |
        vk::BufferUsageFlagBits::eTransferSrc;

    auto staging_buffer = vkutil::BufferBuilder{*vulkan}
        .set_size(mesh->vertex_data_size())
        .set_usage(staging_usage_flags)
        .set_memory_properties(
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent)
        .set_memory_out(staging_buffer_memory)
        .build();

    auto const staging_buffer_map = vulkan->device().mapMemory(
        staging_buffer_memory, 0, mesh->vertex_data_size());
    mesh->copy_vertex_data_to(staging_buffer_map);
    vulkan->device().unmapMemory(staging_buffer_memory);

    vertex_buffer = vkutil::BufferBuilder{*vulkan}
        .set_size(mesh->vertex_data_size())
        .set_usage(
            vk::BufferUsageFlagBits::eVertexBuffer |
            vk::BufferUsageFlagBits::eTransferDst)
        .set_memory_properties(vk::MemoryPropertyFlagBits::eDeviceLocal)
        .build();

    vkutil::copy_buffer(*vulkan, staging_buffer, vertex_buffer, mesh->vertex_data_size());
}

void ShadingScene::setup_uniform_buffer()
{
    uniform_buffer = vkutil::BufferBuilder{*vulkan}
        .set_size(sizeof(Uniforms))
        .set_usage(vk::BufferUsageFlagBits::eUniformBuffer)
        .set_memory_properties(
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent)
        .set_memory_out(uniform_buffer_memory)
        .build();

    uniform_buffer_map = vulkan->device().mapMemory(
        uniform_buffer_memory, 0, sizeof(Uniforms));
}


void ShadingScene::setup_uniform_descriptor_set()
{
    descriptor_set = vkutil::DescriptorSetBuilder{*vulkan}
        .set_type(vk::DescriptorType::eUniformBuffer)
        .set_stage_flags(vk::ShaderStageFlagBits::eVertex |
                         vk::ShaderStageFlagBits::eFragment)
        .set_buffer(uniform_buffer, 0, sizeof(Uniforms))
        .set_layout_out(descriptor_set_layout)
        .build();
}

void ShadingScene::setup_render_pass()
{
    render_pass = vkutil::RenderPassBuilder(*vulkan)
        .set_format(format)
        .set_depth_format(depth_format)
        .build();
}

void ShadingScene::setup_pipeline()
{
    auto const pipeline_layout_create_info = vk::PipelineLayoutCreateInfo{}
        .setSetLayoutCount(1)
        .setPSetLayouts(&descriptor_set_layout);
    pipeline_layout = ManagedResource<vk::PipelineLayout>{
        vulkan->device().createPipelineLayout(pipeline_layout_create_info),
        [this] (auto const& pl) { vulkan->device().destroyPipelineLayout(pl); }};

    std::vector<char> vertex_shader;
    std::vector<char> fragment_shader;

    if (options_["shading"].value == "gouraud")
    {
        vertex_shader = Util::read_data_file("shaders/light-basic.vert.spv");
        fragment_shader = Util::read_data_file("shaders/light-basic.frag.spv");
    }
    else if (options_["shading"].value == "blinn-phong-inf")
    {
        vertex_shader = Util::read_data_file("shaders/light-advanced.vert.spv");
        fragment_shader = Util::read_data_file("shaders/light-advanced.frag.spv");
    }
    else if (options_["shading"].value == "phong")
    {
        vertex_shader = Util::read_data_file("shaders/light-phong.vert.spv");
        fragment_shader = Util::read_data_file("shaders/light-phong.frag.spv");
    }
    else if (options_["shading"].value == "cel")
    {
        vertex_shader = Util::read_data_file("shaders/light-phong.vert.spv");
        fragment_shader = Util::read_data_file("shaders/light-cel.frag.spv");
    }

    pipeline = vkutil::PipelineBuilder(*vulkan)
        .set_extent(extent)
        .set_layout(pipeline_layout)
        .set_render_pass(render_pass)
        .set_vertex_shader(vertex_shader)
        .set_fragment_shader(fragment_shader)
        .set_vertex_input(mesh->binding_descriptions(), mesh->attribute_descriptions())
        .set_depth_test(true)
        .build();
}

void ShadingScene::setup_depth_image()
{
    depth_image = vkutil::ImageBuilder{*vulkan}
        .set_extent(extent)
        .set_format(depth_format)
        .set_tiling(vk::ImageTiling::eOptimal)
        .set_usage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
        .set_memory_properties(vk::MemoryPropertyFlagBits::eDeviceLocal)
        .set_initial_layout(vk::ImageLayout::eUndefined)
        .build();

    vkutil::transition_image_layout(
        *vulkan,
        depth_image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal,
        vk::ImageAspectFlagBits::eDepth);
}

void ShadingScene::setup_framebuffers(std::vector<VulkanImage> const& vulkan_images)
{
    depth_image_view = vkutil::ImageViewBuilder{*vulkan}
        .set_image(depth_image)
        .set_format(depth_format)
        .set_aspect_mask(vk::ImageAspectFlagBits::eDepth)
        .build();

    for (auto const& vulkan_image : vulkan_images)
    {
        image_views.push_back(
            vkutil::ImageViewBuilder{*vulkan}
                .set_image(vulkan_image.image)
                .set_format(vulkan_image.format)
                .set_aspect_mask(vk::ImageAspectFlagBits::eColor)
                .build());
    }

    for (auto const& image_view : image_views)
    {
        framebuffers.push_back(
            vkutil::FramebufferBuilder{*vulkan}
                .set_render_pass(render_pass)
                .set_image_views({image_view, depth_image_view})
                .set_extent(extent)
                .build());
    }
}

void ShadingScene::setup_command_buffers()
{
    auto const command_buffer_allocate_info = vk::CommandBufferAllocateInfo{}
        .setCommandPool(vulkan->command_pool())
        .setCommandBufferCount(framebuffers.size())
        .setLevel(vk::CommandBufferLevel::ePrimary);

    command_buffers = vulkan->device().allocateCommandBuffers(command_buffer_allocate_info);
    auto const binding_offsets = mesh->vertex_data_binding_offsets();

    for (size_t i = 0; i < command_buffers.size(); ++i)
    {
        auto const begin_info = vk::CommandBufferBeginInfo{}
            .setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

        command_buffers[i].begin(begin_info);

        std::array<vk::ClearValue, 2> clear_values{{
            vk::ClearColorValue{std::array<float,4>{{0.0f, 0.0f, 0.0f, 1.0f}}},
            vk::ClearDepthStencilValue{1.0f, 0}}};

        auto const render_pass_begin_info = vk::RenderPassBeginInfo{}
            .setRenderPass(render_pass)
            .setFramebuffer(framebuffers[i])
            .setRenderArea({{0,0}, extent})
            .setClearValueCount(clear_values.size())
            .setPClearValues(clear_values.data());

        command_buffers[i].beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

        command_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
        command_buffers[i].bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, descriptor_set.raw, {});
        command_buffers[i].bindVertexBuffers(
            0,
            std::vector<vk::Buffer>{binding_offsets.size(), vertex_buffer.raw},
            binding_offsets
            );

        command_buffers[i].draw(mesh->num_vertices(), 1, 0, 0);

        command_buffers[i].endRenderPass();
        command_buffers[i].end();
    }
}

void ShadingScene::update_uniforms()
{
    Uniforms ubo;

    glm::mat4 modelview{1.0};
    modelview = glm::translate(modelview, glm::vec3{-center.x, -center.y, -(center.z + 2.0 + radius)});
    modelview = glm::rotate(modelview, glm::radians(rotation), {0.0f, 1.0f, 0.0f});

    ubo.modelviewprojection = projection * modelview;
    ubo.normal = glm::inverseTranspose(modelview);
    ubo.material_diffuse = glm::vec4{0.0f, 0.0f, 0.7f, 1.0f};
    ubo.modelview = modelview;

    memcpy(uniform_buffer_map, &ubo, sizeof(ubo));
}
