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

ManagedResource<int> open_active_vt()
{
    auto fd = open("/dev/tty0", O_RDONLY);
    if (fd < 0)
        throw std::runtime_error{"Failed to open active VT"};

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

}

VTState::VTState()
    : vt_fd{open_active_vt()}
{
    if (ioctl(vt_fd, VT_GETMODE, &prev_vt_mode) < 0)
    {
        throw std::system_error{
            errno, std::system_category(), "Failed to get VT control mode"};
    }

    vt_mode const vtm{VT_PROCESS, 0, 0, 0, 0};

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

KMSWindowSystem::KMSWindowSystem(std::string const& drm_device)
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
      has_crtc_been_set{false}
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

// TODO: Use an official extension to create the VkImages when it becomes
// available (e.g. VK_MESAX_external_image_dma_buf)

static int find_memory_type_index(VkPhysicalDevice physical_device,
                                  const VkMemoryRequirements* requirements,
                                  VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &properties);
    for (int i = 0; i <= 31; i++) {
        if (((1u << i) & requirements->memoryTypeBits) == 0)
            continue;
        if ((properties.memoryTypes[i].propertyFlags & flags) != flags)
            continue;
        return i;
    }
    return -1;
}

void KMSWindowSystem::create_vk_images()
{
    for (auto const& gbm_bo : gbm_bos)
    {
        auto const fd = ManagedResource<int>{gbm_bo_get_fd(gbm_bo), close};

        VkExternalMemoryImageCreateInfoKHR external_image_create_info = {
            .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR,
            .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
        };

        uint64_t modifier = DRM_FORMAT_MOD_LINEAR;
        VkImageDrmFormatModifierListCreateInfoEXT modifier_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT,
            .drmFormatModifierCount = 1,
            .pDrmFormatModifiers = &modifier,
        };
        external_image_create_info.pNext = &modifier_info;

        VkImportMemoryFdInfoKHR import_memory_fd_info = {
            .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
            .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
            .fd = fd,
        };

        VkImageCreateInfo create_info{};
        create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = &external_image_create_info,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = static_cast<VkFormat>(vk_image_format),
            .extent = {vk_extent.width, vk_extent.height, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT,
            .usage = 0,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        VkImage image;

        VkResult result = vkCreateImage(
            vulkan->device(),
            &create_info,
            nullptr,
            &image);
        if (result != VK_SUCCESS)
        {
            vk::throwResultException(static_cast<vk::Result>(result),
                                     "vkCreateImage");
        }

        VkMemoryDedicatedAllocateInfoKHR dedicated_memory_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,
            .pNext = &import_memory_fd_info,
            .image = image,
        };

        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(vulkan->device(), image, &requirements);
        if (!requirements.memoryTypeBits) {
            throw std::runtime_error{"Failed in vkGetImageMemoryRequirements"};
        }

        int index = find_memory_type_index(vulkan->physical_device(),
                                           &requirements,
                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (index < 0) {
            throw std::runtime_error{"Failed to get memoryTypeIndex"};
        }

        VkMemoryAllocateInfo memory_allocate_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = &dedicated_memory_info,
            .allocationSize = requirements.size,
            .memoryTypeIndex = (uint32_t)index,
        };

        VkDeviceMemory device_memory = VK_NULL_HANDLE;
        result = vkAllocateMemory(vulkan->device(), &memory_allocate_info,
            nullptr /* pAllocator */, &device_memory);

        if (result != VK_SUCCESS)
        {
            vk::throwResultException(static_cast<vk::Result>(result),
                                     "vkAllocateMemory");
        }

        result = vkBindImageMemory(vulkan->device(), image, device_memory,
            0 /* memoryOffset */);
        if (result != VK_SUCCESS) {
            vk::throwResultException(static_cast<vk::Result>(result),
                                     "vkBindImageMemory");
        }

        vk_images.push_back(
            ManagedResource<vk::Image>{
                vk::Image{image},
                [vptr=vulkan, device_memory] (auto& image)
                {
                    vptr->device().destroyImage(image);
                    vptr->device().freeMemory(device_memory);
                }});

        std::array<VkSubresourceLayout, 4> layouts = {};
        const VkImageSubresource image_subresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = 0,
          .arrayLayer = 0,
        };
        vkGetImageSubresourceLayout(vulkan->device(), image,
            &image_subresource, &layouts[0]);

        uint32_t fb = 0;

        uint32_t handles[4] = {0,};
        uint32_t strides[4] = {0,};
        uint32_t offsets[4] = {0,};

        for (auto i = 0; i < gbm_bo_get_plane_count(gbm_bo); i++) {
            handles[i] = gbm_bo_get_handle(gbm_bo).u32;
            offsets[i] = layouts[i].offset;
            strides[i] = layouts[i].rowPitch;
        }

        auto const ret = drmModeAddFB2(
            drm_fd, vk_extent.width, vk_extent.height,
            DRM_FORMAT_XRGB8888,
            handles, strides, offsets, &fb, 0);

        if (ret < 0)
            throw std::system_error{-ret, std::system_category(), "Failed to add drm fb"};

        drm_fbs.push_back(
            ManagedResource<uint32_t>{
            std::move(fb),
            [this] (auto& fb) { drmModeRmFB(drm_fd, fb); }});
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

std::vector<char const*> KMSWindowSystem::vulkan_extensions()
{
    return {};
}

bool KMSWindowSystem::is_physical_device_supported(vk::PhysicalDevice const&)
{
    return true;
}

std::vector<uint32_t> KMSWindowSystem::physical_device_queue_family_indices(
    vk::PhysicalDevice const&)
{
    return {};
}
