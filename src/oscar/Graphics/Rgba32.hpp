#pragma once

#include <cstdint>
#include <functional>

namespace osc
{
    struct alignas(alignof(uint32_t)) Rgba32 final {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

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
