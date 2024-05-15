#pragma once

#include <oscar/Graphics/Unorm8.h>
#include <oscar/Shims/Cpp20/bit.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace osc
{
    // Representation of an LDR RGBA color as four `Unorm8` bytes. The color
    // space of the color isn't prescribed, but is usually sRGB.
    struct alignas(alignof(uint32_t)) Color32 final {

        using value_type = Unorm8;
        using reference = value_type&;
        using const_reference = const value_type&;
        using size_type = size_t;
        using const_iterator = const value_type*;

        static constexpr size_type length()
        {
            return 4;
        }

        constexpr reference operator[](size_type pos)
        {
            return (&r)[pos];
        }

        constexpr const_reference operator[](size_type pos) const
        {
            return (&r)[pos];
        }

        constexpr Color32() = default;

        constexpr Color32(value_type r_, value_type g_, value_type b_, value_type a_) :
            r{r_}, g{g_}, b{b_}, a{a_}
        {
        }

        constexpr const_iterator begin() const
        {
            return &r;
        }

        constexpr const_iterator end() const
        {
            return &r + length();
        }

        friend bool operator==(const Color32&, const Color32&) = default;

        Unorm8 r{};
        Unorm8 g{};
        Unorm8 b{};
        Unorm8 a{};
    };

    template<std::integral T>
    T to_integer(const Color32& color32)
    {
        return cpp20::bit_cast<T>(color32);
    }
}

template<>
struct std::hash<osc::Color32> final {
    size_t operator()(const osc::Color32& color32) const
    {
        return std::hash<uint32_t>{}(to_integer<uint32_t>(color32));
    }
};
