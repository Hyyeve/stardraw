#pragma once
#include <string>
#include <string_view>

#include "starlib/types/starlib_stdint.hpp"

namespace stardraw
{
    using namespace starlib_stdint;

    enum class graphics_api : u8
    {
        GL45,
    };

    enum class status_type : u8
    {
        SUCCESS, UNSUPPORTED, UNIMPLEMENTED, NOTHING_TO_DO,
        ALREADY_INITIALIZED, NOT_INITIALIZED,
        UNKNOWN, DUPLICATE, UNEXPECTED, RANGE_OVERFLOW, TIMEOUT,
        BACKEND_ERROR, INVALID,
    };

    struct status
    {
        // ReSharper disable once CppNonExplicitConvertingConstructor
        status(const status_type type) : type{type}, message("No message provided") {}
        status(const status_type type, const std::string_view& message) : type{type}, message(message) {}

        status_type type;
        std::string message;

        inline bool is_error() const
        {
            switch (type)
            {
                case status_type::NOTHING_TO_DO:
                case status_type::SUCCESS: return false;
                default: return true;
            }
        }
    };

    struct object_identifier
    {
        object_identifier() : object_identifier("???") {}
        // ReSharper disable once CppNonExplicitConvertingConstructor
        object_identifier(const std::string_view& string) : hash(std::hash<std::string_view>()(string)), name(string) {}
        // ReSharper disable once CppNonExplicitConvertingConstructor
        object_identifier(const char* string) : hash(std::hash<std::string>()(std::string(string))), name(string) {}

        bool operator==(const object_identifier&) const = default;

        u64 hash;
        std::string name;
    };
}
