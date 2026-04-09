#include "buffer_state.hpp"

#include <format>
#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>
#include "../common.hpp"
#include "transfer_buffer_state.hpp"
#include "stardraw/api/memory_transfer.hpp"
#include "stardraw/internal/internal.hpp"

namespace stardraw::gl45
{
    buffer_state::buffer_state(const buffer& desc, status& out_status)
    {
        ZoneScoped;

        buffer_identifier = desc.identifier();

        {
            ZoneScopedN("GL calls");
            glCreateBuffers(1, &main_buffer_id);
        }
        if (main_buffer_id == 0)
        {
            out_status = {status_type::BACKEND_ERROR, std::format("Creating buffer {0} failed", desc.identifier().name)};
            return;
        }

        const GLbitfield flags = (desc.memory == buffer_memory_storage::SYSRAM) ? GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT | GL_CLIENT_STORAGE_BIT : 0;

        main_buffer_size = desc.size;
        {
            ZoneScopedN("GL calls");
            glNamedBufferStorage(main_buffer_id, main_buffer_size, nullptr, flags);
        }
        out_status = status_type::SUCCESS;
    }

    buffer_state::~buffer_state()
    {
        ZoneScoped;
        {
            ZoneScopedN("GL calls");
            glDeleteBuffers(1, &main_buffer_id);
        }
    }

    descriptor_type buffer_state::object_type() const
    {
        return descriptor_type::BUFFER;
    }

    bool buffer_state::is_valid() const
    {
        return main_buffer_id != 0;
    }

    status buffer_state::bind_to(const GLenum target) const
    {
        ZoneScoped;
        {
            ZoneScopedN("GL calls");
            glBindBuffer(target, main_buffer_id);
        }
        return status_type::SUCCESS;
    }

    status buffer_state::bind_to_slot(const GLenum target, const GLuint slot) const
    {
        ZoneScoped;
        {
            ZoneScopedN("GL calls");
            glBindBufferRange(target, slot, main_buffer_id, 0, main_buffer_size);
        }
        return status_type::SUCCESS;
    }

    status buffer_state::bind_to_slot(const GLenum target, const GLuint slot, const GLintptr address, const GLsizeiptr bytes) const
    {
        ZoneScoped;
        if (!is_in_buffer_range(address, bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested bind range is out of range in buffer '{0}'", buffer_identifier.name)};

        {
            ZoneScopedN("GL calls");
            glBindBufferRange(target, slot, main_buffer_id, address, bytes);
        }

        return status_type::SUCCESS;
    }

    status buffer_state::prepare_upload_data_via_transfer(transfer_buffer_state* transfer_buffer, const GLintptr address, const GLintptr bytes, memory_transfer_handle** out_handle)
    {
        ZoneScoped;
        if (!is_in_buffer_range(address, bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested upload range is out of range in buffer '{0}'", buffer_identifier.name)};

        gl_memory_transfer_handle* staged_handle = nullptr;
        status allocate_status = transfer_buffer->allocate_upload(address, bytes, &staged_handle);
        if (allocate_status.is_error()) return allocate_status;
        *out_handle = staged_handle;
        return status_type::SUCCESS;
    }

    status buffer_state::flush_upload_data_via_transfer(memory_transfer_handle* handle) const
    {
        ZoneScoped;
        const gl_memory_transfer_handle* staged_handle = dynamic_cast<gl_memory_transfer_handle*>(handle);
        if (staged_handle == nullptr) return {status_type::INVALID, std::format("Invalid memory transfer handle cast - this is an internal bug! (trying to upload to buffer '{0}'", buffer_identifier.name)};
        status copy_status = copy_data(staged_handle->transfer_buffer_id, staged_handle->transfer_buffer_address, staged_handle->transfer_destination_address, staged_handle->transfer_size);
        if (copy_status.is_error()) return copy_status;
        return transfer_buffer_state::flush_upload(staged_handle);
    }

    status buffer_state::prepare_upload_data_unchecked(const GLintptr address, const GLintptr bytes, memory_transfer_handle** out_handle)
    {
        ZoneScoped;
        if (!is_in_buffer_range(address, bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested upload range is out of range in buffer '{0}'", buffer_identifier.name)};

        if (main_buff_pointer == nullptr)
        {
            status map_status = map_main_buffer();
            if (map_status.is_error()) return map_status;
        }

        gl_memory_transfer_handle* handle = new gl_memory_transfer_handle();
        handle->transfer_size = bytes;
        handle->transfer_destination_address = address;
        handle->transfer_buffer_id = main_buffer_size;
        handle->transfer_buffer_ptr = main_buff_pointer;
        handle->transfer_buffer_address = 0;
        *out_handle = handle;
        return status_type::SUCCESS;
    }

    // ReSharper disable once CppMemberFunctionMayBeStatic
    status buffer_state::flush_upload_data_unchecked(const memory_transfer_handle* handle) const // NOLINT(*-convert-member-functions-to-static)
    {
        ZoneScoped;
        delete handle;
        return status_type::SUCCESS;
    }

    status buffer_state::copy_data(const GLuint source_buffer_id, const GLintptr read_address, const GLintptr write_address, const GLintptr bytes) const
    {
        ZoneScoped;

        if (!is_in_buffer_range(read_address, bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested upload range is out of range in buffer '{0}'", buffer_identifier.name)};

        {
            ZoneScopedN("GL calls");
            glCopyNamedBufferSubData(source_buffer_id, main_buffer_id, read_address, write_address, bytes);
        }

        return status_type::SUCCESS;
    }

    GLsizeiptr buffer_state::get_size() const
    {
        return main_buffer_size;
    }

    bool buffer_state::is_in_buffer_range(const GLintptr address, const GLsizeiptr size) const
    {
        return address + size <= get_size();
    }

    GLuint buffer_state::gl_id() const
    {
        return main_buffer_id;
    }

    status buffer_state::map_main_buffer()
    {
        ZoneScoped;
        if (main_buff_pointer != nullptr) return status_type::NOTHING_TO_DO;
        constexpr GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        {
            ZoneScopedN("GL calls");
            main_buff_pointer = glMapNamedBufferRange(main_buffer_id, 0, main_buffer_size, flags);
        }
        if (main_buff_pointer == nullptr) return {status_type::BACKEND_ERROR, std::format("Unable to write directly to buffer '{0}' (you probably need to create it with the SYSRAM memory hint?)", buffer_identifier.name)};
        return status_type::SUCCESS;
    }
}
