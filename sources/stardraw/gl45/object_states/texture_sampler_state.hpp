#pragma once
#include "stardraw/api/descriptors.hpp"
#include "stardraw/gl45/common.hpp"

namespace stardraw::gl45
{
    class texture_sampler_state final : public gl45::object_state
    {
    public:
        explicit texture_sampler_state(const sampler& desc, status& out_status);
        ~texture_sampler_state() override;

        [[nodiscard]] bool is_valid() const;
        [[nodiscard]] descriptor_type object_type() const override;
        [[nodiscard]] status bind(u32 slot) const;


    private:
        status set_sampling_config(const texture_sampling_conifg& config, const bool is_integer_type) const;
        GLuint gl_id;
        object_identifier sampler_id;
    };
}