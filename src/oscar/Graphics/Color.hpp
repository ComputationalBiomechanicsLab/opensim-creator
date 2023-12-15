#pragma once

#include <oscar/Graphics/ColorHSLA.hpp>
#include <oscar/Graphics/Color32.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Maths/Vec4.hpp>

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>

namespace osc
{
    // representation of RGBA, usually in sRGB color space, with a range of 0 to 1
    struct Color final {

        using value_type = float;

        static constexpr size_t length()
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

        static constexpr Color orange()
        {
            return {255.0f/255.0f, 165.0f/255.0f, 0.0f};
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

        explicit constexpr Color(float v) :
            r{v}, g{v}, b{v}, a{1.0f}
        {
        }

        constexpr Color(float v, float alpha) :
            r{v}, g{v}, b{v}, a{alpha}
        {
        }

        explicit constexpr Color(Vec3 const& v) :
            r{v.x}, g{v.y}, b{v.z}, a{1.0f}
        {
        }

        constexpr Color(Vec3 const& v, float alpha) :
            r{v.x}, g{v.y}, b{v.z}, a{alpha}
        {
        }

        explicit constexpr Color(Vec4 const& v) :
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

        constexpr float& operator[](ptrdiff_t i)
        {
            static_assert(sizeof(Color) == 4*sizeof(float));
            return (&r)[i];
        }

        constexpr float const& operator[](ptrdiff_t i) const
        {
            static_assert(sizeof(Color) == 4*sizeof(float));
            return (&r)[i];
        }

        constexpr operator Vec4 () const
        {
            return Vec4{r, g, b, a};
        }

        friend bool operator==(Color const&, Color const&) = default;

        constexpr friend Color& operator*=(Color& lhs, Color const& rhs)
        {
            lhs.r *= rhs.r;
            lhs.g *= rhs.g;
            lhs.b *= rhs.b;
            lhs.a *= rhs.a;

            return lhs;
        }

        constexpr friend Color operator*(Color const& lhs, Color const& rhs)
        {
            return Color
            {
                lhs.r * rhs.r,
                lhs.g * rhs.g,
                lhs.b * rhs.b,
                lhs.a * rhs.a,
            };
        }

        constexpr friend Color operator*(float lhs, Color const& rhs)
        {
            return Color
            {
                lhs * rhs.r,
                lhs * rhs.g,
                lhs * rhs.b,
                lhs * rhs.a,
            };
        }

        constexpr Color withAlpha(float a_) const
        {
            return Color{r, g, b, a_};
        }

        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        float a = 0.0f;
    };

    std::ostream& operator<<(std::ostream&, Color const&);

    // returns the normalized (0.0 - 1.0) floating-point equivalent of the
    // given 8-bit (0 - 255) color channel value
    constexpr float ToFloatingPointColorChannel(uint8_t channelValue)
    {
        return (1.0f/255.0f) * static_cast<float>(channelValue);
    }

    // returns the 8-bit (0 - 255) equivalent of the given normalized (0.0 - 1.0)
    // floating-point color channel value
    //
    // input values that fall outside of the 0.0 - 1.0 range are clamped to that range
    constexpr uint8_t ToClamped8BitColorChannel(float channelValue)
    {
        channelValue = channelValue >= 0.0f ? channelValue : 0.0f;
        channelValue = channelValue <= 1.0f ? channelValue : 1.0f;
        return static_cast<uint8_t>(255.0f * channelValue);
    }

    // returns the linear version of one (presumed to be) sRGB color channel value
    float ToLinear(float colorChannelValue);

    // returns the linear version of one (presumed to be) linear color channel value
    float ToSRGB(float colorChannelValue);

    // returns the linear version of a (presumed to be) sRGB color
    Color ToLinear(Color const&);

    // returns a color that is the (presumed to be) linear color with the sRGB gamma curve applied
    Color ToSRGB(Color const&);

    // returns a color that is clamped to the low-dynamic range (LDR, i.e. [0, 1])
    Color ClampToLDR(Color const&);

    // returns the HSL(A) equivalent of the given (RGBA) color
    ColorHSLA ToHSLA(Color const&);

    // returns the color (RGBA) equivalent of the given HSL color
    Color ToColor(ColorHSLA const&);

    // returns a Vec4 version of a Color
    inline Vec4 ToVec4(Color const& c)
    {
        return Vec4{c};
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
    Color Lerp(Color const& a, Color const& b, float t);

    // float-/double-based inputs assume normalized color range (i.e. 0 to 1)
    Color32 ToColor32(Color const&);
    Color32 ToColor32(Vec4 const&);
    Color32 ToColor32(float, float, float, float);
    Color32 ToColor32(uint32_t);  // R at MSB
    Color ToColor(Color32);

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

    // returns a color that is the result of converting the color to HSLA,
    // multiplying it's luminance (L) by `factor`, and converting it back to RGBA
    Color MultiplyLuminance(Color const& color, float factor);
}

// define hashing function for colors
template<>
struct std::hash<osc::Color> final {
    size_t operator()(osc::Color const&) const;
};
