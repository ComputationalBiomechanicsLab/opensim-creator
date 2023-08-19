#pragma once

#include <cstdint>
#include <functional>

namespace osc
{
    struct alignas(alignof(uint32_t)) Rgba32 final {

        static constexpr size_t length() noexcept
        {
            return 4;
        }

        uint8_t& operator[](ptrdiff_t i)
        {
            return (&r)[i];
        }

        uint8_t const& operator[](ptrdiff_t i) const
        {
            return (&r)[i];
        }

        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

    constexpr bool operator==(Rgba32 const& a, Rgba32 const& b) noexcept
    {
        return
            a.r == b.r &&
            a.g == b.g &&
            a.b == b.b &&
            a.a == b.a;
    }

    constexpr bool operator!=(Rgba32 const& a, Rgba32 const& b) noexcept
    {
        return !(a == b);
    }

    inline uint32_t ToUint32(Rgba32 const& rgba32)
    {
        static_assert(alignof(Rgba32) == alignof(uint32_t));
        static_assert(sizeof(Rgba32) == sizeof(uint32_t));
        return reinterpret_cast<uint32_t const&>(rgba32);
    }
}

namespace std
{
    template<>
    struct hash<osc::Rgba32> final {
        size_t operator()(osc::Rgba32 const& rgba32) const noexcept
        {
            return std::hash<uint32_t>{}(ToUint32(rgba32));
        }
    };
}
