#pragma once

#include "oscar/Utils/Cpp20Shims.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>

namespace osc
{
    struct alignas(alignof(uint32_t)) Color32 final {

        static constexpr size_t length() noexcept
        {
            return 4;
        }

        constexpr uint8_t& operator[](ptrdiff_t i) noexcept
        {
            return (&r)[i];
        }

        constexpr uint8_t const& operator[](ptrdiff_t i) const noexcept
        {
            return (&r)[i];
        }

        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

    constexpr bool operator==(Color32 const& a, Color32 const& b) noexcept
    {
        return
            a.r == b.r &&
            a.g == b.g &&
            a.b == b.b &&
            a.a == b.a;
    }

    constexpr bool operator!=(Color32 const& a, Color32 const& b) noexcept
    {
        return !(a == b);
    }

    inline uint32_t ToUint32(Color32 const& color32)
    {
        return bit_cast<uint32_t>(color32);
    }
}

namespace std
{
    template<>
    struct hash<osc::Color32> final {
        size_t operator()(osc::Color32 const& color32) const noexcept
        {
            return std::hash<uint32_t>{}(ToUint32(color32));
        }
    };
}
