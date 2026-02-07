#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "polymorphic_list.hpp"

namespace stardraw
{
    enum class graphics_api : uint8_t
    {
        GL45,
    };

    enum class signal_status : uint8_t
    {
        SIGNALLED, NOT_SIGNALLED, TIMED_OUT, UNKNOWN_SIGNAL
    };

    enum class status_type : uint8_t
    {
        SUCCESS, UNSUPPORTED, UNIMPLEMENTED, NOTHING_TO_DO,
        ALREADY_INITIALIZED, NOT_INITIALIZED,
        UNKNOWN_SOURCE, UNEXPECTED_NULL, RANGE_OVERFLOW, TIMEOUT,
        DUPLICATE_NAME, UNKNOWN_NAME,
        BACKEND_ERROR, BROKEN_SOURCE,
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
        uint64_t hash;
        std::string name;
    };





}

