#include "staging_buffer_uploader.hpp"

#include <format>
#include <queue>
#include <ranges>

#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>

#include "gl_headers.hpp"
#include "starlib/types/block_allocator.hpp"

namespace stardraw::gl45
{
    status staging_buffer_uploader::allocate_upload(const u64 address, const u64 bytes, const u64 max_staging_buffer_size, gl_memory_transfer_handle** out_handle)
    {
        clean_chunks();
        u64 chunk_address;
        bool has_space = chunk_allocator.try_allocate(bytes, chunk_address);
        if (!has_space)
        {
            status new_buffer_status = allocate_new_staging_buffer(std::min(std::max(bytes, active_staging_buffer_size) * 3, max_staging_buffer_size));
            if (new_buffer_status.is_error()) return new_buffer_status;
            has_space = chunk_allocator.try_allocate(bytes, chunk_address);
        }

        if (!has_space) return {status_type::BACKEND_ERROR, "Unable to allocate space for staged memory transfer"};

        chunks.emplace_back(new upload_chunk(address, active_staging_buffer_id, nullptr));
        staging_buffer_refcounts[active_staging_buffer_id]++;
        gl_memory_transfer_handle* handle = new gl_memory_transfer_handle();
        handle->transfer_buffer_ptr = active_staging_buffer_ptr + chunk_address;
        handle->transfer_size = bytes;
        handle->transfer_buffer_address = chunk_address;
        handle->transfer_destination_address = address;
        handle->transfer_buffer_id = active_staging_buffer_id;
        handle->sync_ptr = &chunks.back()->fence;
        *out_handle = handle;

        return status_type::SUCCESS;
    }

    status staging_buffer_uploader::flush_upload(const gl_memory_transfer_handle* handle)
    {
        *handle->sync_ptr = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        delete handle;
        return status_type::SUCCESS;
    }

    staging_buffer_uploader::~staging_buffer_uploader()
    {
        for (const auto& staging_buff : staging_buffer_refcounts | std::views::keys)
        {
            glDeleteBuffers(1, &staging_buff);
        }
    }

    void staging_buffer_uploader::clean_chunks()
    {
        ZoneScoped;
        std::erase_if(chunks, [this](const upload_chunk* chunk)
        {
            if (chunk->fence == nullptr) return false;
            const GLenum status = glClientWaitSync(chunk->fence, 0, 0);
            if (status != GL_ALREADY_SIGNALED && status != GL_CONDITION_SATISFIED) return false;

            if (chunk->staging_buffer_id == active_staging_buffer_id) chunk_allocator.free(chunk->address);

            staging_buffer_refcounts[chunk->staging_buffer_id]--;
            if (staging_buffer_refcounts[chunk->staging_buffer_id] <= 0 && chunk->staging_buffer_id != active_staging_buffer_id)
            {
                glDeleteBuffers(1, &chunk->staging_buffer_id);
                staging_buffer_refcounts.erase(chunk->staging_buffer_id);
            }

            delete chunk;

            return true;
        });
    }

    status staging_buffer_uploader::allocate_new_staging_buffer(const u64 size)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Allocate staging buffer");

        glCreateBuffers(1, &active_staging_buffer_id);
        if (active_staging_buffer_id == 0) return {status_type::BACKEND_ERROR, std::format("Unable to create staging buffer for upload")};

        constexpr GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        glNamedBufferStorage(active_staging_buffer_id, size, nullptr, flags);

        active_staging_buffer_ptr = static_cast<GLbyte*>(glMapNamedBufferRange(active_staging_buffer_id, 0, size, flags));
        if (active_staging_buffer_ptr == nullptr)
        {
            glDeleteBuffers(1, &active_staging_buffer_id);
            active_staging_buffer_id = 0;
            active_staging_buffer_ptr = nullptr;
            active_staging_buffer_size = 0;
            return {status_type::BACKEND_ERROR, std::format("Unable to map staging buffer for upload")};
        }

        staging_buffer_refcounts[active_staging_buffer_id] = 0;

        chunk_allocator.resize(size);
        chunk_allocator.clear();
        return status_type::SUCCESS;
    }
}
