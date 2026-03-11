#pragma once
#include "../common.hpp"
namespace stardraw::gl45
{
    class draw_specification_state final : public object_state
    {
    public:
        explicit draw_specification_state(const draw_specification_descriptor& descriptor, bool has_index_buffer);
        [[nodiscard]] descriptor_type object_type() const override;

        object_identifier shader;
        object_identifier vertex_specification;
        bool has_index_buffer;
    };

}
