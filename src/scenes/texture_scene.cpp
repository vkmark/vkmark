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

#include "texture_scene.h"

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
};

}

TextureScene::TextureScene() : Scene{"texture"}
{
    options_["texture-filter"] = SceneOption("texture-filter", "linear",
                                             "The texture filter to use",
                                             "nearest,linear");
    options_["anisotropy"] = SceneOption("anisotropy", "16",
                                         "The max anisotropy bound to use (use 0 to disable it)");
}

TextureScene::~TextureScene() = default;

void TextureScene::setup(
    VulkanState& vulkan_,
    std::vector<VulkanImage> const& vulkan_images)
{
    Scene::setup(vulkan_, vulkan_images);

    vulkan = &vulkan_;
    extent = vulkan_images[0].extent;
    format = vulkan_images[0].format;
    depth_format = vk::Format::eD32Sfloat;
    aspect = static_cast<float>(extent.height) / extent.width;

    mesh = Model{"cube.3ds"}.to_mesh(
        ModelAttribMap{}
            .with_position(vk::Format::eR32G32B32Sfloat)
            .with_normal(vk::Format::eR32G32B32Sfloat)
            .with_texcoord(vk::Format::eR32G32Sfloat));

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
    setup_texture();
    setup_shader_descriptor_set();
    setup_render_pass();
    setup_pipeline();
    setup_depth_image();
    setup_framebuffers(vulkan_images);
    setup_command_buffers();

    submit_semaphore = vkutil::SemaphoreBuilder{*vulkan}.build();
    rotation = 0.0f;
}

void TextureScene::teardown()
{
    vulkan->device().waitIdle();

    submit_semaphore = {};
    vulkan->device().freeCommandBuffers(vulkan->command_pool(), command_buffers);
    framebuffers.clear();
    image_views.clear();
    depth_image_view = {};
    depth_image = {};
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

VulkanImage TextureScene::draw(VulkanImage const& image)
{
    update_uniforms();

    vk::PipelineStageFlags const mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    auto const submit_info = vk::SubmitInfo{}
        .setCommandBufferCount(1)
        .setPCommandBuffers(&command_buffers[image.index])
        .setWaitSemaphoreCount(image.semaphore ? 1 : 0)
        .setPWaitSemaphores(&image.semaphore)
        .setPWaitDstStageMask(&mask)
        .setSignalSemaphoreCount(image.semaphore ? 1 : 0)
        .setPSignalSemaphores(&submit_semaphore.raw);

    vulkan->graphics_queue().submit(submit_info, image.submit_fence);

    return image.copy_with_semaphore(submit_semaphore);
}

void TextureScene::update()
{
    auto const t = (Util::get_timestamp_us() - start_time) / 1000000.0f;

    rotation = 36.0f * t;

    Scene::update();
}

void TextureScene::setup_vertex_buffer()
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

void TextureScene::setup_uniform_buffer()
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

void TextureScene::setup_texture()
{
    auto const& filter = options_["texture-filter"].value;
    auto const anisotropy = std::stof(options_["anisotropy"].value);
    vk::Filter vk_filter = vk::Filter::eLinear;

    if (filter == "nearest")
        vk_filter = vk::Filter::eNearest;
    else if (filter == "linear")
        vk_filter = vk::Filter::eLinear;

    texture = vkutil::TextureBuilder{*vulkan}
        .set_file("textures/crate-base.jpg")
        .set_filter(vk_filter)
        .set_anisotropy(anisotropy)
        .build();
}

void TextureScene::setup_shader_descriptor_set()
{
    descriptor_set = vkutil::DescriptorSetBuilder{*vulkan}
        .set_type(vk::DescriptorType::eUniformBuffer)
        .set_stage_flags(vk::ShaderStageFlagBits::eVertex)
        .set_buffer(uniform_buffer, 0, sizeof(Uniforms))
        .next_binding()
        .set_type(vk::DescriptorType::eCombinedImageSampler)
        .set_stage_flags(vk::ShaderStageFlagBits::eFragment)
        .set_image_view(texture.image_view, texture.sampler)
        .set_layout_out(descriptor_set_layout)
        .build();
}

void TextureScene::setup_render_pass()
{
    render_pass = vkutil::RenderPassBuilder(*vulkan)
        .set_color_format(format)
        .set_depth_format(depth_format)
        .set_color_load_op(vk::AttachmentLoadOp::eClear)
        .build();
}

void TextureScene::setup_pipeline()
{
    auto const pipeline_layout_create_info = vk::PipelineLayoutCreateInfo{}
        .setSetLayoutCount(1)
        .setPSetLayouts(&descriptor_set_layout);
    pipeline_layout = ManagedResource<vk::PipelineLayout>{
        vulkan->device().createPipelineLayout(pipeline_layout_create_info),
        [this] (auto const& pl) { vulkan->device().destroyPipelineLayout(pl); }};

    pipeline = vkutil::PipelineBuilder(*vulkan)
        .set_extent(extent)
        .set_layout(pipeline_layout)
        .set_render_pass(render_pass)
        .set_vertex_shader(Util::read_data_file("shaders/light-basic-tex.vert.spv"))
        .set_fragment_shader(Util::read_data_file("shaders/light-basic-tex.frag.spv"))
        .set_vertex_input(mesh->binding_descriptions(), mesh->attribute_descriptions())
        .set_depth_test(true)
        .build();
}

void TextureScene::setup_depth_image()
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

void TextureScene::setup_framebuffers(std::vector<VulkanImage> const& vulkan_images)
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

void TextureScene::setup_command_buffers()
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

void TextureScene::update_uniforms()
{
    Uniforms ubo;

    glm::mat4 modelview{1.0};
    modelview = glm::translate(modelview, glm::vec3{-center.x, center.y, -(center.z + 2.5 + radius)});
    modelview = glm::rotate(modelview, glm::radians(rotation), {-1.0f, 0.0f, 0.0f});
    modelview = glm::rotate(modelview, glm::radians(rotation), {0.0f,  1.0f, 0.0f});
    modelview = glm::rotate(modelview, glm::radians(rotation), {0.0f, 0.0f, -1.0f});

    ubo.modelviewprojection = projection * modelview;
    ubo.normal = glm::inverseTranspose(modelview);
    ubo.material_diffuse = glm::vec4{0.7f, 0.7f, 0.7f, 1.0f};

    memcpy(uniform_buffer_map, &ubo, sizeof(ubo));
}
