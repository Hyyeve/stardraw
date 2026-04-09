#include "transfer_buffer_state.hpp"

#include <format>
#include <queue>
#include <ranges>

#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>

#include "../gl_headers.hpp"
#include "stardraw/internal/internal.hpp"
#include "starlib/types/block_allocator.hpp"

namespace stardraw::gl45
{
    transfer_buffer_state::transfer_buffer_state(const transfer_buffer& descriptor, status& out_status) : buffer_size(descriptor.size), usage(descriptor.usage), buffer_id(descriptor.identifier())
    {
        out_status = allocate_buffer();
    }

    status transfer_buffer_state::allocate_upload(const u64 address, const u64 bytes, gl_memory_transfer_handle** out_handle)
    {
        ZoneScoped;

        if (usage == transfer_buffer_usage::DOWNLOAD_ONLY) return {status_type::INVALID, std::format("Transfer buffer '{0}' cannot be used for uploads! (do you need to change the usage flag?)", buffer_id.name)};

        clean_chunks();
        u64 chunk_address;
        const bool has_space = chunk_allocator.try_allocate(bytes, chunk_address);
        if (!has_space) return {status_type::RANGE_OVERFLOW, std::format("Not enough space in transfer buffer '{0}'!", buffer_id.name)};

        chunks.emplace_back(new upload_chunk(address, current_buffer_id, nullptr));
        buffer_refcounts[current_buffer_id]++;
        gl_memory_transfer_handle* handle = new gl_memory_transfer_handle();
        handle->transfer_buffer_ptr = current_buffer_ptr + chunk_address;
        handle->transfer_size = bytes;
        handle->transfer_buffer_address = chunk_address;
        handle->transfer_destination_address = address;
        handle->transfer_buffer_id = current_buffer_id;
        handle->sync_ptr = &chunks.back()->fence;
        *out_handle = handle;

        return status_type::SUCCESS;
    }

    u64 transfer_buffer_state::get_buffer_size() const
    {
        return buffer_size;
    }

    bool transfer_buffer_state::check_can_allocate(const u64 size) const
    {
        return chunk_allocator.can_allocate(size);
    }

    status transfer_buffer_state::flush_upload(const gl_memory_transfer_handle* handle)
    {
        ZoneScoped;
        {
            ZoneScopedN("TracyGpuZone");
        }
        {
            ZoneScopedN("GL calls");
            *handle->sync_ptr = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        }
        delete handle;

        {
            ZoneScopedN("Destructors");
            return status_type::SUCCESS;
        }
    }

    transfer_buffer_state::~transfer_buffer_state()
    {
        ZoneScoped;
        for (const auto& staging_buff : buffer_refcounts | std::views::keys)
        {
            {
                ZoneScopedN("GL calls");
                glDeleteBuffers(1, &staging_buff);
            }
        }
    }

    void transfer_buffer_state::clean_chunks()
    {
        ZoneScoped;
        std::erase_if(chunks, [this](const upload_chunk* chunk)
        {
            if (chunk->fence == nullptr) return false;

            GLenum status;
            {
                ZoneScopedN("GL calls");
                status = glClientWaitSync(chunk->fence, 0, 0);
            }
            if (status != GL_ALREADY_SIGNALED && status != GL_CONDITION_SATISFIED) return false;

            if (chunk->staging_buffer_id == current_buffer_id) chunk_allocator.free(chunk->address);

            buffer_refcounts[chunk->staging_buffer_id]--;
            if (buffer_refcounts[chunk->staging_buffer_id] <= 0 && chunk->staging_buffer_id != current_buffer_id)
            {
                {
                    ZoneScopedN("GL calls");
                    glDeleteBuffers(1, &chunk->staging_buffer_id);
                }
                buffer_refcounts.erase(chunk->staging_buffer_id);
            }

            delete chunk;

            return true;
        });
    }

    GLbitfield usage_to_flags(const transfer_buffer_usage usage)
    {
        switch (usage)
        {
            case transfer_buffer_usage::UPLOAD_ONLY: return GL_MAP_WRITE_BIT;
            case transfer_buffer_usage::DOWNLOAD_ONLY: return GL_MAP_READ_BIT;
            case transfer_buffer_usage::UPLOAD_OR_DOWNLOAD: return GL_MAP_WRITE_BIT | GL_MAP_READ_BIT;
        }

        return 0;
    }

    descriptor_type transfer_buffer_state::object_type() const
    {
        return descriptor_type::TRANSFER_BUFFER;
    }

    status transfer_buffer_state::allocate_buffer()
    {
        ZoneScoped;

        {
            ZoneScopedN("GL calls");
            glCreateBuffers(1, &current_buffer_id);
        }
        if (current_buffer_id == 0) return {status_type::BACKEND_ERROR, std::format("Unable to allocate transfer buffer '{0}'", buffer_id.name)};

        const GLbitfield flags = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | usage_to_flags(usage);

        {
            ZoneScopedN("GL calls");
            glNamedBufferStorage(current_buffer_id, buffer_size, nullptr, flags);

            //Ideally, we would rely on the coherent mapping bit exclusively, and not flush manually.
            //However, automatic flushing is broken in some cases, notably for pixel buffer transfer operations when uploading textures
            //- at least, on my laptop RTX 3060. Is that a driver bug? Probably! The spec for coherent mapping is somewhat unclear.
            current_buffer_ptr = static_cast<GLbyte*>(glMapNamedBufferRange(current_buffer_id, 0, buffer_size, flags));
        }

        if (current_buffer_ptr == nullptr)
        {
            {
                ZoneScopedN("GL calls");
                glDeleteBuffers(1, &current_buffer_id);
            }
            current_buffer_id = 0;
            current_buffer_ptr = nullptr;
            buffer_size = 0;
            return {status_type::BACKEND_ERROR, std::format("Unable to map transfer buffer '{0}'", buffer_id.name)};
        }

        buffer_refcounts[current_buffer_id] = 0;

        chunk_allocator.resize(buffer_size);
        chunk_allocator.clear();
        return status_type::SUCCESS;
    }
}
