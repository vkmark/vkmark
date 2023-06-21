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

#include "kms_window_system.h"

#include "vulkan_image.h"
#include "vulkan_state.h"

#include "log.h"

#include <xf86drm.h>
#include <drm_fourcc.h>

#include <algorithm>
#include <system_error>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <csignal>

namespace
{

ManagedResource<int> open_drm_device(std::string const& drm_device)
{
    int drm_fd = open(drm_device.c_str(), O_RDWR);
    if (drm_fd < 0)
    {
        throw std::system_error{
            errno,
            std::system_category(),
            "Failed to open drm device"};
    }

    return ManagedResource<int>{std::move(drm_fd), close};
}

ManagedResource<drmModeResPtr> get_resources_for(int drm_fd)
{
    return ManagedResource<drmModeResPtr>{
        drmModeGetResources(drm_fd), drmModeFreeResources};
}

ManagedResource<drmModeConnectorPtr> get_connector_with_id(int drm_fd, uint32_t connector_id)
{
    return ManagedResource<drmModeConnectorPtr>{
        drmModeGetConnector(drm_fd, connector_id), drmModeFreeConnector};
}


ManagedResource<drmModeEncoderPtr> get_encoder_with_id(int drm_fd, uint32_t encoder_id)
{
    return ManagedResource<drmModeEncoderPtr>{
        drmModeGetEncoder(drm_fd, encoder_id), drmModeFreeEncoder};
}

ManagedResource<drmModeCrtcPtr> get_crtc_with_id(int drm_fd, uint32_t crtc_id)
{
    return ManagedResource<drmModeCrtcPtr>{
        drmModeGetCrtc(drm_fd, crtc_id), drmModeFreeCrtc};
}

ManagedResource<drmModeConnectorPtr> get_connected_connector(
    int drm_fd, drmModeResPtr resources)
{
    for (int c = 0; c < resources->count_connectors; c++)
    {
        auto connector = get_connector_with_id(drm_fd, resources->connectors[c]);

        if (connector->connection == DRM_MODE_CONNECTED)
        {
            Log::debug("KMSWindowSystem: Using connector %d\n",
                       connector->connector_id);
            return connector;
        }
    }

    throw std::runtime_error{"Failed to find a connected drm connector"};
}

bool is_encoder_available(int drm_fd, drmModeResPtr resources, uint32_t encoder_id)
{
    for (int c = 0; c < resources->count_connectors; c++)
    {
        auto connector = get_connector_with_id(drm_fd, resources->connectors[c]);

        if (connector->encoder_id == encoder_id &&
            connector->connection == DRM_MODE_CONNECTED)
        {
            auto encoder = get_encoder_with_id(drm_fd, connector->encoder_id);
            if (encoder->crtc_id)
                return false;
        }
    }

    return true;
}

bool is_crtc_available(int drm_fd, drmModeResPtr resources, uint32_t crtc_id)
{
    for (int c = 0; c < resources->count_connectors; c++)
    {
        auto connector = get_connector_with_id(drm_fd, resources->connectors[c]);

        if (connector->encoder_id &&
            connector->connection == DRM_MODE_CONNECTED)
        {
            auto encoder = get_encoder_with_id(drm_fd, connector->encoder_id);
            if (encoder->crtc_id == crtc_id)
                return false;
        }
    }

    return true;
}

std::vector<ManagedResource<drmModeEncoderPtr>> get_available_encoders_for_connector(
    int drm_fd, drmModeResPtr resources, drmModeConnectorPtr connector)
{
    std::vector<ManagedResource<drmModeEncoderPtr>> encoders;

    for (int e = 0; e < connector->count_encoders; e++)
    {
        auto const encoder_id = connector->encoders[e];
        if (is_encoder_available(drm_fd, resources, encoder_id))
            encoders.push_back(get_encoder_with_id(drm_fd, encoder_id));
    }

    return encoders;
}

bool does_encoder_support_crtc_index(drmModeEncoderPtr encoder, uint32_t crtc_index)
{
    return encoder->possible_crtcs & (1 << crtc_index);
}

ManagedResource<drmModeCrtcPtr> get_current_crtc_for_connector(
    int drm_fd, drmModeConnectorPtr connector)
{
    if (connector->encoder_id)
    {
        auto const encoder = get_encoder_with_id(drm_fd, connector->encoder_id);
        if (encoder->crtc_id)
            return get_crtc_with_id(drm_fd, encoder->crtc_id);
    }

    return ManagedResource<drmModeCrtcPtr>{
        new drmModeCrtc{},
        [] (auto& c) { delete c; }};
}

ManagedResource<drmModeCrtcPtr> get_configured_crtc_with_id(
    int drm_fd, drmModeConnectorPtr connector, uint32_t crtc_id)
{
    auto crtc = get_crtc_with_id(drm_fd, crtc_id);
    crtc->mode = {};

    // Use the preferred mode, otherwise the largest available
    for (int m = 0; m < connector->count_modes; ++m)
    {
        auto const& mode_info = connector->modes[m];
        if (mode_info.type & DRM_MODE_TYPE_PREFERRED)
        {
            crtc->mode = mode_info;
            break;
        }

        auto const mode_pixels = mode_info.hdisplay * mode_info.vdisplay;
        if (mode_pixels > crtc->mode.hdisplay * crtc->mode.vdisplay)
            crtc->mode = mode_info;
    }

    Log::debug("KMSWindowSystem: Using crtc mode %dx%d%s\n",
               crtc->mode.hdisplay,
               crtc->mode.vdisplay,
               crtc->mode.type & DRM_MODE_TYPE_PREFERRED ? " (preferred)" : "");
    return crtc;
}

ManagedResource<drmModeCrtcPtr> get_crtc_for_connector(
    int drm_fd, drmModeResPtr resources, drmModeConnectorPtr connector)
{
    if (connector->encoder_id)
    {
        auto const encoder = get_encoder_with_id(drm_fd, connector->encoder_id);
        if (encoder->crtc_id)
        {
            Log::debug("KMSWindowSystem: Using already attached encoder %d, crtc %d\n",
                       encoder->encoder_id,
                       encoder->crtc_id);
            return get_configured_crtc_with_id(drm_fd, connector, encoder->crtc_id);
        }
    }

    Log::debug("KMSWindowSystem: No crtc/encoder attached to connector, "
               "trying to attach\n");

    auto const available_encoders = get_available_encoders_for_connector(
        drm_fd, resources, connector);

    for (int c = 0; c < resources->count_crtcs; c++)
    {
        auto const crtc_id = resources->crtcs[c];

        Log::debug("KMSWindowSystem: Trying crtc %d\n", crtc_id);

        if (is_crtc_available(drm_fd, resources, crtc_id))
        {
            for (auto const& encoder : available_encoders)
            {
                if (does_encoder_support_crtc_index(encoder, c))
                {
                    return get_configured_crtc_with_id(drm_fd, connector, crtc_id);
                }
            }
        }
    }

    throw std::runtime_error{"Failed to get usable crtc"};
}

ManagedResource<gbm_device*> create_gbm_device(int drm_fd)
{
    auto gbm_raw = gbm_create_device(drm_fd);
    if (!gbm_raw)
        throw std::runtime_error{"Failed to create gbm device"};

    return ManagedResource<gbm_device*>{std::move(gbm_raw), gbm_device_destroy};

}

int open_vt(const char *path)
{
    struct vt_mode vt_mode;
    auto fd = open(path, O_RDONLY);

    if (fd < 0)
        return -1;

    if (ioctl(fd, VT_GETMODE, &vt_mode) < 0)
    {
        close(fd);
        return -1;
    }

    return fd;
}

ManagedResource<int> open_active_vt()
{
    auto fd = open_vt("/dev/tty");
    if (fd < 0)
    {
        Log::debug("/dev/tty is not a VT, trying to use /dev/tty0\n");
        fd = open_vt("/dev/tty0");
        if (fd < 0)
            throw std::runtime_error{"Failed to open active VT"};
    }

    return ManagedResource<int>{std::move(fd), close};
}

void page_flip_handler(int, unsigned int, unsigned int, unsigned int, void*)
{
}

VTState* global_vt_state = nullptr;

void restore_vt(int)
{
    if (global_vt_state)
        global_vt_state->restore();
}

uint32_t find_memory_type_index(vk::PhysicalDevice const& physical_device,
                                vk::MemoryRequirements const& requirements,
                                vk::MemoryPropertyFlags flags)
{
    auto const properties = physical_device.getMemoryProperties();

    for (uint32_t i = 0; i < properties.memoryTypeCount; i++)
    {
        if ((requirements.memoryTypeBits & (1 << i)) &&
            (properties.memoryTypes[i].propertyFlags & flags) == flags)
        {
            return i;
        }
    }

    throw std::runtime_error{"Coudn't find matching memory type"};
}

}

VTState::VTState()
    : vt_fd{open_active_vt()}
{
    if (ioctl(vt_fd, VT_GETMODE, &prev_vt_mode) < 0)
    {
        throw std::system_error{
            errno, std::system_category(), "Failed to get VT control mode"};
    }

    vt_mode const vtm{VT_PROCESS, 0, SIGINT, SIGINT, SIGINT};

    if (ioctl(vt_fd, VT_SETMODE, &vtm) < 0)
    {
        throw std::system_error{
            errno, std::system_category(), "Failed to set VT process control mode"};
    }

    global_vt_state = this;

    struct sigaction sa{};
    sa.sa_handler = restore_vt;

    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
}

void VTState::restore() const
{
    if (prev_vt_mode.mode == VT_AUTO)
        ioctl(vt_fd, VT_SETMODE, &prev_vt_mode);
}

VTState::~VTState()
{
    restore();

    struct sigaction sa{};
    sa.sa_handler = SIG_DFL;

    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);

    global_vt_state = nullptr;
}

KMSWindowSystem::KMSWindowSystem(std::string const& drm_device,
                                 vk::ImageTiling mod_invalid_tiling)
    : drm_fd{open_drm_device(drm_device)},
      drm_resources{get_resources_for(drm_fd)},
      drm_connector{get_connected_connector(drm_fd, drm_resources)},
      drm_prev_crtc{get_current_crtc_for_connector(drm_fd, drm_connector)},
      drm_crtc{get_crtc_for_connector(drm_fd, drm_resources, drm_connector)},
      gbm{create_gbm_device(drm_fd)},
      vk_extent{drm_crtc->mode.hdisplay, drm_crtc->mode.vdisplay},
      vulkan{nullptr},
      vk_image_format{vk::Format::eUndefined},
      current_image_index{0},
      has_crtc_been_set{false},
      mod_invalid_tiling{mod_invalid_tiling}
{
}

KMSWindowSystem::~KMSWindowSystem()
{
    drmModeSetCrtc(
        drm_fd, drm_prev_crtc->crtc_id, drm_prev_crtc->buffer_id,
        drm_prev_crtc->x, drm_prev_crtc->y,
        &drm_connector->connector_id, 1,
        &drm_prev_crtc->mode);
}

VulkanWSI& KMSWindowSystem::vulkan_wsi()
{
    return *this;
}

void KMSWindowSystem::init_vulkan(VulkanState& vulkan_)
{
    vulkan = &vulkan_;

    vk_image_format = vk::Format::eB8G8R8A8Srgb;
    create_gbm_bos();
    create_drm_fbs();
    create_vk_images();
}

void KMSWindowSystem::deinit_vulkan()
{
    vulkan->device().waitIdle();

    vk_images.clear();
    drm_fbs.clear();
    gbm_bos.clear();
}

VulkanImage KMSWindowSystem::next_vulkan_image()
{
    return {current_image_index, vk_images[current_image_index], vk_image_format, vk_extent, nullptr};
}

void KMSWindowSystem::present_vulkan_image(VulkanImage const& vulkan_image)
{
    auto const& fb = drm_fbs[vulkan_image.index];

    // We can't use the VulkanImage semaphore in the KMS window system to
    // synchronize rendering and presentation, so just wait for the graphics
    // queue to finish before flipping.
    vulkan->graphics_queue().waitIdle();

    if (!has_crtc_been_set)
    {
        auto const ret = drmModeSetCrtc(
            drm_fd, drm_crtc->crtc_id, fb, 0, 0, &drm_connector->connector_id, 1,
            &drm_crtc->mode);
        if (ret < 0)
            throw std::system_error{-ret, std::system_category(), "Failed to set crtc"};

        has_crtc_been_set = true;
    }

    drmModePageFlip(drm_fd, drm_crtc->crtc_id, fb, DRM_MODE_PAGE_FLIP_EVENT, nullptr);

    wait_for_drm_page_flip_event();

    current_image_index = (current_image_index + 1) % vk_images.size();
}

std::vector<VulkanImage> KMSWindowSystem::vulkan_images()
{
    std::vector<VulkanImage> vulkan_images;

    for (uint32_t i = 0; i < vk_images.size(); ++i)
        vulkan_images.push_back({i, vk_images[i], vk_image_format, vk_extent, {}});

    return vulkan_images;
}

bool KMSWindowSystem::should_quit()
{
    return false;
}

void KMSWindowSystem::create_gbm_bos()
{
    for (int i = 0; i < 2; ++i)
    {
        auto bo_raw = gbm_bo_create(
            gbm, vk_extent.width, vk_extent.height, GBM_FORMAT_XRGB8888,
            GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);

        if (!bo_raw)
            throw std::runtime_error{"Failed to create gbm bo"};

        gbm_bos.push_back(
            ManagedResource<gbm_bo*>{std::move(bo_raw), gbm_bo_destroy});
    }
}

void KMSWindowSystem::create_drm_fbs()
{
    for (auto const& gbm_bo : gbm_bos)
    {
        uint32_t fb = 0;
        uint32_t handles[4] = {0};
        uint32_t strides[4] = {0};
        uint32_t offsets[4] = {0};

        for (auto i = 0; i < gbm_bo_get_plane_count(gbm_bo); i++)
        {
            handles[i] = gbm_bo_get_handle_for_plane(gbm_bo, i).u32;
            offsets[i] = gbm_bo_get_offset(gbm_bo, i);
            strides[i] = gbm_bo_get_stride_for_plane(gbm_bo, i);
        }

        auto const ret = drmModeAddFB2(
            drm_fd, vk_extent.width, vk_extent.height,
            gbm_bo_get_format(gbm_bo),
            handles, strides, offsets, &fb, 0);

        if (ret < 0)
            throw std::system_error{-ret, std::system_category(), "Failed to add drm fb"};

        drm_fbs.push_back(
            ManagedResource<uint32_t>{
                std::move(fb),
                [this] (auto& fb) { drmModeRmFB(drm_fd, fb); }});
    }
}

void KMSWindowSystem::create_vk_images()
{
    bool warned_mod_invalid = false;

    for (auto const& gbm_bo : gbm_bos)
    {
        vk::SubresourceLayout layouts[4] = {0};
        uint32_t num_planes = gbm_bo_get_plane_count(gbm_bo);
        auto const fd = ManagedResource<int>{gbm_bo_get_fd(gbm_bo), close};
        uint64_t modifier = gbm_bo_get_modifier(gbm_bo);

        if (modifier == DRM_FORMAT_MOD_INVALID && !warned_mod_invalid)
        {
            Log::warning("KMSWindowSystem: Using VK_IMAGE_TILING_OPTIMAL for "
                         "dmabuf with invalid modifier, but this is not "
                         "guaranteed to work.\n");
            warned_mod_invalid = true;
        }

        for (uint32_t i = 0; i < num_planes; i++)
        {
            layouts[i].offset = gbm_bo_get_offset(gbm_bo, i);
            layouts[i].rowPitch = gbm_bo_get_stride_for_plane(gbm_bo, i);
        }

        auto const modifier_info = vk::ImageDrmFormatModifierExplicitCreateInfoEXT{}
            .setDrmFormatModifier(modifier)
            .setDrmFormatModifierPlaneCount(num_planes)
            .setPPlaneLayouts(layouts);
        auto const external_memory_create_info = vk::ExternalMemoryImageCreateInfoKHR{}
            .setHandleTypes(vk::ExternalMemoryHandleTypeFlagBitsKHR::eDmaBufEXT)
            .setPNext(modifier != DRM_FORMAT_MOD_INVALID ? &modifier_info : nullptr);
        auto const image_create_info = vk::ImageCreateInfo{}
            .setPNext(&external_memory_create_info)
            .setImageType(vk::ImageType::e2D)
            .setFormat(vk_image_format)
            .setExtent({vk_extent.width, vk_extent.height, 1})
            .setMipLevels(1)
            .setArrayLayers(1)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setTiling(modifier != DRM_FORMAT_MOD_INVALID ?
                       vk::ImageTiling::eDrmFormatModifierEXT :
                       mod_invalid_tiling)
            .setUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setInitialLayout(vk::ImageLayout::eUndefined);

        auto vk_image = ManagedResource<vk::Image>{
            vulkan->device().createImage(image_create_info),
            [vptr=vulkan] (auto const& i) { vptr->device().destroyImage(i); }};

        auto const requirements = vulkan->device().getImageMemoryRequirements(vk_image);
        uint32_t index = find_memory_type_index(vulkan->physical_device(),
                                                requirements,
                                                vk::MemoryPropertyFlagBits::eDeviceLocal);

        auto const import_memory_fd_info = vk::ImportMemoryFdInfoKHR{}
            .setHandleType(vk::ExternalMemoryHandleTypeFlagBitsKHR::eDmaBufEXT)
            .setFd(fd);
        auto const dedicated_allocate_info = vk::MemoryDedicatedAllocateInfoKHR{}
            .setPNext(&import_memory_fd_info)
            .setImage(vk_image);
        auto const memory_allocate_info = vk::MemoryAllocateInfo{}
            .setPNext(&dedicated_allocate_info)
            .setAllocationSize(requirements.size)
            .setMemoryTypeIndex((uint32_t)index);

        auto device_memory = ManagedResource<vk::DeviceMemory>{
            vulkan->device().allocateMemory(memory_allocate_info),
            [vptr=vulkan] (auto const& m) { vptr->device().freeMemory(m); }};

        vulkan->device().bindImageMemory(vk_image, device_memory, 0);

        vk_images.push_back(
            ManagedResource<vk::Image>{
                vk_image.steal(),
                [vptr=vulkan, mem=device_memory.steal()] (auto const& image)
                {
                    vptr->device().destroyImage(image);
                    vptr->device().freeMemory(mem);
                }});
    }
}

void KMSWindowSystem::wait_for_drm_page_flip_event()
{
    static int constexpr drm_event_context_version = 2;
    static drmEventContext event_context = {
        drm_event_context_version,
        nullptr,
        page_flip_handler};

    pollfd pfd{drm_fd, POLLIN, 0};

    while (true)
    {
        auto const ret = poll(&pfd, 1, 1000);
        if (ret < 0)
        {
            if (errno == EINTR)
                continue;

            throw std::system_error{
                errno, std::system_category(), "Failed while polling for pages flip event"};
        }

        if (pfd.revents & POLLIN)
        {
            drmHandleEvent(drm_fd, &event_context);
            break;
        }
    }
}

VulkanWSI::Extensions KMSWindowSystem::required_extensions()
{
    return {{}, {VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
                 VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME,
                 VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME}};
}

bool KMSWindowSystem::is_physical_device_supported(vk::PhysicalDevice const& physdev)
{
    auto const props = physdev.enumerateDeviceExtensionProperties();
    auto const exts = required_extensions();
    auto const extension_is_supported =
        [&props] (std::string const& ext)
        {
            auto const iter = std::find_if(props.begin(), props.end(),
                [&ext](vk::ExtensionProperties prop)
                {
                    return ext == prop.extensionName;
                });
            return iter != props.end();
        };

    return std::all_of(exts.device.begin(), exts.device.end(),
                       extension_is_supported);
}

std::vector<uint32_t> KMSWindowSystem::physical_device_queue_family_indices(
    vk::PhysicalDevice const&)
{
    return {};
}
