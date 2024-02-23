#pragma once

#include <oscar/Graphics/Color32.h>
#include <oscar/Graphics/ColorHSLA.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>

namespace osc
{
    struct Color final {

        using value_type = float;
        using reference = float&;
        using const_reference = float const&;
        using size_type = size_t;
        using iterator = value_type*;
        using const_iterator = value_type const*;

        static constexpr size_type length()
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

        explicit constexpr Color(value_type v) :
            r{v}, g{v}, b{v}, a(1.0f)
        {
        }

        constexpr Color(value_type v, value_type alpha) :
            r{v}, g{v}, b{v}, a{alpha}
        {
        }

        explicit constexpr Color(Vec<3, value_type> const& v) :
            r{v.x}, g{v.y}, b{v.z}, a(1.0f)
        {
        }

        constexpr Color(Vec<3, value_type> const& v, value_type alpha) :
            r{v.x}, g{v.y}, b{v.z}, a{alpha}
        {
        }

        explicit constexpr Color(Vec<4, value_type> const& v) :
            r{v.x}, g{v.y}, b{v.z}, a{v.w}
        {
        }

        constexpr Color(value_type r_, value_type g_, value_type b_, value_type a_) :
            r{r_}, g{g_}, b{b_}, a{a_}
        {
        }

        constexpr Color(value_type r_, value_type g_, value_type b_) :
            r{r_}, g{g_}, b{b_}, a(1.0f)
        {
        }

        constexpr reference operator[](size_type i)
        {
            static_assert(sizeof(Color) == 4*sizeof(value_type));
            return (&r)[i];
        }

        constexpr const_reference operator[](size_type i) const
        {
            static_assert(sizeof(Color) == 4*sizeof(value_type));
            return (&r)[i];
        }

        constexpr iterator begin()
        {
            return &r;
        }

        constexpr iterator end()
        {
            return &r + length();
        }

        constexpr const_iterator begin() const
        {
            return &r;
        }

        constexpr const_iterator end() const
        {
            return &r + length();
        }

        constexpr operator Vec<4, value_type> () const
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
            Color copy{lhs};
            copy *= rhs;
            return copy;
        }

        constexpr friend Color operator*(value_type lhs, Color const& rhs)
        {
            return Color {
                lhs * rhs.r,
                lhs * rhs.g,
                lhs * rhs.b,
                lhs * rhs.a,
            };
        }

        constexpr Color withAlpha(value_type a_) const
        {
            return Color{r, g, b, a_};
        }

        value_type r{};
        value_type g{};
        value_type b{};
        value_type a{};
    };

    std::ostream& operator<<(std::ostream&, Color const&);

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
    constexpr float const* value_ptr(Color const& color)
    {
        return &color.r;
    }

    // returns a pointer to the first float element in the color
    constexpr float* value_ptr(Color& color)
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
