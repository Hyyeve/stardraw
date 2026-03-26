#include "buffer_state.hpp"

#include <format>
#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>
#include "../common.hpp"
#include "../staging_buffer_uploader.hpp"
#include "stardraw/api/memory_transfer.hpp"

namespace stardraw::gl45
{
    buffer_state::buffer_state(const buffer& desc, status& out_status)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Create buffer object");

        buffer_identifier = desc.identifier();

        glCreateBuffers(1, &main_buffer_id);
        if (main_buffer_id == 0)
        {
            out_status = {status_type::BACKEND_ERROR, std::format("Creating buffer {0} failed", desc.identifier().name)};
            return;
        }

        const GLbitfield flags = (desc.memory == buffer_memory_storage::SYSRAM) ? GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT | GL_CLIENT_STORAGE_BIT : 0;

        main_buffer_size = desc.size;
        glNamedBufferStorage(main_buffer_id, main_buffer_size, nullptr, flags);
        out_status = status_type::SUCCESS;
    }

    buffer_state::~buffer_state()
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Delete buffer object");
        glDeleteBuffers(1, &main_buffer_id);
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
        TracyGpuZone("[Stardraw] Bind buffer");
        glBindBuffer(target, main_buffer_id);
        return status_type::SUCCESS;
    }

    status buffer_state::bind_to_slot(const GLenum target, const GLuint slot) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Bind buffer (slot binding)");
        glBindBufferRange(target, slot, main_buffer_id, 0, main_buffer_size);
        return status_type::SUCCESS;
    }

    status buffer_state::bind_to_slot(const GLenum target, const GLuint slot, const GLintptr address, const GLsizeiptr bytes) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Bind buffer (slot binding)");
        if (!is_in_buffer_range(address, bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested bind range is out of range in buffer '{0}'", buffer_identifier.name)};
        glBindBufferRange(target, slot, main_buffer_id, address, bytes);
        return status_type::SUCCESS;
    }

    status buffer_state::prepare_upload_data_streaming(const GLintptr address, const GLintptr bytes, memory_transfer_handle** out_handle)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Prepare direct buffer upload");
        if (!is_in_buffer_range(address, bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested upload range is out of range in buffer '{0}'", buffer_identifier.name)};

        gl_memory_transfer_handle* staged_handle = nullptr;
        status allocate_status = staging_uploader.allocate_upload(address, bytes, main_buffer_size, &staged_handle);
        if (allocate_status.is_error()) return allocate_status;
        *out_handle = staged_handle;
        return status_type::SUCCESS;
    }

    status buffer_state::flush_upload_data_streaming(memory_transfer_handle* handle) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Flush staged buffer upload");
        const gl_memory_transfer_handle* staged_handle = dynamic_cast<gl_memory_transfer_handle*>(handle);
        if (staged_handle == nullptr) return {status_type::INVALID, "Invalid memory transfer handle cast - this is an internal bug!"};
        status copy_status = copy_data(staged_handle->transfer_buffer_id, staged_handle->transfer_buffer_address, staged_handle->transfer_destination_address, staged_handle->transfer_size);
        if (copy_status.is_error()) return copy_status;
        return staging_buffer_uploader::flush_upload(staged_handle);
    }

    status buffer_state::prepare_upload_data_chunked(const GLintptr address, const GLintptr bytes, memory_transfer_handle** out_handle)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Prepare staged buffer upload");
        if (!is_in_buffer_range(address, bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested upload range is out of range in buffer '{0}'", buffer_identifier.name)};

        GLuint temp_buffer;
        glCreateBuffers(1, &temp_buffer);
        if (temp_buffer == 0) return {status_type::BACKEND_ERROR, std::format("Unable to create temporary upload destination for buffer '{0}'", buffer_identifier.name)};

        glNamedBufferStorage(temp_buffer, bytes, nullptr, GL_MAP_WRITE_BIT);
        GLbyte* temp_buffer_ptr = static_cast<GLbyte*>(glMapNamedBuffer(temp_buffer, GL_WRITE_ONLY));
        if (temp_buffer_ptr == nullptr)
        {
            glDeleteBuffers(1, &temp_buffer);
            return {status_type::BACKEND_ERROR, std::format("Unable to write to temporary upload destination for buffer '{0}'", buffer_identifier.name)};
        }

        gl_memory_transfer_handle* handle = new gl_memory_transfer_handle();
        handle->transfer_size = bytes;
        handle->transfer_destination_address = address;
        handle->transfer_buffer_id = temp_buffer;
        handle->transfer_buffer_ptr = temp_buffer_ptr;
        handle->transfer_buffer_address = 0;
        *out_handle = handle;
        return status_type::SUCCESS;
    }

    status buffer_state::flush_upload_data_chunked(memory_transfer_handle* handle) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Flush staged buffer upload");
        const gl_memory_transfer_handle* chunked_handle = dynamic_cast<gl_memory_transfer_handle*>(handle);
        if (chunked_handle == nullptr) return {status_type::INVALID, "Invalid memory transfer handle cast - this is an internal bug!"};
        glUnmapNamedBuffer(chunked_handle->transfer_buffer_id);
        status copy_status = copy_data(chunked_handle->transfer_buffer_id, chunked_handle->transfer_buffer_address, chunked_handle->transfer_destination_address, chunked_handle->transfer_size);
        glDeleteBuffers(1, &chunked_handle->transfer_buffer_id);
        delete handle;
        return copy_status;
    }

    status buffer_state::prepare_upload_data_unchecked(const GLintptr address, const GLintptr bytes, memory_transfer_handle** out_handle)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Prepare direct buffer upload");
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
    status buffer_state::flush_upload_data_unchecked(const memory_transfer_handle* handle) // NOLINT(*-convert-member-functions-to-static)
    {
        ZoneScoped;
        delete handle;
        return status_type::SUCCESS;
    }

    status buffer_state::copy_data(const GLuint source_buffer_id, const GLintptr read_address, const GLintptr write_address, const GLintptr bytes) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Buffer data transfer");

        if (!is_in_buffer_range(read_address, bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested upload range is out of range in buffer '{0}'", buffer_identifier.name)};
        glCopyNamedBufferSubData(source_buffer_id, main_buffer_id, read_address, write_address, bytes);
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
        main_buff_pointer = glMapNamedBufferRange(main_buffer_id, 0, main_buffer_size, flags);
        if (main_buff_pointer == nullptr) return {status_type::BACKEND_ERROR, std::format("Unable to write directly to buffer '{0}' (you probably need to create it with the SYSRAM memory hint?)", buffer_identifier.name)};
        return status_type::SUCCESS;
    }
}