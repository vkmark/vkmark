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

#include "effect2d_scene.h"

#include "mesh.h"
#include "util.h"
#include "vulkan_state.h"
#include "vulkan_image.h"
#include "vkutil/vkutil.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace
{

struct Uniforms
{
    float texture_step_x;
    float texture_step_y;
};

std::unique_ptr<Mesh> create_quad_mesh()
{
    auto mesh = std::make_unique<Mesh>(
        std::vector<vk::Format>{vk::Format::eR32G32Sfloat, vk::Format::eR32G32Sfloat});

    mesh->next_vertex();
    mesh->set_attribute(0, {-1,-1});
    mesh->set_attribute(1, {0,0});
    mesh->next_vertex();
    mesh->set_attribute(0, {-1,1});
    mesh->set_attribute(1, {0,1});
    mesh->next_vertex();
    mesh->set_attribute(0, {1,1});
    mesh->set_attribute(1, {1,1});

    mesh->next_vertex();
    mesh->set_attribute(0, {-1,-1});
    mesh->set_attribute(1, {0,0});
    mesh->next_vertex();
    mesh->set_attribute(0, {1,1});
    mesh->set_attribute(1, {1,1});
    mesh->next_vertex();
    mesh->set_attribute(0, {1,-1});
    mesh->set_attribute(1, {1,0});

    mesh->set_interleave(true);

    return mesh;
}

}

Effect2DScene::Effect2DScene() : Scene{"effect2d"}
{
    options_["kernel"] =
        SceneOption("kernel", "blur",
                    "the convolution kernel to use",
                    "blur,edge,none");
    options_["background-resolution"] =
        SceneOption("background-resolution", "800x600",
                    "the resolution of the background image",
                    "800x600,1920x1080");
}

Effect2DScene::~Effect2DScene() = default;

void Effect2DScene::setup(
    VulkanState& vulkan_,
    std::vector<VulkanImage> const& vulkan_images)
{
    Scene::setup(vulkan_, vulkan_images);

    vulkan = &vulkan_;
    extent = vulkan_images[0].extent;
    format = vulkan_images[0].format;

    mesh = create_quad_mesh();

    setup_vertex_buffer();
    setup_uniform_buffer();
    setup_texture();
    setup_shader_descriptor_set();
    setup_render_pass();
    setup_pipeline();
    setup_framebuffers(vulkan_images);
    setup_command_buffers();

    update_uniforms();

    submit_semaphore = vkutil::SemaphoreBuilder{*vulkan}.build();
}

void Effect2DScene::teardown()
{
    vulkan->device().waitIdle();

    submit_semaphore = {};
    vulkan->device().freeCommandBuffers(vulkan->command_pool(), command_buffers);
    framebuffers.clear();
    image_views.clear();
    pipeline = {};
    pipeline_layout = {};
    render_pass = {};
    descriptor_set = {};
    texture = {};
    uniform_buffer_map = {};
    uniform_buffer = {};
    vertex_buffer = {};

    Scene::teardown();
}

VulkanImage Effect2DScene::draw(VulkanImage const& image)
{
    vk::PipelineStageFlags const mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    auto const submit_info = vk::SubmitInfo{}
        .setCommandBufferCount(1)
        .setPCommandBuffers(&command_buffers[image.index])
        .setWaitSemaphoreCount(image.semaphore ? 1 : 0)
        .setPWaitSemaphores(&image.semaphore)
        .setPWaitDstStageMask(&mask)
        .setSignalSemaphoreCount(image.semaphore ? 1 : 0)
        .setPSignalSemaphores(&submit_semaphore.raw);

    vulkan->graphics_queue().submit(submit_info, {});

    return image.copy_with_semaphore(submit_semaphore);
}

void Effect2DScene::update()
{
    Scene::update();
}

void Effect2DScene::setup_vertex_buffer()
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

    {
        auto const staging_buffer_map = vkutil::map_memory(
            *vulkan, staging_buffer_memory, 0, mesh->vertex_data_size());
        mesh->copy_vertex_data_to(staging_buffer_map);
    }

    vertex_buffer = vkutil::BufferBuilder{*vulkan}
        .set_size(mesh->vertex_data_size())
        .set_usage(
            vk::BufferUsageFlagBits::eVertexBuffer |
            vk::BufferUsageFlagBits::eTransferDst)
        .set_memory_properties(vk::MemoryPropertyFlagBits::eDeviceLocal)
        .build();

    vkutil::copy_buffer(*vulkan, staging_buffer, vertex_buffer, mesh->vertex_data_size());
}

void Effect2DScene::setup_uniform_buffer()
{
    uniform_buffer = vkutil::BufferBuilder{*vulkan}
        .set_size(sizeof(Uniforms))
        .set_usage(vk::BufferUsageFlagBits::eUniformBuffer)
        .set_memory_properties(
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent)
        .set_memory_out(uniform_buffer_memory)
        .build();

    uniform_buffer_map = vkutil::map_memory(
        *vulkan, uniform_buffer_memory, 0, sizeof(Uniforms));
}

void Effect2DScene::setup_texture()
{
    auto const texture_file =
        "textures/desktop-background-" +
        options_["background-resolution"].value +
        ".png";

    texture = vkutil::TextureBuilder{*vulkan}
        .set_file(texture_file)
        .set_filter(vk::Filter::eNearest)
        .build();
}

void Effect2DScene::setup_shader_descriptor_set()
{
    descriptor_set = vkutil::DescriptorSetBuilder{*vulkan}
        .set_type(vk::DescriptorType::eUniformBuffer)
        .set_stage_flags(vk::ShaderStageFlagBits::eFragment)
        .set_buffer(uniform_buffer, 0, sizeof(Uniforms))
        .next_binding()
        .set_type(vk::DescriptorType::eCombinedImageSampler)
        .set_stage_flags(vk::ShaderStageFlagBits::eFragment)
        .set_image_view(texture.image_view, texture.sampler)
        .set_layout_out(descriptor_set_layout)
        .build();
}

void Effect2DScene::setup_render_pass()
{
    render_pass = vkutil::RenderPassBuilder(*vulkan)
        .set_color_format(format)
        .set_color_load_op(vk::AttachmentLoadOp::eDontCare)
        .build();
}

void Effect2DScene::setup_pipeline()
{
    auto const pipeline_layout_create_info = vk::PipelineLayoutCreateInfo{}
        .setSetLayoutCount(1)
        .setPSetLayouts(&descriptor_set_layout);
    pipeline_layout = ManagedResource<vk::PipelineLayout>{
        vulkan->device().createPipelineLayout(pipeline_layout_create_info),
        [this] (auto const& pl) { vulkan->device().destroyPipelineLayout(pl); }};

    auto const frag_shader_file =
        "shaders/effect2d-" + options_["kernel"].value + ".frag.spv";

    pipeline = vkutil::PipelineBuilder{*vulkan}
        .set_extent(extent)
        .set_layout(pipeline_layout)
        .set_render_pass(render_pass)
        .set_vertex_shader(Util::read_data_file("shaders/effect2d.vert.spv"))
        .set_fragment_shader(Util::read_data_file(frag_shader_file))
        .set_vertex_input(mesh->binding_descriptions(), mesh->attribute_descriptions())
        .build();

}

void Effect2DScene::setup_framebuffers(std::vector<VulkanImage> const& vulkan_images)
{
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
                .set_image_views({image_view})
                .set_extent(extent)
                .build());
    }
}

void Effect2DScene::setup_command_buffers()
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

        auto const render_pass_begin_info = vk::RenderPassBeginInfo{}
            .setRenderPass(render_pass)
            .setFramebuffer(framebuffers[i])
            .setRenderArea({{0,0}, extent});

        command_buffers[i].beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

        command_buffers[i].bindVertexBuffers(
            0,
            std::vector<vk::Buffer>{binding_offsets.size(), vertex_buffer.raw},
            binding_offsets
            );

        command_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

        command_buffers[i].bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, descriptor_set.raw, {});
        command_buffers[i].draw(mesh->num_vertices(), 1, 0, 0);

        command_buffers[i].endRenderPass();
        command_buffers[i].end();
    }
}

void Effect2DScene::update_uniforms()
{
    Uniforms ubo;

    ubo.texture_step_x = 1.0 / extent.width;
    ubo.texture_step_y = 1.0 / extent.height;

    memcpy(uniform_buffer_map, &ubo, sizeof(ubo));
}
