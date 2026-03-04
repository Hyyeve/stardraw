#pragma once
#include <memory>
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

    enum class signal_status : u8
    {
        SIGNALLED, NOT_SIGNALLED, TIMED_OUT, UNKNOWN_SIGNAL, CONTEXT_ERROR
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
    };

    inline bool is_status_error(const status& status)
    {
        switch (status.type)
        {
            case status_type::NOTHING_TO_DO:
            case status_type::SUCCESS: return false;
            default: return true;
        }
    }

    struct object_identifier
    {
        explicit object_identifier(const std::string_view& string) : hash(std::hash<std::string_view>()(string)), name(string) {}
        u64 hash;
        std::string name;
    };


}

