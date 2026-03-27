#include "draw_specification_state.hpp"
namespace stardraw::gl45
{
    draw_specification_state::draw_specification_state(const draw_configuration& descriptor, const bool has_index_buffer) : shader(descriptor.shader), vertex_specification(descriptor.vertex_specification), has_index_buffer(has_index_buffer), framebuffer(descriptor.framebuffer) {}

    descriptor_type draw_specification_state::object_type() const
    {
        return descriptor_type::DRAW_CONFIGURATION;
    }
}

