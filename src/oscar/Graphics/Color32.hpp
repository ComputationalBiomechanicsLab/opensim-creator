#pragma once

#include <oscar/Shims/Cpp20/bit.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>

namespace osc
{
    struct alignas(alignof(uint32_t)) Color32 final {

        static constexpr size_t length()
        {
            return 4;
        }

        constexpr uint8_t& operator[](ptrdiff_t i)
        {
            return (&r)[i];
        }

        constexpr uint8_t const& operator[](ptrdiff_t i) const
        {
            return (&r)[i];
        }

        uint8_t const* begin() const
        {
            return &r;
        }

        uint8_t const* end() const
        {
            return &a + 1;
        }

        friend bool operator==(Color32 const&, Color32 const&) = default;

        uint32_t toU32() const
        {
            return bit_cast<uint32_t>(*this);
        }

        uint8_t r = 0x00;
        uint8_t g = 0x00;
        uint8_t b = 0x00;
        uint8_t a = 0x00;
    };
}

template<>
struct std::hash<osc::Color32> final {
    size_t operator()(osc::Color32 const& color32) const
    {
        return std::hash<uint32_t>{}(color32.toU32());
    }
};
