#pragma once

#include "../api/types.hpp"
#include <slang.h>

namespace stardraw
{
    struct shader_cursor
    {
        slang::TypeLayoutReflection* type_layout = nullptr;
        int32_t byte_address = 0;
        uint32_t binding_slot = 0;
        uint32_t binding_slot_index = 0;

        [[nodiscard]] shader_cursor index(uint32_t index) const;
        [[nodiscard]] shader_cursor field(const std::string_view& name) const;
    };

    constexpr shader_cursor invalid_location = {
        nullptr, -1, 0, 0
    };
}
