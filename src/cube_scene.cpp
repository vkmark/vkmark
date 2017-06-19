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

// Vertex data based on kmscube/vkcube examples

#include "cube_scene.h"

#include "util.h"
#include "vulkan_state.h"
#include "vulkan_image.h"
#include "vkutil.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <cmath>

namespace
{

struct Uniforms
{
    glm::mat4 modelview;
    glm::mat4 modelviewprojection;
    glm::mat4 normal;
};

struct VertexData
{
    std::vector<vk::VertexInputBindingDescription> binding_descriptions() const
    {
        return std::vector<vk::VertexInputBindingDescription>{
            vk::VertexInputBindingDescription{}
                .setBinding(0)
                .setStride(sizeof(glm::vec3))
                .setInputRate(vk::VertexInputRate::eVertex),
            vk::VertexInputBindingDescription{}
                .setBinding(1)
                .setStride(sizeof(glm::vec3))
                .setInputRate(vk::VertexInputRate::eVertex),
            vk::VertexInputBindingDescription{}
                .setBinding(2)
                .setStride(sizeof(glm::vec3))
                .setInputRate(vk::VertexInputRate::eVertex)};
    }

    std::vector<vk::VertexInputAttributeDescription> attribute_descriptions() const
    {
        std::vector<vk::VertexInputAttributeDescription> desc(3);
        desc[0].binding = 0;
        desc[0].location = 0;
        desc[0].format = vk::Format::eR32G32B32Sfloat;
        desc[0].offset = 0;
        desc[1].binding = 1;
        desc[1].location = 1;
        desc[1].format = vk::Format::eR32G32B32Sfloat;
        desc[1].offset = 0;
        desc[2].binding = 2;
        desc[2].location = 2;
        desc[2].format = vk::Format::eR32G32B32Sfloat;
        desc[2].offset = 0;
        return desc;
    }

    void copy_to(void* dst) const
    {
        auto const dst_c = static_cast<char*>(dst);
        memcpy(dst_c + positions_offset(), positions.data(), positions_size());
        memcpy(dst_c + colors_offset(), colors.data(), colors_size());
        memcpy(dst_c + normals_offset(), normals.data(), normals_size());
    }

    size_t positions_offset() const
    {
        return 0;
    }

    size_t positions_size() const
    {
        return positions.size() * sizeof(positions[0]);
    }

    size_t colors_offset() const
    {
        return positions_offset() + positions_size();
    }

    size_t colors_size() const
    {
        return colors.size() * sizeof(colors[0]);
    }

    size_t normals_offset() const
    {
        return colors_offset() + colors_size();
    }

    size_t normals_size() const
    {
        return normals.size() * sizeof(normals[0]);
    }

    size_t size() const
    {
        return positions_size() + colors_size() + normals_size();
    }

    std::vector<glm::vec3> const positions = {
        // front
        {-1.0f, -1.0f, +1.0f},
        {+1.0f, -1.0f, +1.0f},
        {-1.0f, +1.0f, +1.0f},
        {+1.0f, +1.0f, +1.0f},
        // back
        {+1.0f, -1.0f, -1.0f},
        {-1.0f, -1.0f, -1.0f},
        {+1.0f, +1.0f, -1.0f},
        {-1.0f, +1.0f, -1.0f},
        // right
        {+1.0f, -1.0f, +1.0f},
        {+1.0f, -1.0f, -1.0f},
        {+1.0f, +1.0f, +1.0f},
        {+1.0f, +1.0f, -1.0f},
        // left
        {-1.0f, -1.0f, -1.0f},
        {-1.0f, -1.0f, +1.0f},
        {-1.0f, +1.0f, -1.0f},
        {-1.0f, +1.0f, +1.0f},
        // top
        {-1.0f, +1.0f, +1.0f},
        {+1.0f, +1.0f, +1.0f},
        {-1.0f, +1.0f, -1.0f},
        {+1.0f, +1.0f, -1.0f},
        // bottom
        {-1.0f, -1.0f, -1.0f},
        {+1.0f, -1.0f, -1.0f},
        {-1.0f, -1.0f, +1.0f},
        {+1.0f, -1.0f, +1.0f}
    };

    static glm::vec3 constexpr black = {0.0f, 0.0f, 0.0f};
    static glm::vec3 constexpr blue = {0.0f, 0.0f, 1.0f};
    static glm::vec3 constexpr cyan = {0.0f, 1.0f, 1.0f};
    static glm::vec3 constexpr green = {0.0f, 1.0f, 0.0f};
    static glm::vec3 constexpr magenta = {1.0f, 0.0f, 1.0f};
    static glm::vec3 constexpr red = {1.0f, 0.0f, 0.0f};
    static glm::vec3 constexpr white = {1.0f, 1.0f, 1.0f};
    static glm::vec3 constexpr yellow = {1.0f, 1.0f, 0.0f};

    std::vector<glm::vec3> const colors = {
        // front
        blue, magenta, cyan, white,
        // back
        red, black, yellow, green,
        // right
        magenta, red, white, yellow,
        // left
        black, blue, green, cyan,
        // top
        cyan, white, green, yellow,
        // bottom
        black, red, blue, magenta
    };

   std::vector<glm::vec3> const normals = {
       // front
       {+0.0f, +0.0f, +1.0f},
       {+0.0f, +0.0f, +1.0f},
       {+0.0f, +0.0f, +1.0f},
       {+0.0f, +0.0f, +1.0f},
       // back
       {+0.0f, +0.0f, -1.0f},
       {+0.0f, +0.0f, -1.0f},
       {+0.0f, +0.0f, -1.0f},
       {+0.0f, +0.0f, -1.0f},
       // right
       {+1.0f, +0.0f, +0.0f},
       {+1.0f, +0.0f, +0.0f},
       {+1.0f, +0.0f, +0.0f},
       {+1.0f, +0.0f, +0.0f},
       // left
       {-1.0f, +0.0f, +0.0f},
       {-1.0f, +0.0f, +0.0f},
       {-1.0f, +0.0f, +0.0f},
       {-1.0f, +0.0f, +0.0f},
       // top
       {+0.0f, +1.0f, +0.0f},
       {+0.0f, +1.0f, +0.0f},
       {+0.0f, +1.0f, +0.0f},
       {+0.0f, +1.0f, +0.0f},
       // bottom
       {+0.0f, -1.0f, +0.0f},
       {+0.0f, -1.0f, +0.0f},
       {+0.0f, -1.0f, +0.0f},
       {+0.0f, -1.0f, +0.0f}
   };
};

VertexData const vertex_data{};

}

CubeScene::CubeScene() : Scene{"cube"}
{
}

bool CubeScene::setup(
    VulkanState& vulkan_,
    std::vector<VulkanImage> const& vulkan_images)
{
    Scene::setup(vulkan_, vulkan_images);

    vulkan = &vulkan_;
    extent = vulkan_images[0].extent;
    format = vulkan_images[0].format;
    aspect = static_cast<float>(extent.height) / extent.width;

    setup_vertex_buffer();
    setup_uniform_buffer();
    setup_uniform_descriptor_set();
    setup_render_pass();
    setup_pipeline();
    setup_framebuffers(vulkan_images);
    setup_command_buffers();

    submit_semaphore = vulkan->device().createSemaphore(vk::SemaphoreCreateInfo());

    running = true;

    return true;
}

void CubeScene::teardown()
{
    vulkan->device().waitIdle();

    vulkan->device().destroySemaphore(submit_semaphore);
    vulkan->device().unmapMemory(uniform_buffer_memory);
    vulkan->device().freeCommandBuffers(vulkan->command_pool(), command_buffers);
    framebuffers.clear();
    image_views.clear();
    pipeline = {};
    pipeline_layout = {};
    render_pass = {};
    descriptor_set = {};
    uniform_buffer = {};
    vertex_buffer = {};

    Scene::teardown();
}

VulkanImage CubeScene::draw(VulkanImage const& image)
{
    update_uniforms();

    vk::PipelineStageFlags const mask = vk::PipelineStageFlagBits::eTopOfPipe;
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

void CubeScene::update()
{
    auto const t = (Util::get_timestamp_us() - start_time) / 5000.0;

    rotation = {45.0f + (0.25f * t), 45.0f + (0.5f * t), 10.0f + (0.15f * t)};

    Scene::update();
}

void CubeScene::setup_vertex_buffer()
{
    vk::DeviceMemory vertex_buffer_memory;

    vertex_buffer = vkutil::BufferBuilder{*vulkan}
        .set_size(vertex_data.size())
        .set_usage(vk::BufferUsageFlagBits::eVertexBuffer)
        .set_memory_properties(
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent)
        .set_memory_out(vertex_buffer_memory)
        .build();

    auto const vertex_buffer_map = vulkan->device().mapMemory(
        vertex_buffer_memory, 0, vertex_data.size());
    vertex_data.copy_to(vertex_buffer_map);
    vulkan->device().unmapMemory(vertex_buffer_memory);

}

void CubeScene::setup_uniform_buffer()
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


void CubeScene::setup_uniform_descriptor_set()
{
    descriptor_set = vkutil::DescriptorSetBuilder{*vulkan}
        .set_type(vk::DescriptorType::eUniformBuffer)
        .set_stage_flags(vk::ShaderStageFlagBits::eVertex)
        .set_buffer(uniform_buffer, 0, sizeof(Uniforms))
        .set_layout_out(descriptor_set_layout)
        .build();
}

void CubeScene::setup_render_pass()
{
    render_pass = vkutil::RenderPassBuilder(*vulkan)
        .set_format(format)
        .build();
}

void CubeScene::setup_pipeline()
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
        .set_vertex_shader(Util::read_data_file("shaders/vkcube.vert.spv"))
        .set_fragment_shader(Util::read_data_file("shaders/vkcube.frag.spv"))
        .set_vertex_input(vertex_data.binding_descriptions(), vertex_data.attribute_descriptions())
        .build();
}

void CubeScene::setup_framebuffers(std::vector<VulkanImage> const& vulkan_images)
{
    for (auto const& vulkan_image : vulkan_images)
    {
        image_views.push_back(
            vkutil::ImageViewBuilder{*vulkan}
                .set_image(vulkan_image.image)
                .set_format(vulkan_image.format)
                .build());
    }

    for (auto const& image_view : image_views)
    {
        framebuffers.push_back(
            vkutil::FramebufferBuilder{*vulkan}
                .set_render_pass(render_pass)
                .set_image_view(image_view)
                .set_extent(extent)
                .build());
    }
}

void CubeScene::setup_command_buffers()
{
    auto const command_buffer_allocate_info = vk::CommandBufferAllocateInfo{}
        .setCommandPool(vulkan->command_pool())
        .setCommandBufferCount(framebuffers.size())
        .setLevel(vk::CommandBufferLevel::ePrimary);

    command_buffers = vulkan->device().allocateCommandBuffers(command_buffer_allocate_info);

    for (size_t i = 0; i < command_buffers.size(); ++i)
    {
        auto const begin_info = vk::CommandBufferBeginInfo{}
            .setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

        command_buffers[i].begin(begin_info);

        vk::ClearValue const clear_color{
            vk::ClearColorValue{std::array<float,4>{{0.2f, 0.2f, 0.2f, 1.0f}}}};

        auto const render_pass_begin_info = vk::RenderPassBeginInfo{}
            .setRenderPass(render_pass)
            .setFramebuffer(framebuffers[i])
            .setRenderArea({{0,0}, extent})
            .setClearValueCount(1)
            .setPClearValues(&clear_color);

        command_buffers[i].beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

        command_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
        command_buffers[i].bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, descriptor_set.raw, {});
        command_buffers[i].bindVertexBuffers(
            0,
            {vertex_buffer.raw, vertex_buffer.raw, vertex_buffer.raw},
            {
                vk::DeviceSize{vertex_data.positions_offset()},
                vk::DeviceSize{vertex_data.colors_offset()},
                vk::DeviceSize{vertex_data.normals_offset()}
            });

        command_buffers[i].draw(4, 1, 0, 0);
        command_buffers[i].draw(4, 1, 4, 0);
        command_buffers[i].draw(4, 1, 8, 0);
        command_buffers[i].draw(4, 1, 12, 0);
        command_buffers[i].draw(4, 1, 16, 0);
        command_buffers[i].draw(4, 1, 20, 0);

        command_buffers[i].endRenderPass();
        command_buffers[i].end();
    }
}

void CubeScene::update_uniforms()
{
    Uniforms ubo;

    ubo.modelview = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -8.0f));
    ubo.modelview = glm::rotate(ubo.modelview, glm::radians(rotation.x), {1.0f, 0.0f, 0.0f});
    ubo.modelview = glm::rotate(ubo.modelview, glm::radians(rotation.y), {0.0f, 1.0f, 0.0f});
    ubo.modelview = glm::rotate(ubo.modelview, glm::radians(rotation.z), {0.0f, 0.0f, 1.0f});

    auto const projection = glm::frustum(-2.8f, 2.8f, -2.8f * aspect, 2.8f * aspect, 6.0f, 10.0f);

    ubo.modelviewprojection = projection * ubo.modelview;
    ubo.normal = glm::inverseTranspose(ubo.modelview);

    memcpy(uniform_buffer_map, &ubo, sizeof(ubo));
}
