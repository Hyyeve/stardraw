#pragma once
#include <string>
#include <string_view>

#include "starlib/types/starlib_stdint.hpp"

namespace stardraw
{
    struct object_identifier
    {
        object_identifier() : object_identifier("???") {}
        // ReSharper disable once CppNonExplicitConvertingConstructor
        object_identifier(const std::string_view& string) : hash(std::hash<std::string_view>()(string)), name(string) {}
        // ReSharper disable once CppNonExplicitConvertingConstructor
        object_identifier(const char* string) : hash(std::hash<std::string>()(std::string(string))), name(string) {}

        bool operator==(const object_identifier&) const = default;

        starlib_stdint::u64 hash;
        std::string name;
    };
}
