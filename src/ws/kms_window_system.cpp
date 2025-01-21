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
#include <optional>
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
    auto resources = drmModeGetResources(drm_fd);
    if (!resources)
    {
        throw std::system_error{
            errno, std::system_category(), "Failed to get drm resources"};
    }
    return ManagedResource<drmModeResPtr>{
        std::move(resources), drmModeFreeResources};
}

ManagedResource<drmModeConnectorPtr> get_connector_with_id(int drm_fd, uint32_t connector_id)
{
    auto connector = drmModeGetConnector(drm_fd, connector_id);
    if (!connector)
    {
        throw std::system_error{
            errno, std::system_category(), "Failed to get drm connector"};
    }
    return ManagedResource<drmModeConnectorPtr>{
        std::move(connector), drmModeFreeConnector};
}


ManagedResource<drmModeEncoderPtr> get_encoder_with_id(int drm_fd, uint32_t encoder_id)
{
    auto encoder = drmModeGetEncoder(drm_fd, encoder_id);
    if (!encoder)
    {
        throw std::system_error{
            errno, std::system_category(), "Failed to get drm encoder"};
    }
    return ManagedResource<drmModeEncoderPtr>{
        std::move(encoder), drmModeFreeEncoder};
}

ManagedResource<drmModeCrtcPtr> get_crtc_with_id(int drm_fd, uint32_t crtc_id)
{
    auto crtc = drmModeGetCrtc(drm_fd, crtc_id);
    if (!crtc)
    {
        throw std::system_error{
            errno, std::system_category(), "Failed to get drm crtc"};
    }
    return ManagedResource<drmModeCrtcPtr>{std::move(crtc), drmModeFreeCrtc};
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

    Log::debug("KMSWindowSystem: Using tty %s\n", path);

    return fd;
}

ManagedResource<int> open_active_vt(std::string const& vt)
{
    auto fd = open_vt(vt.c_str());
    if (fd < 0)
    {
        Log::debug("%s is not an accessible VT, trying to use /dev/tty0\n", vt.c_str());
        fd = open_vt("/dev/tty0");
        if (fd < 0)
            throw std::runtime_error{"Failed to open VT"};
    }

    return ManagedResource<int>{std::move(fd), close};
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

std::optional<uint64_t> drm_props_get_value(int drm_fd, drmModeObjectProperties *props,
                                            char const* name)
{
    for (uint32_t i = 0; i < props->count_props; ++i)
    {
        auto const prop = ManagedResource<drmModePropertyPtr>{
            drmModeGetProperty(drm_fd, props->props[i]),
            drmModeFreeProperty};
        if (!prop) continue;
        if (!strcmp(prop->name, name))
            return {props->prop_values[i]};
    }

    return std::nullopt;
}

std::vector<uint64_t> drm_get_supported_mods_for_format(int drm_fd, uint32_t format)
{
    std::vector<uint64_t> supported_mods;
    auto const res = ManagedResource<drmModePlaneResPtr>{
        drmModeGetPlaneResources(drm_fd),
        drmModeFreePlaneResources};
    if (!res)
        return {};

    for (uint32_t i = 0; i < res->count_planes; ++i)
    {
        auto const props = ManagedResource<drmModeObjectPropertiesPtr>{
            drmModeObjectGetProperties(drm_fd, res->planes[i], DRM_MODE_OBJECT_PLANE),
            drmModeFreeObjectProperties};
        if (!props)
            continue;
        auto const type = drm_props_get_value(drm_fd, props, "type").value_or(0);
        if (type != DRM_PLANE_TYPE_PRIMARY)
            continue;
        auto const blob_id = drm_props_get_value(drm_fd, props, "IN_FORMATS").value_or(0);
        if (!blob_id)
            continue;
        auto const blob = ManagedResource<drmModePropertyBlobPtr>{
            drmModeGetPropertyBlob(drm_fd, blob_id),
            drmModeFreePropertyBlob};
        if (!blob)
            continue;

        auto const data = static_cast<struct drm_format_modifier_blob*>(blob->data);
        auto const fmts = reinterpret_cast<uint32_t*>(
            reinterpret_cast<char*>(data) + data->formats_offset);
        auto const mods = reinterpret_cast<struct drm_format_modifier*>(
            reinterpret_cast<char*>(data) + data->modifiers_offset);
        auto const fmt_p = std::find(fmts, fmts + data->count_formats, format);
        if (fmt_p == fmts + data->count_formats)
            continue;
        auto const fmt_mask = 1 << (fmt_p - fmts);

        for (uint32_t m = 0; m < data->count_modifiers; ++m)
        {
            if (mods[m].formats & fmt_mask)
                supported_mods.push_back(mods[m].modifier);
        }

        break;
    }

    return supported_mods;
}

class GetFormatProperties2Dispatcher
{
public:
    GetFormatProperties2Dispatcher(vk::Instance const& instance) :
        vkGetPhysicalDeviceFormatProperties2KHR(
            PFN_vkGetPhysicalDeviceFormatProperties2KHR(
                vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFormatProperties2KHR")))
    {
    }

    size_t getVkHeaderVersion() const
    {
        return VK_HEADER_VERSION;
    }

    PFN_vkGetPhysicalDeviceFormatProperties2KHR vkGetPhysicalDeviceFormatProperties2KHR;
};

std::vector<uint64_t> vk_get_supported_mods_for_format(VulkanState& vulkan,
                                                       vk::Format format)
{
    std::vector<uint64_t> mods;
    auto const dispatch = GetFormatProperties2Dispatcher{vulkan.instance()};
    vk::DrmFormatModifierPropertiesEXT mod_props[256];

    auto mod_props_list = vk::DrmFormatModifierPropertiesListEXT{};
    mod_props_list.drmFormatModifierCount = 256;
    mod_props_list.pDrmFormatModifierProperties = mod_props;
    auto props = vk::FormatProperties2KHR{};
    props.pNext = &mod_props_list;
    vulkan.physical_device().getFormatProperties2KHR(format, &props, dispatch);

    for (uint32_t i = 0; i < mod_props_list.drmFormatModifierCount; ++i)
        mods.push_back(mod_props_list.pDrmFormatModifierProperties[i].drmFormatModifier);

    return mods;
}

}

VTState::VTState(std::string const& tty)
    : vt_fd{open_active_vt(tty)}
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
                                 std::string const& tty,
                                 vk::PresentModeKHR present_mode)
    : drm_fd{open_drm_device(drm_device)},
      drm_resources{get_resources_for(drm_fd)},
      drm_connector{get_connected_connector(drm_fd, drm_resources)},
      drm_prev_crtc{get_current_crtc_for_connector(drm_fd, drm_connector)},
      drm_crtc{get_crtc_for_connector(drm_fd, drm_resources, drm_connector)},
      gbm{create_gbm_device(drm_fd)},
      vk_extent{drm_crtc->mode.hdisplay, drm_crtc->mode.vdisplay},
      vt_state{tty},
      vulkan{nullptr},
      vk_image_format{vk::Format::eUndefined},
      current_image_index{0},
      has_crtc_been_set{false},
      present_mode{present_mode},
      flipped_image_index{-1},
      presented_image_index{-1}
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
    create_vk_submit_fences();
}

void KMSWindowSystem::deinit_vulkan()
{
    vulkan->device().waitIdle();

    vk_images.clear();
    drm_fbs.clear();
    gbm_bos.clear();
    vk_submit_fences.clear();
}

int32_t KMSWindowSystem::get_free_image_index()
{
    for (int32_t i = 0; i < static_cast<int32_t>(vk_images.size()); ++i)
    {
        if (i != presented_image_index && i != flipped_image_index &&
            i != current_image_index)
        {
            return i;
        }
    }

    return -1;
}

VulkanImage KMSWindowSystem::next_vulkan_image()
{
    return {
        static_cast<uint32_t>(current_image_index),
        vk_images[current_image_index], vk_image_format, vk_extent, nullptr,
        vk_submit_fences[current_image_index]
    };
}

void KMSWindowSystem::flip(uint32_t image_index)
{
    auto const& fb = drm_fbs[image_index];

    if (!has_crtc_been_set)
    {
        auto const ret = drmModeSetCrtc(
            drm_fd, drm_crtc->crtc_id, fb, 0, 0, &drm_connector->connector_id, 1,
            &drm_crtc->mode);
        if (ret < 0)
            throw std::system_error{-ret, std::system_category(), "Failed to set crtc"};

        has_crtc_been_set = true;
    }

    auto const ret = drmModePageFlip(drm_fd, drm_crtc->crtc_id, fb,
                                     DRM_MODE_PAGE_FLIP_EVENT, this);
    if (ret < 0)
        throw std::system_error{-ret, std::system_category(), "Failed to page flip"};
}

void KMSWindowSystem::present_vulkan_image(VulkanImage const& vulkan_image)
{
    static uint64_t const one_sec = 1000000000;

    (void)vulkan->device().waitForFences(vulkan_image.submit_fence, true, one_sec);
    vulkan->device().resetFences(vulkan_image.submit_fence);

    if (present_mode == vk::PresentModeKHR::eMailbox)
    {
        wait_for_drm_page_flip_event(0);
    }
    else
    {
        while (flipped_image_index >= 0)
            wait_for_drm_page_flip_event(-1);
    }

    if (flipped_image_index == -1)
    {
        flip(vulkan_image.index);
        flipped_image_index = vulkan_image.index;
    }

    while ((current_image_index = get_free_image_index()) < 0)
        wait_for_drm_page_flip_event(-1);
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
    auto const drm_format = DRM_FORMAT_XRGB8888;
    auto drm_mods = drm_get_supported_mods_for_format(drm_fd, drm_format);
    auto vk_mods = vk_get_supported_mods_for_format(*vulkan, vk_image_format);
    std::vector<uint64_t> mods;
    int num_bos = (present_mode == vk::PresentModeKHR::eMailbox) ? 4 : 3;

    // Find the modifiers supported by both DRM and Vulkan
    for (auto mod : drm_mods)
    {
        if (std::find(vk_mods.begin(), vk_mods.end(), mod) != vk_mods.end())
            mods.push_back(mod);
    }

    for (int i = 0; i < num_bos; ++i)
    {
        struct gbm_bo *bo_raw;

        if (mods.empty())
        {
            bo_raw = gbm_bo_create(
                gbm, vk_extent.width, vk_extent.height, drm_format,
                GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
        }
        else
        {
#if HAVE_GBM_BO_CREATE_WITH_MODIFIERS2
        bo_raw = gbm_bo_create_with_modifiers2(
            gbm, vk_extent.width, vk_extent.height, drm_format,
            mods.data(), mods.size(),
            GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
#else
        bo_raw = gbm_bo_create_with_modifiers(
            gbm, vk_extent.width, vk_extent.height, drm_format,
            mods.data(), mods.size());
#endif
        }

        if (!bo_raw)
            throw std::runtime_error{"Failed to create gbm bo"};

        gbm_bos.push_back(
            ManagedResource<gbm_bo*>{std::move(bo_raw), gbm_bo_destroy});
    }

    Log::debug("KMSWindowSystem: Created buffers with format=0x%x mod=0x%lx\n",
               gbm_bo_get_format(gbm_bos[0].raw),
               gbm_bo_get_modifier(gbm_bos[0].raw));
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
                       vk::ImageTiling::eOptimal)
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

void KMSWindowSystem::create_vk_submit_fences()
{
    for (unsigned int i = 0; i < vk_images.size(); ++i)
    {
        vk_submit_fences.push_back(ManagedResource<vk::Fence>{
            vulkan->device().createFence(vk::FenceCreateInfo{}),
            [this] (auto& f) { vulkan->device().destroyFence(f); }});
    }
}

void KMSWindowSystem::wait_for_drm_page_flip_event(int timeout_ms)
{
    static int constexpr drm_event_context_version = 2;
    static drmEventContext event_context = {
        drm_event_context_version,
        nullptr,
        page_flip_handler};

    pollfd pfd{drm_fd, POLLIN, 0};

    while (true)
    {
        auto const ret = poll(&pfd, 1, timeout_ms);
        if (ret < 0)
        {
            if (errno == EINTR)
                continue;

            throw std::system_error{
                errno, std::system_category(), "Failed while polling for pages flip event"};
        }
        else if (ret == 0) // timeout
        {
            break;
        }
        else if (pfd.revents & POLLIN)
        {
            drmHandleEvent(drm_fd, &event_context);
            break;
        }

        throw std::runtime_error{"Failed while polling for pages flip event"};
    }
}

void KMSWindowSystem::page_flip_handler(int, unsigned int, unsigned int, unsigned int, void* data)
{
    auto* ws = reinterpret_cast<KMSWindowSystem*>(data);
    //fprintf(stderr, "presented_image %d\n", ws->flipped_image_index);
    ws->presented_image_index = ws->flipped_image_index;
    ws->flipped_image_index = -1;
}

VulkanWSI::Extensions KMSWindowSystem::required_extensions()
{
    return {{VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
             VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME},
            {VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
             VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
             VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME,
             VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
             VK_KHR_MAINTENANCE1_EXTENSION_NAME,
             VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
             VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,
             VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
             VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME,
             VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME}};
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
