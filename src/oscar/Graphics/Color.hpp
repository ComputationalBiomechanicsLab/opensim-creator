#pragma once

#include "oscar/Graphics/Rgba32.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstddef>
#include <cstdint>

namespace osc
{
    // representation of RGBA, usually in sRGB color space, with a range of 0 to 1
    struct Color final {

        using value_type = float;

        static constexpr size_t length() noexcept
        {
            return 4;
        }

        static constexpr Color halfGrey()
        {
            return {0.5f, 0.5f, 0.5f};
        }

        static constexpr Color black()
        {
            return {0.0f, 0.0f, 0.0f};
        }

        static constexpr Color blue()
        {
            return {0.0f, 0.0f, 1.0f};
        }

        static constexpr Color mutedBlue()
        {
            return {0.06f, 0.53f, 0.98f};
        }

        static constexpr Color clear()
        {
            return {0.0f, 0.0f, 0.0f, 0.0f};
        }

        static constexpr Color green()
        {
            return {0.0f, 1.0f, 0.0f};
        }

        static constexpr Color mutedGreen()
        {
            return {0.5f, 1.0f, 0.5f};
        }

        static constexpr Color darkGreen()
        {
            return {0.0f, 0.6f, 0.0f};
        }

        static constexpr Color red()
        {
            return {1.0f, 0.0f, 0.0f};
        }

        static constexpr Color mutedRed()
        {
            return {1.0f, 0.5f, 0.5f};
        }

        static constexpr Color white()
        {
            return {1.0f, 1.0f, 1.0f};
        }

        static constexpr Color yellow()
        {
            return {1.0f, 1.0f, 0.0f};
        }

        static constexpr Color purple()
        {
            return {191.0f/255.0f, 85.0f/255.0f, 236.0f/255.0f};
        }

        Color() = default;

        // i.e. a "solid" color (no transparency)
        explicit constexpr Color(glm::vec3 const& v) :
            r{v.x}, g{v.y}, b{v.z}, a{1.0f}
        {
        }

        explicit constexpr Color(glm::vec4 const& v) :
            r{v.x}, g{v.y}, b{v.z}, a{v.w}
        {
        }

        constexpr Color(float r_, float g_, float b_, float a_) :
            r{r_}, g{g_}, b{b_}, a{a_}
        {
        }

        constexpr Color(float r_, float g_, float b_) :
            r{r_}, g{g_}, b{b_}, a{1.0f}
        {
        }

        constexpr Color& operator*=(Color const& other) noexcept
        {
            r *= other.r;
            g *= other.g;
            b *= other.b;
            a *= other.a;

            return *this;
        }

        constexpr float& operator[](ptrdiff_t i) noexcept
        {
            static_assert(sizeof(Color) == 4*sizeof(float));
            return (&r)[i];
        }

        constexpr float const& operator[](ptrdiff_t i) const noexcept
        {
            static_assert(sizeof(Color) == 4*sizeof(float));
            return (&r)[i];
        }

        constexpr operator glm::vec4 () const noexcept
        {
            return glm::vec4{r, g, b, a};
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

    constexpr Color operator*(Color const& a, Color const& b) noexcept
    {
        return Color
        {
            a.r * b.r,
            a.g * b.g,
            a.b * b.b,
            a.a * b.a,
        };
    }

    // returns the linear version of one (presumed to be) sRGB color channel value
    float ToLinear(float colorChannelValue) noexcept;

    // returns the linear version of one (presumed to be) linear color channel value
    float ToSRGB(float colorChannelValue) noexcept;

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

    // linearly interpolates between `a` and `b` by `t`
    //
    // `t` is clamped to [0.0f, 1.0f]. When `t` is 0, returns `a`. When `t` is 1, returns `b`
    Color Lerp(Color const& a, Color const& b, float t) noexcept;

    // float-/double-based inputs assume normalized color range (i.e. 0 to 1)
    Rgba32 ToRgba32(glm::vec4 const&) noexcept;
    Rgba32 ToRgba32(float, float, float, float) noexcept;
    Rgba32 ToRgba32(uint32_t) noexcept;  // R at MSB
}

// define hashing function for colors
namespace std
{
    // declare a hash function for glm::vec4, so it can be used as a key in
    // unordered maps

    template<>
    struct hash<osc::Color> final {
        size_t operator()(osc::Color const&) const;
    };
}
