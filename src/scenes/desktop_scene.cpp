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

#include "desktop_scene.h"

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
    glm::mat4 transform;
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

class DesktopScene::RenderObject
{
public:
    RenderObject(VulkanState& vulkan, std::string const& texture_file)
        : vulkan{vulkan}
    {
        texture = vkutil::TextureBuilder{vulkan}
            .set_file(texture_file)
            .set_filter(vk::Filter::eLinear)
            .build();

        uniform_buffer = vkutil::BufferBuilder{vulkan}
            .set_size(sizeof(Uniforms))
            .set_usage(vk::BufferUsageFlagBits::eUniformBuffer)
            .set_memory_properties(
                vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent)
            .set_memory_out(uniform_buffer_memory)
            .build();

        uniform_buffer_map = vulkan.device().mapMemory(
            uniform_buffer_memory, 0, sizeof(Uniforms));

        descriptor_set = vkutil::DescriptorSetBuilder{vulkan}
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

    ~RenderObject()
    {
        vulkan.device().unmapMemory(uniform_buffer_memory);
        descriptor_set = {};
        uniform_buffer = {};
        texture = {};
    }

    void update(float dt)
    {
        bool should_update = true;
        auto const new_pos = position + speed * dt;

        if (new_pos.x - size.x < -1.0f || new_pos.x + size.x > 1.0f)
        {
            speed.x = -speed.x;
            should_update = false;
        }

        if (new_pos.y - size.y < -1.0f || new_pos.y + size.y > 1.0f)
        {
            speed.y = -speed.y;
            should_update = false;
        }

        if (should_update)
            position = new_pos;
    }

    void update_uniforms() const
    {
        Uniforms ubo;

        ubo.transform = glm::translate(glm::mat4{1.0f}, glm::vec3{position.x, position.y, 0.0f});
        ubo.transform = glm::scale(ubo.transform, glm::vec3{size.x, size.y, 1.0f});

        memcpy(uniform_buffer_map, &ubo, sizeof(ubo));
    }

    glm::vec2 position{0.0f, 0.0f};
    glm::vec2 size{1.0f, 1.0f};
    glm::vec2 speed{0.0f, 0.0f};

    VulkanState& vulkan;
    vkutil::Texture texture;
    ManagedResource<vk::Buffer> uniform_buffer;
    ManagedResource<vk::DescriptorSet> descriptor_set;
    vk::DescriptorSetLayout descriptor_set_layout;
    vk::DeviceMemory uniform_buffer_memory;
    void* uniform_buffer_map;
};

DesktopScene::DesktopScene() : Scene{"desktop"}
{
    options_["windows"] = SceneOption("windows", "4",
                                      "the number of windows");
    options_["window-size"] = SceneOption("window-size", "0.35",
                                          "the window size as a percentage of the minimum screen dimension [0.0 - 0.5]");
    options_["background-resolution"] =
        SceneOption("background-resolution", "800x600",
                    "the resolution of the background image",
                    "800x600,1920x1080");
}

DesktopScene::~DesktopScene() = default;

void DesktopScene::setup(
    VulkanState& vulkan_,
    std::vector<VulkanImage> const& vulkan_images)
{
    Scene::setup(vulkan_, vulkan_images);

    vulkan = &vulkan_;
    extent = vulkan_images[0].extent;
    format = vulkan_images[0].format;

    mesh = create_quad_mesh();

    auto const texture_file =
        "textures/desktop-background-" +
        options_["background-resolution"].value +
        ".png";

    background = std::make_unique<RenderObject>(*vulkan, texture_file);
    background->update_uniforms();

    auto const aspect = static_cast<float>(extent.width) / extent.height;
    auto const num_windows = Util::from_string<unsigned int>(options_["windows"].value);
    auto const window_size_factor = Util::from_string<float>(options_["window-size"].value);
    auto const window_size = glm::vec2{window_size_factor * (aspect > 1.0f ? 1.0 / aspect : 1.0f),
                                       window_size_factor * (aspect < 1.0f ? aspect : 1.0f)};

    windows.resize(num_windows);

    for (auto i = 0u; i < windows.size(); ++i)
    {
        windows[i] = std::make_unique<RenderObject>(*vulkan, "textures/desktop-window.png");
        windows[i]->size = window_size;
        windows[i]->speed = {std::cos(0.1 + i * M_PI / 6.0) * 2.0 / 3,
                             std::sin(0.1 + i * M_PI / 6.0) * 2.0 / 3};
    }

    setup_vertex_buffer();
    setup_render_pass();
    setup_pipeline();
    setup_framebuffers(vulkan_images);
    setup_command_buffers();

    submit_semaphore = vkutil::SemaphoreBuilder{*vulkan}.build();
}

void DesktopScene::teardown()
{
    vulkan->device().waitIdle();

    submit_semaphore = {};
    vulkan->device().freeCommandBuffers(vulkan->command_pool(), command_buffers);
    framebuffers.clear();
    image_views.clear();
    pipeline_opaque = {};
    pipeline_blend = {};
    pipeline_layout = {};
    render_pass = {};
    vertex_buffer = {};

    windows.clear();
    background = {};

    Scene::teardown();
}

VulkanImage DesktopScene::draw(VulkanImage const& image)
{
    update_uniforms();

    vk::PipelineStageFlags const mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    auto const submit_info = vk::SubmitInfo{}
        .setCommandBufferCount(1)
        .setPCommandBuffers(&command_buffers[image.index])
        .setWaitSemaphoreCount(image.semaphore ? 1 : 0)
        .setPWaitSemaphores(&image.semaphore)
        .setPWaitDstStageMask(&mask)
        .setSignalSemaphoreCount(1)
        .setPSignalSemaphores(&submit_semaphore.raw);

    vulkan->graphics_queue().submit(submit_info, {});

    return image.copy_with_semaphore(submit_semaphore);
}

void DesktopScene::update()
{
    auto const dt = (Util::get_timestamp_us() - last_update_time) / 1000000.0f;

    for (auto const& window : windows)
        window->update(dt);

    Scene::update();
}

void DesktopScene::setup_vertex_buffer()
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

void DesktopScene::setup_render_pass()
{
    render_pass = vkutil::RenderPassBuilder(*vulkan)
        .set_color_format(format)
        .set_color_load_op(vk::AttachmentLoadOp::eDontCare)
        .build();
}

void DesktopScene::setup_pipeline()
{
    auto const pipeline_layout_create_info = vk::PipelineLayoutCreateInfo{}
        .setSetLayoutCount(1)
        .setPSetLayouts(&windows.front()->descriptor_set_layout);
    pipeline_layout = ManagedResource<vk::PipelineLayout>{
        vulkan->device().createPipelineLayout(pipeline_layout_create_info),
        [this] (auto const& pl) { vulkan->device().destroyPipelineLayout(pl); }};

    vkutil::PipelineBuilder pipeline_builder{*vulkan};

    pipeline_builder
        .set_extent(extent)
        .set_layout(pipeline_layout)
        .set_render_pass(render_pass)
        .set_vertex_shader(Util::read_data_file("shaders/desktop.vert.spv"))
        .set_fragment_shader(Util::read_data_file("shaders/desktop.frag.spv"))
        .set_vertex_input(mesh->binding_descriptions(), mesh->attribute_descriptions());

    pipeline_opaque = pipeline_builder.build();

    pipeline_blend = pipeline_builder.set_blend(true).build();
}

void DesktopScene::setup_framebuffers(std::vector<VulkanImage> const& vulkan_images)
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

void DesktopScene::setup_command_buffers()
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

        // Draw background opaquely
        command_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_opaque);

        command_buffers[i].bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, background->descriptor_set.raw, {});
        command_buffers[i].draw(mesh->num_vertices(), 1, 0, 0);

        // Draw windows with blending
        command_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_blend);

        for (auto const& window : windows)
        {
            command_buffers[i].bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, window->descriptor_set.raw, {});

            command_buffers[i].draw(mesh->num_vertices(), 1, 0, 0);
        }

        command_buffers[i].endRenderPass();
        command_buffers[i].end();
    }
}

void DesktopScene::update_uniforms()
{
    for (auto const& window : windows)
        window->update_uniforms();
}
