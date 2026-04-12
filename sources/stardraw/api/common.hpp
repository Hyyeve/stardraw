#pragma once
#include <string>
#include <string_view>

#include "starlib/general/stdint.hpp"

namespace stardraw
{
    ///Used internally to identify objects created from descriptors. Usually constructed automatically.
    struct object_identifier
    {
        object_identifier() : object_identifier("<unspecified>") {}
        // ReSharper disable once CppNonExplicitConvertingConstructor
        object_identifier(const std::string_view& string) : hash(std::hash<std::string_view>()(string)), name(string) {}
        // ReSharper disable once CppNonExplicitConvertingConstructor
        object_identifier(const char* string) : hash(std::hash<std::string>()(std::string(string))), name(string) {}

        bool operator==(const object_identifier&) const = default;

        starlib::u64 hash;
        std::string name;
    };
}
