#include "object_states.hpp"

#include <format>

#include "tracy/Tracy.hpp"
#include "tracy/TracyOpenGL.hpp"

namespace stardraw::gl45
{
    buffer_state::buffer_state(const buffer_descriptor& desc)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Create buffer object");

        buffer_name = desc.identifier().name;

        glCreateBuffers(1, &main_buffer_id);
        if (main_buffer_id == 0) return;

        const GLbitfield flags = (desc.memory == buffer_memory_storage::SYSRAM) ? GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT | GL_CLIENT_STORAGE_BIT : 0;

        main_buffer_size = desc.size;
        glNamedBufferStorage(main_buffer_id, main_buffer_size, nullptr, flags);
    }

    buffer_state::~buffer_state()
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Delete buffer object");

        if (staging_buff_pointer != nullptr)
        {
            glUnmapNamedBuffer(staging_buffer_id);
            staging_buff_pointer = nullptr;
        }

        if (staging_buffer_id != 0)
        {
            glDeleteBuffers(1, &staging_buffer_id);
        }

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
        if (!is_in_buffer_range(address, bytes)) return  {status_type::RANGE_OVERFLOW, std::format("Requested bind range is out of range in buffer '{0}'", buffer_name)};
        glBindBufferRange(target, slot, main_buffer_id, address, bytes);
        return status_type::SUCCESS;
    }

    status buffer_state::upload_data_direct(const GLintptr address, const void* const data, const GLsizeiptr bytes)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Direct buffer upload");
        if (!is_in_buffer_range(address, bytes)) return  {status_type::RANGE_OVERFLOW, std::format("Requested upload range is out of range in buffer '{0}'", buffer_name)};

        if (main_buff_pointer == nullptr)
        {
            const status map_status = map_main_buffer();
            if (is_error_status(map_status)) return map_status;
        }

        const GLbyte* source_pointer = static_cast<const GLbyte*>(data);
        GLbyte* dest_pointer = static_cast<GLbyte*>(main_buff_pointer);
        memcpy(dest_pointer + address, source_pointer, bytes);
        return status_type::SUCCESS;
    }

    status buffer_state::upload_data_staged(const GLintptr address, const void* const data, const GLintptr bytes)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Staged buffer upload");
        if (!is_in_buffer_range(address, bytes)) return  {status_type::RANGE_OVERFLOW, std::format("Requested upload range is out of range in buffer '{0}'", buffer_name)};

        update_staging_buffer_space(); //Clean up any free blocks ahead of us

        if (bytes > remaining_staging_buffer_space)
        {
            //Not enough space - try wrapping to the start of the buffer and cleaning up more blocks
            current_staging_buff_address = 0;
            remaining_staging_buffer_space = 0;
            update_staging_buffer_space();

            if (bytes > remaining_staging_buffer_space)
            {
                //Still not enough - try and create new staging buffer.
                const status prepared = prepare_staging_buffer(std::min(bytes * 3, main_buffer_size));
                if (is_error_status(prepared)) return prepared;
            }
        }

        GLbyte* upload_buffer_pointer = static_cast<GLbyte*>(staging_buff_pointer);
        const GLbyte* source_pointer = static_cast<const GLbyte*>(data);
        memcpy(upload_buffer_pointer + current_staging_buff_address, source_pointer, bytes);

        const status copy_status = copy_data(staging_buffer_id, current_staging_buff_address, address, bytes);

        upload_chunks.emplace(current_staging_buff_address, bytes, glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));
        current_staging_buff_address += bytes;
        remaining_staging_buffer_space -= bytes;

        return copy_status;
    }

    status buffer_state::upload_data_temp_copy(const GLintptr address, const void* const data, const GLintptr bytes) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Temp copy buffer upload");
        if (!is_in_buffer_range(address, bytes)) return  {status_type::RANGE_OVERFLOW, std::format("Requested upload range is out of range in buffer '{0}'", buffer_name)};

        GLuint temp_buffer;
        glCreateBuffers(1, &temp_buffer);
        if (temp_buffer == 0) return  { status_type::BACKEND_FAILURE, std::format("Unable to create temporary upload destination for buffer '{0}'", buffer_name) };

        glNamedBufferStorage(temp_buffer, bytes, nullptr, GL_MAP_WRITE_BIT);
        GLbyte* pointer = static_cast<GLbyte*>(glMapNamedBuffer(temp_buffer, GL_WRITE_ONLY));
        if (pointer == nullptr)
        {
            glDeleteBuffers(1, &temp_buffer);
            return { status_type::BACKEND_FAILURE, std::format("Unable to write to temporary upload destination for buffer '{0}'", buffer_name) };
        }

        memcpy(pointer, data, bytes);

        const bool unmap_success = glUnmapNamedBuffer(temp_buffer);
        if (!unmap_success)
        {
            glDeleteBuffers(1, &temp_buffer);
            return { status_type::BACKEND_FAILURE, std::format("Unable to write to temporary upload destination for buffer '{0}'", buffer_name) };
        }

        const status copy_status = copy_data(temp_buffer, 0, address, bytes);
        glDeleteBuffers(1, &temp_buffer);

        return copy_status;
    }

    status buffer_state::copy_data(const GLuint source_buffer_id, const GLintptr read_address, const GLintptr write_address, const GLintptr bytes) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Buffer data transfer");

        if (!is_in_buffer_range(read_address, bytes)) return  {status_type::RANGE_OVERFLOW, std::format("Requested upload range is out of range in buffer '{0}'", buffer_name)};
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

    status buffer_state::prepare_staging_buffer(const GLsizeiptr size)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Allocate staging buffer");

        if (staging_buff_pointer != nullptr)
        {
            glUnmapNamedBuffer(staging_buffer_id);
            staging_buff_pointer = nullptr;
        }

        if (staging_buffer_id != 0)
        {
            glDeleteBuffers(1, &staging_buffer_id);
        }

        while (!upload_chunks.empty())
        {
            const upload_chunk& chunk = upload_chunks.front();
            upload_chunks.pop();
            glDeleteSync(chunk.fence);
        }

        staging_buffer_size = size;
        current_staging_buff_address = 0;
        remaining_staging_buffer_space = size;

        glCreateBuffers(1, &staging_buffer_id);
        if (staging_buffer_id == 0) return  { status_type::BACKEND_FAILURE, std::format("Unable to create staging buffer for buffer '{0}'", buffer_name)};

        constexpr GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        glNamedBufferStorage(staging_buffer_id, staging_buffer_size, nullptr, flags);

        staging_buff_pointer = glMapNamedBufferRange(staging_buffer_id, 0, staging_buffer_size, flags);
        if (staging_buffer_id == 0) return { status_type::BACKEND_FAILURE, std::format("Unable to create staging buffer for buffer '{0}'", buffer_name)};

        return status_type::SUCCESS;
    }

    void buffer_state::update_staging_buffer_space()
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Staging buffer free space check");

        while (true)
        {
            if (upload_chunks.empty()) break;

            const upload_chunk& previous_chunk = upload_chunks.front();
            if (previous_chunk.address < current_staging_buff_address) break;

            const GLenum status = glClientWaitSync(previous_chunk.fence, 0, 0);
            if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED)
            {
                upload_chunks.pop();
                remaining_staging_buffer_space += previous_chunk.size;
            }
            else
            {
                break;
            }
        }
    }

    status buffer_state::map_main_buffer()
    {
        if (main_buff_pointer != nullptr) return status_type::NOTHING_TO_DO;
        constexpr GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        main_buff_pointer = glMapNamedBufferRange(main_buffer_id, 0, main_buffer_size, flags);
        if (main_buff_pointer == nullptr) return  { status_type::BACKEND_FAILURE, std::format("Unable to write directly to buffer '{0}' (you probably need to create it with the SYSRAM memory hint?)", buffer_name) };
        return status_type::SUCCESS;
    }

    vertex_specification_state::vertex_specification_state()
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Create vertex specification");
        glCreateVertexArrays(1, &vertex_array_id);
    }

    vertex_specification_state::~vertex_specification_state()
    {
        glDeleteVertexArrays(1, &vertex_array_id);
    }

    bool vertex_specification_state::is_valid() const
    {
        ZoneScoped;
        if (vertex_array_id == 0) return false;
        for (const GLuint buffer : vertex_buffers)
        {
            if (!glIsBuffer(buffer)) return false;
        }

        if (index_buffer != 0 && !glIsBuffer(index_buffer)) return false;

        return true;
    }

    status vertex_specification_state::bind() const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Bind vertex specification");
        glBindVertexArray(vertex_array_id);
        return status_type::SUCCESS;
    }

    status vertex_specification_state::attach_vertex_buffer(const GLuint slot, const GLuint id, const GLintptr offset, const GLsizei stride)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Attach vertex buffer to vertex specification");
        glVertexArrayVertexBuffer(vertex_array_id, slot, id, offset, stride);
        vertex_buffers.push_back(id);
        return status_type::SUCCESS;

    }

    status vertex_specification_state::attach_index_buffer(const GLuint index_buffer_id)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Attach index buffer to vertex specification");
        glVertexArrayElementBuffer(vertex_array_id, index_buffer_id);
        index_buffer = index_buffer_id;
        return status_type::SUCCESS;
    }
}
