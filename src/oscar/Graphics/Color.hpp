#pragma once

#include <oscar/Graphics/Color32.hpp>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

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

        static constexpr Color cyan()
        {
            return {0.0f, 1.0f, 1.0f};
        }

        static constexpr Color magenta()
        {
            return {1.0f, 0.0f, 1.0f};
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

        constexpr friend bool operator==(Color const& lhs, Color const& rhs) noexcept
        {
            return
                lhs.r == rhs.r &&
                lhs.g == rhs.g &&
                lhs.b == rhs.b &&
                lhs.a == rhs.a;
        }

        constexpr friend bool operator!=(Color const& lhs, Color const& rhs) noexcept
        {
            return !(lhs == rhs);
        }

        constexpr friend Color& operator*=(Color& lhs, Color const& rhs) noexcept
        {
            lhs.r *= rhs.r;
            lhs.g *= rhs.g;
            lhs.b *= rhs.b;
            lhs.a *= rhs.a;

            return lhs;
        }

        constexpr friend Color operator*(Color const& lhs, Color const& rhs) noexcept
        {
            return Color
            {
                lhs.r * rhs.r,
                lhs.g * rhs.g,
                lhs.b * rhs.b,
                lhs.a * rhs.a,
            };
        }

        constexpr friend Color operator*(float lhs, Color const& rhs) noexcept
        {
            return Color
            {
                lhs * rhs.r,
                lhs * rhs.g,
                lhs * rhs.b,
                lhs * rhs.a,
            };
        }

        float r;
        float g;
        float b;
        float a;
    };

    // returns the normalized (0.0 - 1.0) floating-point equivalent of the
    // given 8-bit (0 - 255) color channel value
    constexpr float ToFloatingPointColorChannel(uint8_t channelValue) noexcept
    {
        return (1.0f/255.0f) * static_cast<float>(channelValue);
    }

    // returns the 8-bit (0 - 255) equivalent of the given normalized (0.0 - 1.0)
    // floating-point color channel value
    //
    // input values that fall outside of the 0.0 - 1.0 range are clamped to that range
    constexpr uint8_t ToClamped8BitColorChannel(float channelValue) noexcept
    {
        channelValue = channelValue >= 0.0f ? channelValue : 0.0f;
        channelValue = channelValue <= 1.0f ? channelValue : 1.0f;
        return static_cast<uint8_t>(255.0f * channelValue);
    }

    // returns the linear version of one (presumed to be) sRGB color channel value
    float ToLinear(float colorChannelValue) noexcept;

    // returns the linear version of one (presumed to be) linear color channel value
    float ToSRGB(float colorChannelValue) noexcept;

    // returns the linear version of a (presumed to be) sRGB color
    Color ToLinear(Color const&) noexcept;

    // returns a color that is the (presumed to be) linear color with the sRGB gamma curve applied
    Color ToSRGB(Color const&) noexcept;

    // returns a color that is clamped to the low-dynamic range (LDR, i.e. [0, 1])
    Color ClampToLDR(Color const&) noexcept;

    // returns a Vec4 version of a Color
    inline glm::vec4 ToVec4(Color const& c) noexcept
    {
        return glm::vec4{c};
    }

    // returns a pointer to the first float element in the color
    constexpr float const* ValuePtr(Color const& color)
    {
        return &color.r;
    }

    // returns a pointer to the first float element in the color
    constexpr float* ValuePtr(Color& color)
    {
        return &color.r;
    }

    // linearly interpolates between `a` and `b` by `t`
    //
    // `t` is clamped to [0.0f, 1.0f]. When `t` is 0, returns `a`. When `t` is 1, returns `b`
    Color Lerp(Color const& a, Color const& b, float t) noexcept;

    // float-/double-based inputs assume normalized color range (i.e. 0 to 1)
    Color32 ToColor32(Color const&) noexcept;
    Color32 ToColor32(glm::vec4 const&) noexcept;
    Color32 ToColor32(float, float, float, float) noexcept;
    Color32 ToColor32(uint32_t) noexcept;  // R at MSB

    // returns the color as a hexadecimal string in the format "#rrggbbaa", as
    // commonly-used in web applications, configuration files, etc.
    //
    // - HDR values are clamped to LDR (they can't fit in this format)
    // - examples:
    //   - red --> "#ff0000ff"
    //   - green --> "#00ff00ff"
    //   - blue --> "#0000ffff"
    //   - black --> "#000000ff"
    //   - clear --> "#00000000"
    //   - etc.
    std::string ToHtmlStringRGBA(Color const&);
    std::optional<Color> TryParseHtmlString(std::string_view);
}

// define hashing function for colors
template<>
struct std::hash<osc::Color> final {
    size_t operator()(osc::Color const&) const;
};
