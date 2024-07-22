#pragma once

#include <oscar/Utils/SharedPreHashedString.h>
#include <oscar/Utils/StringName.h>

#include <cstddef>
#include <string_view>

namespace osc
{
    // a `std::hash`-like object that can transparently hash any object that is
    // string-like (i.e. implicitly converts into a `std::string_view`)
    struct TransparentStringHasher {

        using is_transparent = void;  // C++20, `std::unordered_map` uses this

        size_t operator()(std::string_view sv) const noexcept
        {
            // if something implicitly converts into a `std::string_view` then it's
            // eligible for transparent hashing
            return std::hash<std::string_view>{}(sv);
        }

        size_t operator()(const SharedPreHashedString& sn) const noexcept
        {
            // special case: `SharedPreHashedString`s are pre-hashed
            return std::hash<SharedPreHashedString>{}(sn);
        }

        size_t operator()(const StringName& sn) const noexcept
        {
            // special case: `StringName`s are pre-hashed
            return std::hash<StringName>{}(sn);
        }
    };
}
