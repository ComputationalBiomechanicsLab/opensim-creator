#pragma once

#include <liboscar/utilities/shared_pre_hashed_string.h>
#include <liboscar/utilities/string_name.h>

#include <cstddef>
#include <string_view>

namespace osc
{
    // a `std::hash`-like object that can transparently hash any object that is
    // string-like (i.e. implicitly converts into a `std::string_view`)
    struct TransparentStringHasher {

        using is_transparent = void;  // C++20, `std::unordered_map` uses this

        size_t operator()(std::string_view string_view) const noexcept
        {
            // if something implicitly converts into a `std::string_view` then it's
            // eligible for transparent hashing
            return std::hash<std::string_view>{}(string_view);
        }

        size_t operator()(const SharedPreHashedString& shared_pre_hashed_string) const noexcept
        {
            // special case: `SharedPreHashedString`s are pre-hashed
            return std::hash<SharedPreHashedString>{}(shared_pre_hashed_string);
        }

        size_t operator()(const StringName& string_name) const noexcept
        {
            // special case: `StringName`s are pre-hashed
            return std::hash<StringName>{}(string_name);
        }
    };
}
