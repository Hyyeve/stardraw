#include "texture_state.hpp"
#include <format>
#include <spirv_glsl.hpp>
#include "stardraw/api/memory_transfer.hpp"
#include "stardraw/gl45/api_conversion.hpp"
#include "tracy/Tracy.hpp"
#include "tracy/TracyOpenGL.hpp"

namespace stardraw::gl45
{
    bool is_sampling_config_valid_for_type(const texture_data_type type, const texture_sampling_conifg& sampling_config)
    {
        ZoneScoped;
        const bool is_integer_type = is_texture_data_type_integer(type);
        if (!is_integer_type) return true;

        if (sampling_config.upscale_filter != texture_filtering_mode::NEAREST) return false;
        if (sampling_config.downscale_filter != texture_filtering_mode::NEAREST) return false;
        if (sampling_config.mipmap_filter != texture_filtering_mode::NEAREST) return false;

        return true;
    }

    texture_state::texture_state(const texture_descriptor& desc, status& out_status)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Create texture object");

        const status initial_status = initalize_and_validate_texture_descriptor(desc);
        if (initial_status.is_error())
        {
            out_status = initial_status;
            return;
        }

        const u32 width = desc.format.width;
        const u32 height = desc.format.height;
        const u32 depth = desc.format.depth;
        const bool has_msaa = desc.format.msaa != texture_msaa_level::NONE;

        root_data_store_texture = desc.identifier(); //This texture stores data, the root store is itself.
        glCreateTextures(gl_texture_target, 1, &gl_texture_id);

        if (gl_texture_id == 0)
        {
            out_status = {status_type::BACKEND_ERROR, std::format("Creating texture {0} failed", desc.identifier().name)};
            return;
        }

        switch (desc.format.shape)
        {
            case texture_shape::_1D:
            {
                if (num_texture_array_layers > 1)
                    glTextureStorage2D(gl_texture_id, num_texture_mipmap_levels, gl_texture_format, width, num_texture_array_layers);
                else
                    glTextureStorage1D(gl_texture_id, num_texture_mipmap_levels, gl_texture_format, width);
                break;
            }
            case texture_shape::_3D:
            {
                glTextureStorage3D(gl_texture_id, num_texture_mipmap_levels, gl_texture_format, width, height, depth);
                break;
            }
            case texture_shape::_2D:
            case texture_shape::CUBE_MAP:
            {
                if (num_texture_array_layers > 1 && has_msaa)
                {
                    //It's unclear whether the last parameter (fixed sample locations) is actually used by any implementations, so for now I'm not supporting customizing it.
                    glTextureStorage3DMultisample(gl_texture_id, num_texture_msaa_samples, gl_texture_format, width, height, num_texture_array_layers, GL_TRUE);
                    break;
                }

                if (num_texture_array_layers > 1)
                {
                    glTextureStorage3D(gl_texture_id, num_texture_mipmap_levels, gl_texture_format, width, height, num_texture_array_layers);
                    break;
                }

                if (has_msaa)
                {
                    //It's unclear whether the last parameter (fixed sample locations) is actually used by any implementations, so for now I'm not supporting customizing it.
                    glTextureStorage2DMultisample(gl_texture_id, num_texture_msaa_samples, gl_texture_format, width, height, GL_TRUE);
                    break;
                }

                glTextureStorage2D(gl_texture_id, num_texture_mipmap_levels, gl_texture_format, width, height);
                break;
            }
        }

        texture_sampling_conifg sampling_config = desc.default_sampling_config;
        sampling_config.mipmap_max_level = std::min(sampling_config.mipmap_max_level, num_texture_mipmap_levels - 1);
        out_status = set_sampling_config(sampling_config, is_texture_data_type_integer(desc.format.data_type));
    }

    texture_state::texture_state(const texture_state* original, const texture_descriptor& desc, status& out_status)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Create texture object");

        const status compatibility_status = original->is_view_compatible(desc);
        if (compatibility_status.is_error())
        {
            out_status = compatibility_status;
            return;
        }

        const status initial_status = initalize_and_validate_texture_descriptor(desc);
        if (initial_status.is_error())
        {
            out_status = initial_status;
            return;
        }

        root_data_store_texture = original->root_data_store_texture; //This texture is NOT the original, the root data store is the data store of the original.
        glGenTextures(1, &gl_texture_id);

        if (gl_texture_id == 0)
        {
            out_status = {status_type::BACKEND_ERROR, std::format("Creating texture view {0} failed", desc.identifier().name)};
            return;
        }

        glTextureView(gl_texture_id, gl_texture_target, original->gl_texture_id, gl_texture_format, desc.format.view_texture_base_mipmap, desc.format.mipmap_levels, desc.format.view_texture_base_layer, num_texture_array_layers);

        texture_sampling_conifg sampling_config = desc.default_sampling_config;
        sampling_config.mipmap_max_level = std::min(sampling_config.mipmap_max_level, num_texture_mipmap_levels - 1);
        out_status = set_sampling_config(sampling_config, is_texture_data_type_integer(original->data_type));
    }

    texture_state::~texture_state()
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Delete texture object");
        glDeleteTextures(1, &gl_texture_id);
    }

    status texture_state::unpack_pixels(const u32 mipmap_level, const u32 x, const u32 y, const u32 z, const u32 width, const u32 height, const u32 depth, const GLenum format, const GLenum gl_data_type) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Unpack texture data");
        texture_shape effective_shape = shape;
        if (num_texture_array_layers > 1 && shape == texture_shape::_1D) effective_shape = texture_shape::_2D;
        if (num_texture_array_layers > 1 && shape == texture_shape::_2D) effective_shape = texture_shape::_3D;

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        switch (effective_shape)
        {
            case texture_shape::_1D:
            {
                glTextureSubImage1D(gl_texture_id, mipmap_level, x, width, format, gl_data_type, nullptr);
                break;
            }
            case texture_shape::_2D:
            {
                glTextureSubImage2D(gl_texture_id, mipmap_level, x, y, width, height, format, gl_data_type, nullptr);
                break;
            }
            case texture_shape::_3D:
            case texture_shape::CUBE_MAP:
            {
                glTextureSubImage3D(gl_texture_id, mipmap_level, x, y, z, width, height, depth, format, gl_data_type, nullptr);
                break;
            }
        }
        return status_type::SUCCESS;
    }

    status texture_state::copy_pixels(const texture_state* read_texture, const texture_copy_info& copy_info) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Copy texture data");
        if (!is_view_format_compatible(gl_texture_format, read_texture->gl_texture_format)) return {status_type::INVALID, "Can't transfer between textures; incompatible data formats"};
        if (!is_view_target_compatible(gl_texture_target, read_texture->gl_texture_target)) return {status_type::INVALID, "Can't transfer between textures; incompatible texture shapes"};

        if (read_texture->num_texture_msaa_samples != num_texture_msaa_samples)
        {
            return {status_type::INVALID, "Can't transfer between textures; number of MSAA samples doesn't match"};
        }

        const u32 read_x = copy_info.read_x;
        const u32 read_y = copy_info.read_y;
        const u32 read_z = copy_info.read_z;
        const u32 write_x = copy_info.write_x;
        const u32 write_y = copy_info.write_y;
        const u32 write_z = copy_info.write_z;

        if (read_x + copy_info.copy_width > read_texture->size.x || read_y + copy_info.copy_height > read_texture->size.y || read_z + copy_info.copy_depth > read_texture->size.z)
        {
            return {status_type::RANGE_OVERFLOW, "Texture copy dimensions outside the bounds of the read texture"};
        }

        if (write_x + copy_info.copy_width > size.x || write_y + copy_info.copy_height > size.y || write_z + copy_info.copy_depth > size.z)
        {
            return {status_type::RANGE_OVERFLOW, "Texture copy dimensions outside the bounds of the write texture"};
        }

        if (copy_info.read_mipmap_level >= read_texture->num_texture_mipmap_levels) return {status_type::RANGE_OVERFLOW, "Texture copy mipmap level is outside the bounds of the read texture"};
        if (copy_info.write_mipmap_level >= num_texture_mipmap_levels) return {status_type::RANGE_OVERFLOW, "Texture copy mipmap level is outside the bounds of the write texture"};

        if (copy_info.read_layer + copy_info.copy_layers >= read_texture->num_texture_array_layers) return {status_type::RANGE_OVERFLOW, "Texture copy array layers are outside the bounds of the read texture"};
        if (copy_info.write_layer + copy_info.copy_layers >= num_texture_array_layers) return {status_type::RANGE_OVERFLOW, "Texture copy array layers are outside the bounds of the write texture"};

        switch (read_texture->shape)
        {
            case texture_shape::_1D: //Copy between 1D textures (or 1D texture arrays)
            {
                glCopyImageSubData(read_texture->gl_texture_id, read_texture->gl_texture_target, copy_info.read_mipmap_level, read_x, copy_info.read_layer, 0,
                                   gl_texture_id, gl_texture_target, copy_info.write_mipmap_level, write_x, copy_info.write_layer, 0, copy_info.copy_width, copy_info.copy_layers, 0);
                break;
            }
            case texture_shape::_2D: //Copy between 2D textures (or 2D texture arrays. Cubemaps are a special kind of 2D texture array)
            case texture_shape::CUBE_MAP:
            {
                glCopyImageSubData(read_texture->gl_texture_id, read_texture->gl_texture_target, copy_info.read_mipmap_level, read_x, read_y, copy_info.read_layer,
                                   gl_texture_id, gl_texture_target, copy_info.write_mipmap_level, write_x, write_y, copy_info.write_layer, copy_info.copy_width, copy_info.copy_height, copy_info.copy_layers);
                break;
            }
            case texture_shape::_3D: //Copy between 3D textures
            {
                glCopyImageSubData(read_texture->gl_texture_id, read_texture->gl_texture_target, copy_info.read_mipmap_level, read_x, read_y, read_z,
                                   gl_texture_id, gl_texture_target, copy_info.write_mipmap_level, write_x, write_y, write_z, copy_info.copy_width, copy_info.copy_height, copy_info.copy_depth);
                break;
            }
        }

        return status_type::SUCCESS;
    }

    status texture_state::prepare_upload(const texture_memory_transfer_info& info, memory_transfer_handle** out_handle) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Prepare texture upload");

        if (info.x + info.width > size.x || info.y + info.height > size.y || info.z + info.depth > size.z)
        {
            return {status_type::RANGE_OVERFLOW, "Texture upload dimensions outside the bounds of the texture"};
        }

        if (info.mipmap_level >= num_texture_mipmap_levels) return {status_type::RANGE_OVERFLOW, "Texture upload mipmap level is outside the bounds of the texture"};
        if (info.layer + info.layers > num_texture_array_layers || info.layers < 1) return {status_type::RANGE_OVERFLOW, "Texture upload array layers are outside the bounds of the texture"};

        if (info.channels == texture_memory_transfer_info::pixel_channels::STENCIL && !does_texture_data_type_have_stencil(data_type)) return {status_type::INVALID, "Texture upload channels is set to stencil, but this texture does not contain stencil data!"};
        if (info.channels == texture_memory_transfer_info::pixel_channels::DEPTH && !does_texture_data_type_have_depth(data_type)) return {status_type::INVALID, "Texture upload channels is set to depth, but this texture does not contain depth data!"};

        GLuint temp_buffer;
        glCreateBuffers(1, &temp_buffer);
        if (temp_buffer == 0) return {status_type::BACKEND_ERROR, std::format("Unable to create temporary upload destination for texture")};

        const u64 bytes = compute_bytes_in_transfer(info);

        glNamedBufferStorage(temp_buffer, bytes, nullptr, GL_MAP_WRITE_BIT);
        GLbyte* temp_buffer_ptr = static_cast<GLbyte*>(glMapNamedBuffer(temp_buffer, GL_WRITE_ONLY));
        if (temp_buffer_ptr == nullptr)
        {
            glDeleteBuffers(1, &temp_buffer);
            return {status_type::BACKEND_ERROR, std::format("Unable to write to temporary upload destination for texture")};
        }

        gl_memory_transfer_handle* handle = new gl_memory_transfer_handle();
        handle->transfer_size = bytes;
        handle->transfer_destination_address = 0;
        handle->transfer_buffer_id = temp_buffer;
        handle->transfer_buffer_ptr = temp_buffer_ptr;
        handle->transfer_buffer_address = 0;
        *out_handle = handle;
        return status_type::SUCCESS;
    }

    status texture_state::flush_upload(const texture_memory_transfer_info& info, memory_transfer_handle* handle) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Flush texture upload");
        const gl_memory_transfer_handle* gl_handle = dynamic_cast<gl_memory_transfer_handle*>(handle);
        if (gl_handle == nullptr) return {status_type::INVALID, "Invalid memory transfer handle cast - this is an internal bug!"};
        glUnmapNamedBuffer(gl_handle->transfer_buffer_id);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gl_handle->transfer_buffer_id);
        status unpack_status = unpack_pixels(info.mipmap_level, info.x, info.y, info.z, info.width, info.height, info.depth, to_gl_channels_format(info.channels), to_gl_memory_transfer_data_type(info.data_type));
        glDeleteBuffers(1, &gl_handle->transfer_buffer_id);
        delete handle;
        return unpack_status;
    }

    u64 texture_state::compute_bytes_in_transfer(const texture_memory_transfer_info& info) const
    {
        ZoneScoped;
        switch (shape)
        {
            case texture_shape::_1D: return info.width * info.layers * bytes_per_pixel;
            case texture_shape::_2D: return info.width * info.height * info.layers * bytes_per_pixel;
            case texture_shape::_3D: return info.width * info.height * info.depth * info.layers * bytes_per_pixel;
            case texture_shape::CUBE_MAP: return info.width * info.height * info.layers * bytes_per_pixel;
            default: return -1;
        }
    }

    status texture_state::initalize_and_validate_texture_descriptor(const texture_descriptor& desc)
    {
        ZoneScoped;
        num_texture_mipmap_levels = desc.format.mipmap_levels;
        num_texture_array_layers = desc.format.layers;
        shape = desc.format.shape;
        data_type = desc.format.data_type;
        bytes_per_pixel = bytes_per_texture_data_element(data_type);
        size = {desc.format.width, desc.format.height, desc.format.depth};
        num_texture_msaa_samples = static_cast<u8>(desc.format.msaa);

        const bool has_msaa = desc.format.msaa != texture_msaa_level::NONE;
        const bool can_type_be_array = desc.format.shape != texture_shape::_3D;
        const bool can_shape_be_multisample = desc.format.shape == texture_shape::_2D || desc.format.shape == texture_shape::_3D;
        const bool is_array = (desc.format.shape != texture_shape::CUBE_MAP && num_texture_array_layers > 1) || num_texture_array_layers > 6;

        gl_texture_target = to_gl_texture_target(desc.format.shape, has_msaa, is_array);
        gl_texture_format = to_gl_texture_format(desc.format.data_type);

        if (size.x <= 0 || size.y <= 0 || size.z <= 0)
        {
            return {status_type::INVALID, std::format("Texture '{0}' has zero size in an axis", desc.identifier().name)};
        }

        if (num_texture_mipmap_levels <= 0)
        {
            return {status_type::INVALID, std::format("Texture '{0}' has zero mipmap layers", desc.identifier().name)};
        }

        if (num_texture_array_layers <= 0)
        {
            return {status_type::INVALID, std::format("Texture '{0}' has zero texture layers", desc.identifier().name)};
        }

        if (!can_shape_be_multisample && has_msaa)
        {
            return {status_type::INVALID, std::format("Texture '{0}' shape cannot have MSAA, but msaa level is not zero", desc.identifier().name)};
        }

        if (!can_type_be_array && is_array)
        {
            return {status_type::INVALID, std::format("Texture '{0}' shape cannot be an array texture, but has more than 1 texture layer", desc.identifier().name)};
        }

        if (desc.format.shape == texture_shape::CUBE_MAP && num_texture_array_layers % 6 != 0)
        {
            return {status_type::INVALID, std::format("Texture '{0}' is a cubemap array texture, but number of texture layers is not a multiple of 6", desc.identifier().name)};
        }

        if (!is_sampling_config_valid_for_type(desc.format.data_type, desc.default_sampling_config))
        {
            return {status_type::INVALID, std::format("Provided default sampling config isn't valid for texture '{0}' (are you trying to use linear filtering on an integer texture?)", desc.identifier().name)};
        }

        return status_type::SUCCESS;
    }

    inline void set_texture_border_color(const GLuint texture_id, const texture_border_color border_color, const bool is_integer)
    {
        ZoneScoped;
        if (is_integer)
        {
            switch (border_color)
            {
                case texture_border_color::OPAUQE_BLACK:
                {
                    constexpr std::array col = {0, 0, 0, 1};
                    glTextureParameteriv(texture_id, GL_TEXTURE_BORDER_COLOR, col.data());
                    break;
                }
                case texture_border_color::OPAQUE_WHITE:
                {
                    constexpr std::array col = {1, 1, 1, 1};
                    glTextureParameteriv(texture_id, GL_TEXTURE_BORDER_COLOR, col.data());
                    break;
                }
                case texture_border_color::TRANSPARENT:
                {
                    constexpr std::array col = {0, 0, 0, 0};
                    glTextureParameteriv(texture_id, GL_TEXTURE_BORDER_COLOR, col.data());
                    break;
                }
            }
        }
        else
        {
            switch (border_color)
            {
                case texture_border_color::OPAUQE_BLACK:
                {
                    constexpr std::array col = {0.f, 0.f, 0.f, 1.f};
                    glTextureParameterfv(texture_id, GL_TEXTURE_BORDER_COLOR, col.data());
                    break;
                }
                case texture_border_color::OPAQUE_WHITE:
                {
                    constexpr std::array col = {1.f, 1.f, 1.f, 1.f};
                    glTextureParameterfv(texture_id, GL_TEXTURE_BORDER_COLOR, col.data());
                    break;
                }
                case texture_border_color::TRANSPARENT:
                {
                    constexpr std::array col = {0.f, 0.f, 0.f, 0.f};
                    glTextureParameterfv(texture_id, GL_TEXTURE_BORDER_COLOR, col.data());
                    break;
                }
            }
        }
    }

    bool texture_state::is_valid() const
    {
        ZoneScoped;
        return gl_texture_id != 0 && glIsTexture(gl_texture_id);
    }

    status texture_state::bind_to_texture_slot(const u32 slot) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Bind texture");
        glBindTextureUnit(slot, gl_texture_id);
        return status_type::SUCCESS;
    }

    status texture_state::bind_to_image_slot(const u32 slot, const u32 mipmap_level, const u32 array_layer, const bool entire_array, const GLenum access) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Bind texture as image");
        if (mipmap_level >= num_texture_mipmap_levels) return {status_type::INVALID, "Texture does not contain specified mipmap level for image binding"};
        if (array_layer >= num_texture_array_layers) return {status_type::INVALID, "Texture does not contain specified array layer for image binding"};
        glBindImageTexture(slot, gl_texture_id, mipmap_level, entire_array, array_layer, access, gl_texture_format);
        return status_type::SUCCESS;
    }

    bool texture_state::is_view_format_compatible(const GLenum source_format, const GLenum view_format)
    {
        ZoneScoped;
        const std::array<std::vector<GLenum>, 12> compatible_sets = {
            std::vector<GLenum> {GL_RGBA32F, GL_RGBA32UI, GL_RGBA32I},
            {GL_RGB32F, GL_RGB32UI, GL_RGB32I},
            {GL_RGBA16F, GL_RG32F, GL_RGBA16UI, GL_RG32UI, GL_RGBA16I, GL_RG32I, GL_RGBA16, GL_RGBA16_SNORM},
            {GL_RGB16, GL_RGB16_SNORM, GL_RGB16F, GL_RGB16UI, GL_RGB16I},
            {GL_RG16F, GL_R11F_G11F_B10F, GL_R32F, GL_RGB10_A2UI, GL_RGBA8UI, GL_RG16UI, GL_R32UI, GL_RGBA8I, GL_RG16I, GL_R32I, GL_RGB10_A2, GL_RGBA8, GL_RG16, GL_RGBA8_SNORM, GL_RG16_SNORM, GL_SRGB8_ALPHA8, GL_RGB9_E5},
            {GL_RGB8, GL_RGB8_SNORM, GL_SRGB8, GL_RGB8UI, GL_RGB8I},
            {GL_R16F, GL_RG8UI, GL_R16UI, GL_RG8I, GL_R16I, GL_RG8, GL_R16, GL_RG8_SNORM, GL_R16_SNORM},
            {GL_R8UI, GL_R8I, GL_R8, GL_R8_SNORM},
            {GL_COMPRESSED_RED_RGTC1, GL_COMPRESSED_SIGNED_RED_RGTC1},
            {GL_COMPRESSED_RG_RGTC2, GL_COMPRESSED_SIGNED_RG_RGTC2},
            {GL_COMPRESSED_RGBA_BPTC_UNORM, GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM},
            {GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT, GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT}
        };

        const auto set_ptr = std::ranges::find_if(compatible_sets, [view_format](const std::vector<GLenum>& set)
        {
            return std::ranges::contains(set, view_format);
        });

        if (set_ptr == compatible_sets.end()) return false;
        return std::ranges::contains(*set_ptr, source_format);
    }

    bool texture_state::is_view_target_compatible(const GLenum source_target, const GLenum view_target)
    {
        ZoneScoped;
        const std::array<std::vector<GLenum>, 11> compatible_sets = {
            std::vector<GLenum> {GL_TEXTURE_1D, GL_TEXTURE_1D_ARRAY},
            {GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY},
            {GL_TEXTURE_3D},
            {GL_TEXTURE_CUBE_MAP, GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP_ARRAY},
            {GL_TEXTURE_1D_ARRAY, GL_TEXTURE_1D},
            {GL_TEXTURE_2D_ARRAY, GL_TEXTURE_2D},
            {GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP},
            {GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_2D_MULTISAMPLE_ARRAY},
            {GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_2D_MULTISAMPLE},
        };

        const auto set_ptr = std::ranges::find_if(compatible_sets, [source_target](const std::vector<GLenum>& set)
        {
            return set[0] == source_target; //Target compatibility is not 100% commutable - a 2D texture can view into a cube map, but not vice versa.
        });

        if (set_ptr == compatible_sets.end()) return false;
        return std::ranges::contains(*set_ptr, view_target);
    }

    status texture_state::is_view_compatible(const texture_descriptor& view_descriptor) const
    {
        ZoneScoped;
        const u32 min_view_mipmap = view_descriptor.format.view_texture_base_mipmap;
        const u32 max_view_mipmap = min_view_mipmap + view_descriptor.format.mipmap_levels - 1;

        const u32 min_view_layer = view_descriptor.format.view_texture_base_layer;
        const u32 max_view_layer = min_view_mipmap + view_descriptor.format.layers - 1;

        const bool is_array = (view_descriptor.format.shape != texture_shape::CUBE_MAP && view_descriptor.format.layers > 1) || view_descriptor.format.layers > 6;

        if (min_view_mipmap >= num_texture_mipmap_levels) return {status_type::RANGE_OVERFLOW, std::format("Texture view '{0}' is not compatible with source - source has {1} mipmap levels, but requesting index {2}", view_descriptor.identifier().name, num_texture_mipmap_levels, min_view_mipmap)};
        if (max_view_mipmap >= num_texture_mipmap_levels) return {status_type::RANGE_OVERFLOW, std::format("Texture view '{0}' is not compatible with source - source has {1} mipmap levels, but requesting index {2}", view_descriptor.identifier().name, num_texture_mipmap_levels, max_view_mipmap)};
        if (min_view_layer >= num_texture_array_layers) return {status_type::RANGE_OVERFLOW, std::format("Texture view '{0}' is not compatible with source - source has {1} texture layers, but requesting index {2}", view_descriptor.identifier().name, num_texture_array_layers, min_view_layer)};
        if (max_view_layer >= num_texture_array_layers) return {status_type::RANGE_OVERFLOW, std::format("Texture view '{0}' is not compatible with source - source has {1} texture layers, but requesting index {2}", view_descriptor.identifier().name, num_texture_array_layers, max_view_layer)};
        if (!is_view_format_compatible(gl_texture_format, to_gl_texture_format(view_descriptor.format.data_type))) return {status_type::INVALID, std::format("Texture view '{0}' is not compatible with source - texture data types are not compatible", view_descriptor.identifier().name)};
        if (!is_view_target_compatible(gl_texture_target, to_gl_texture_target(view_descriptor.format.shape, view_descriptor.format.msaa != texture_msaa_level::NONE, is_array))) return {status_type::INVALID, std::format("Texture view '{0}' is not compatible with source - texture shapes are not compatible", view_descriptor.identifier().name)};
        return status_type::SUCCESS;
    }

    status texture_state::set_sampling_config(const texture_sampling_conifg& config, const bool is_integer_texture) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Set texture sampling config");
        glTextureParameteri(gl_texture_id, GL_TEXTURE_MAG_FILTER, to_gl_filter_mode_from_modes(config.upscale_filter, config.mipmap_filter, false));
        glTextureParameteri(gl_texture_id, GL_TEXTURE_MIN_FILTER, to_gl_filter_mode_from_modes(config.downscale_filter, config.mipmap_filter, true));

        glTextureParameteri(gl_texture_id, GL_TEXTURE_WRAP_S, to_gl_wrapping_mode(config.wrapping_mode_x));
        glTextureParameteri(gl_texture_id, GL_TEXTURE_WRAP_T, to_gl_wrapping_mode(config.wrapping_mode_y));
        glTextureParameteri(gl_texture_id, GL_TEXTURE_WRAP_R, to_gl_wrapping_mode(config.wrapping_mode_z));

        set_texture_border_color(gl_texture_id, config.border_color, is_integer_texture);

        if (GLAD_GL_VERSION_4_6 || GLAD_GL_ARB_texture_filter_anisotropic || GLAD_GL_EXT_texture_filter_anisotropic)
        {
            //Anisotropy is not *strictly* core, but it's supported practically universally
            glTextureParameterf(gl_texture_id, GL_TEXTURE_MAX_ANISOTROPY, static_cast<float>(config.anisotropy_level));
        }

        glTextureParameteri(gl_texture_id, GL_TEXTURE_BASE_LEVEL, config.mipmap_min_level);
        glTextureParameteri(gl_texture_id, GL_TEXTURE_MAX_LEVEL, config.mipmap_max_level);
        glTextureParameterf(gl_texture_id, GL_TEXTURE_LOD_BIAS, config.mipmap_bias);

        glTextureParameteri(gl_texture_id, GL_TEXTURE_SWIZZLE_R, to_gl_swizzle_mode(config.swizzling.swizzle_red));
        glTextureParameteri(gl_texture_id, GL_TEXTURE_SWIZZLE_G, to_gl_swizzle_mode(config.swizzling.swizzle_green));
        glTextureParameteri(gl_texture_id, GL_TEXTURE_SWIZZLE_B, to_gl_swizzle_mode(config.swizzling.swizzle_blue));
        glTextureParameteri(gl_texture_id, GL_TEXTURE_SWIZZLE_A, to_gl_swizzle_mode(config.swizzling.swizzle_alpha));

        return status_type::SUCCESS;
    }

    texture_shape texture_state::get_shape() const
    {
        return shape;
    }
}
