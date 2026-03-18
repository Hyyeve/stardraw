#pragma once

#include "stardraw/api/descriptors.hpp"
#include "stardraw/api/memory_transfer.hpp"
#include "stardraw/gl45/gl_headers.hpp"

namespace stardraw::gl45
{
    #pragma pack(push, 1)
    struct draw_arrays_indirect_params
    {
        u32 vertex_count;
        u32 instance_count;
        u32 vertex_begin;
        u32 instance_begin;
    };

    struct draw_elements_indirect_params
    {
        u32 vertex_count;
        u32 instance_count;
        u32 index_begin;
        i32 vertex_begin;
        u32 instance_begin;
    };

    struct dispatch_compute_indirect_params {
        u32  num_groups_x;
        u32  num_groups_y;
        u32  num_groups_z;
    };

    #pragma pack(pop)

    class object_state
    {
    public:
        virtual ~object_state() = default;
        [[nodiscard]] virtual descriptor_type object_type() const = 0;
    };

    struct signal_state
    {
        GLsync sync_point;
    };

    class gl_memory_transfer_handle final : public memory_transfer_handle
    {
    public:
        status transfer(void* data) override
        {
            if (current_status != memory_transfer_status::READY) return {status_type::INVALID, "Transfer has already been called on this handle!"};
            current_status = memory_transfer_status::TRANSFERRING;
            memcpy(transfer_buffer_ptr, data, transfer_size);
            current_status = memory_transfer_status::COMPLETE;
            return status_type::SUCCESS;
        }
        memory_transfer_status transfer_status() override
        {
            return current_status;
        }

        ~gl_memory_transfer_handle() override = default;

        void* transfer_buffer_ptr = nullptr;
        GLuint transfer_buffer_id = 0;
        u64 transfer_buffer_address = 0;
        u64 transfer_destination_address = 0;
        u64 transfer_size = 0;
        GLsync* sync_ptr = nullptr;
        std::atomic<stardraw::memory_transfer_status> current_status = memory_transfer_status::READY;
    };
}
