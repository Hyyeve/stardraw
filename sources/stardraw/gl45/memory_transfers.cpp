#include "render_context.hpp"

#include "api_conversion.hpp"
#include "stardraw/internal/internal.hpp"
#include "starlib/general/string.hpp"
#include "tracy/TracyOpenGL.hpp"

namespace stardraw::gl45
{
    status render_context::prepare_buffer_memory_transfer(const buffer_memory_transfer_info& info, memory_transfer_handle*& out_handle)
    {
        ZoneScoped;
        buffer_state* buffer;
        status find_status = find_buffer_state(info.target, &buffer);
        if (find_status.is_error()) return find_status;

        switch (info.transfer_type)
        {
            case buffer_memory_transfer_info::type::UPLOAD_STREAMING:
            {
                memory_transfer_handle* handle;
                status prepare_status = buffer->prepare_upload_data_streaming(info.address, info.bytes, &handle);
                if (prepare_status.is_error()) return prepare_status;
                buffer_transfers[handle] = info;
                out_handle = handle;
                return status_type::SUCCESS;
            }
            case buffer_memory_transfer_info::type::UPLOAD_CHUNK:
            {
                memory_transfer_handle* handle;
                status prepare_status = buffer->prepare_upload_data_chunked(info.address, info.bytes, &handle);
                if (prepare_status.is_error()) return prepare_status;
                buffer_transfers[handle] = info;
                out_handle = handle;
                return status_type::SUCCESS;
            }
            case buffer_memory_transfer_info::type::UPLOAD_UNCHECKED:
            {
                memory_transfer_handle* handle;
                status prepare_status = buffer->prepare_upload_data_unchecked(info.address, info.bytes, &handle);
                if (prepare_status.is_error()) return prepare_status;
                buffer_transfers[handle] = info;
                out_handle = handle;
                return status_type::SUCCESS;
            }
            default: return {status_type::UNSUPPORTED};
        }
    }

    status render_context::flush_buffer_memory_transfer(memory_transfer_handle* handle)
    {
        ZoneScoped;
        if (!buffer_transfers.contains(handle)) return {status_type::UNKNOWN, "Memory transfer handle not recognized - did you create it with a different context or type?"};
        const buffer_memory_transfer_info info = buffer_transfers[handle];
        buffer_transfers.erase(handle);

        buffer_state* buffer;
        status find_status = find_buffer_state(info.target, &buffer);
        if (find_status.is_error()) return find_status;

        mem_barrier_controller.barrier_if_needed(info.target, GL_BUFFER_UPDATE_BARRIER_BIT);

        switch (info.transfer_type)
        {
            case buffer_memory_transfer_info::type::UPLOAD_STREAMING: return buffer->flush_upload_data_streaming(handle);
            case buffer_memory_transfer_info::type::UPLOAD_CHUNK: return buffer->flush_upload_data_chunked(handle);
            case buffer_memory_transfer_info::type::UPLOAD_UNCHECKED: return buffer->flush_upload_data_unchecked(handle);

            default: return {status_type::UNSUPPORTED};
        }
    }

    status render_context::prepare_texture_memory_transfer(const texture_memory_transfer_info& info, memory_transfer_handle*& out_handle)
    {
        ZoneScoped;
        texture_state* texture;
        status find_status = find_texture_state(object_identifier(info.target), &texture);
        if (find_status.is_error()) return find_status;

        memory_transfer_handle* handle;
        status prepare_status = texture->prepare_upload(info, &handle);
        if (prepare_status.is_error()) return prepare_status;
        texture_transfers[handle] = info;
        out_handle = handle;
        return status_type::SUCCESS;
    }

    status render_context::flush_texture_memory_transfer(memory_transfer_handle* handle)
    {
        ZoneScoped;
        if (!texture_transfers.contains(handle)) return {status_type::UNKNOWN, "Memory transfer handle not recognized - did you create it with a different context or type?"};
        const texture_memory_transfer_info info = texture_transfers[handle];
        texture_transfers.erase(handle);

        texture_state* texture;
        status find_status = find_texture_state(info.target, &texture);
        if (find_status.is_error()) return find_status;

        mem_barrier_controller.barrier_if_needed(info.target, GL_TEXTURE_UPDATE_BARRIER_BIT);

        return texture->flush_upload(info, handle);
    }
}