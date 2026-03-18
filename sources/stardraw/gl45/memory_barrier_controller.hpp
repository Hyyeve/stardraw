#pragma once

#include "stardraw/gl45/gl_headers.hpp"
#include "tracy/Tracy.hpp"

namespace stardraw::gl45
{
    class memory_barrier_controller
    {
    public:
        inline void barrier_if_needed(const object_identifier& object_id, const GLbitfield barrier_bits)
        {
            ZoneScoped;
            if (!blocking.contains(object_id.hash)) return;
            const GLbitfield blocking_bits = blocking[object_id.hash] & barrier_bits;
            if (blocking_bits == 0) return;

            blocking[object_id.hash] &= ~blocking_bits;
            glMemoryBarrier(blocking_bits);
        }
        inline void flag_barriers(const object_identifier& object_id)
        {
            blocking[object_id.hash] = u32_max; //All bits
        }
    private:
        std::unordered_map<u64, GLbitfield> blocking;
    };
}
