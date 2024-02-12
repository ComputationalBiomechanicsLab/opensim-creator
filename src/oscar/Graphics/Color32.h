#pragma once

#include <oscar/Graphics/Unorm8.h>
#include <oscar/Shims/Cpp20/bit.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace osc
{
    // Representation of an LDR RGBA color as four Unorm8 bytes. The color
    // space of the color isn't prescribed, but is usually sRGB.
    struct alignas(alignof(uint32_t)) Color32 final {

        using value_type = Unorm8;
        using reference = value_type&;
        using const_reference = value_type const&;
        using length_type = size_t;
        using size_type = size_t;
        using const_iterator = value_type const*;

        static constexpr length_type length()
        {
            return 4;
        }

        constexpr reference operator[](size_type i)
        {
            return (&r)[i];
        }

        constexpr const_reference operator[](size_type i) const
        {
            return (&r)[i];
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

        friend bool operator==(Color32 const&, Color32 const&) = default;

        Unorm8 r{};
        Unorm8 g{};
        Unorm8 b{};
        Unorm8 a{};
    };

    template<std::integral T>
    T to_integer(Color32 const& color32)
    {
        return cpp20::bit_cast<T>(color32);
    }
}

template<>
struct std::hash<osc::Color32> final {
    size_t operator()(osc::Color32 const& color32) const
    {
        return std::hash<uint32_t>{}(to_integer<uint32_t>(color32));
    }
};
