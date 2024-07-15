#pragma once

#include <cstddef>
#include <string_view>

namespace osc
{
    // a `std::hash`-like object that can transparently hash any object that is
    // implicitly convertible to a `std::string_view`
    struct TransparentStringViewHasher final {

        // important: this is how associative containers check that the hasher
        // doesn't need to create keys at runtime (e.g. using a `std::string_view`
        // to `.find` an `std::unordered_map<std::string, T>` won't require
        // constructing a `std::string`)
        using is_transparent = void;

        size_t operator()(std::string_view sv) const noexcept
        {
            return std::hash<std::string_view>{}(sv);
        }
    };
}
