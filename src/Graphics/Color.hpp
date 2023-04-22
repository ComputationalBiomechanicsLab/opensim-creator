#pragma once

#include "src/Graphics/Rgba32.hpp"

#include <glm/vec4.hpp>

#include <cstdint>

namespace osc
{
    // representation of RGBA, usually in sRGB color space, with a range of 0 to 1
    struct Color final {

        explicit constexpr Color(glm::vec4 const& v) :
            r{v.x}, g{v.y}, b{v.z}, a{v.w}
        {
        }

        constexpr Color(float r_, float g_, float b_, float a_) :
            r{r_}, g{g_}, b{b_}, a{a_}
        {
        }

        constexpr operator glm::vec4 () const noexcept
        {
            return glm::vec4{r, g, b, a};
        }

        static constexpr Color blue()
        {
            return {0.0f, 0.0f, 1.0f, 1.0f};
        }

        static constexpr Color clear()
        {
            return {0.0f, 0.0f, 0.0f, 0.0f};
        }

        static constexpr Color red()
        {
            return {1.0f, 0.0f, 0.0f, 1.0f};
        }

        float r;
        float g;
        float b;
        float a;
    };

    constexpr bool operator==(Color const& a, Color const& b) noexcept
    {
        return
            a.r == b.r &&
            a.g == b.g &&
            a.b == b.b &&
            a.a == b.a;
    }

    constexpr bool operator!=(Color const& a, Color const& b) noexcept
    {
        return !(a == b);
    }

    // returns the linear version of a (presumed to be) sRGB color
    Color ToLinear(Color const&) noexcept;

    // returns a color that is the (presumed to be) linear color with the sRGB gamma curve applied
    Color ToSRGB(Color const&) noexcept;

    // returns a Vec4 version of a Color
    inline glm::vec4 ToVec4(Color const& c) noexcept
    {
        return glm::vec4{c};
    }

    // returns a pointer to the first float element in the color (used by ImGui etc.)
    constexpr float const* ValuePtr(Color const& color)
    {
        return &color.r;
    }

    // returns a pointer to the first float element in the color (used by ImGui etc.)
    constexpr float* ValuePtr(Color& color)
    {
        return &color.r;
    }

    // float-/double-based inputs assume normalized color range (i.e. 0 to 1)
    Rgba32 ToRgba32(glm::vec4 const&) noexcept;
    Rgba32 ToRgba32(float, float, float, float) noexcept;
    Rgba32 ToRgba32(uint32_t) noexcept;  // R at MSB
}
