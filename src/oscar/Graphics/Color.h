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
#include <tuple>

namespace osc
{
    struct Color final {
        using value_type = float;
        using reference = float&;
        using const_reference = const float&;
        using size_type = size_t;
        using iterator = value_type*;
        using const_iterator = const value_type*;

        static constexpr Color half_grey()
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

        static constexpr Color muted_blue()
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

        static constexpr Color muted_green()
        {
            return {0.5f, 1.0f, 0.5f};
        }

        static constexpr Color dark_green()
        {
            return {0.0f, 0.6f, 0.0f};
        }

        static constexpr Color red()
        {
            return {1.0f, 0.0f, 0.0f};
        }

        static constexpr Color muted_red()
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
        {}

        constexpr Color(value_type v, value_type alpha) :
            r{v}, g{v}, b{v}, a{alpha}
        {}

        explicit constexpr Color(const Vec<3, value_type>& v) :
            r{v.x}, g{v.y}, b{v.z}, a(1.0f)
        {}

        constexpr Color(const Vec<3, value_type>& v, value_type alpha) :
            r{v.x}, g{v.y}, b{v.z}, a{alpha}
        {}

        explicit constexpr Color(const Vec<4, value_type>& v) :
            r{v.x}, g{v.y}, b{v.z}, a{v.w}
        {}

        constexpr Color(value_type r_, value_type g_, value_type b_, value_type a_) :
            r{r_}, g{g_}, b{b_}, a{a_}
        {}

        constexpr Color(value_type r_, value_type g_, value_type b_) :
            r{r_}, g{g_}, b{b_}, a(1.0f)
        {}

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

        constexpr size_t size() const
        {
            return 4;
        }

        constexpr iterator begin()
        {
            return &r;
        }

        constexpr iterator end()
        {
            return &r + size();
        }

        constexpr const_iterator begin() const
        {
            return &r;
        }

        constexpr const_iterator end() const
        {
            return &r + size();
        }

        constexpr operator Vec<4, value_type> () const
        {
            return Vec4{r, g, b, a};
        }

        friend bool operator==(const Color&, const Color&) = default;

        constexpr friend Color& operator*=(Color& lhs, const Color& rhs)
        {
            lhs.r *= rhs.r;
            lhs.g *= rhs.g;
            lhs.b *= rhs.b;
            lhs.a *= rhs.a;

            return lhs;
        }

        constexpr friend Color operator*(const Color& lhs, const Color& rhs)
        {
            Color copy{lhs};
            copy *= rhs;
            return copy;
        }

        constexpr friend Color operator*(value_type lhs, const Color& rhs)
        {
            return Color{
                lhs * rhs.r,
                lhs * rhs.g,
                lhs * rhs.b,
                lhs * rhs.a,
            };
        }

        constexpr Color with_alpha(value_type a_) const
        {
            return Color{r, g, b, a_};
        }

        value_type r{};
        value_type g{};
        value_type b{};
        value_type a{};
    };

    std::ostream& operator<<(std::ostream&, const Color&);

    // returns the linear version of one (presumed to be) sRGB color channel value
    float to_linear_colorspace(float colorChannelValue);

    // returns the linear version of one (presumed to be) linear color channel value
    float to_srgb_colorspace(float colorChannelValue);

    // returns the linear version of a (presumed to be) sRGB color
    Color to_linear_colorspace(const Color&);

    // returns a color that is the (presumed to be) linear color with the sRGB gamma curve applied
    Color to_srgb_colorspace(const Color&);

    // returns a color that is clamped to the low-dynamic range (LDR, i.e. [0, 1])
    Color clamp_to_ldr(const Color&);

    // returns the HSL(A) equivalent of the given (RGBA) color
    ColorHSLA to_hsla_color(const Color&);

    // returns the color (RGBA) equivalent of the given HSL color
    Color to_color(const ColorHSLA&);

    // returns a Vec4 version of a Color
    inline Vec4 to_vec4(const Color& c)
    {
        return Vec4{c};
    }

    // returns a pointer to the first float element in the color
    constexpr const float* value_ptr(const Color& color)
    {
        return &color.r;
    }

    // returns a pointer to the first float element in the color
    constexpr float* value_ptr(Color& color)
    {
        return &color.r;
    }

    // linearly interpolates all color channels and alpha of `a` and `b` by `t`
    //
    // `t` is clamped to [0.0f, 1.0f]. When `t` is 0, returns `a`. When `t` is 1, returns `b`
    Color lerp(const Color& a, const Color& b, float t);

    // float-/double-based inputs assume normalized color range (i.e. 0 to 1)
    Color32 to_color32(const Color&);
    Color32 to_color32(const Vec4&);
    Color32 to_color32(float, float, float, float);
    Color32 to_color32(uint32_t);  // R at MSB
    Color to_color(Color32);

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
    std::string to_html_string_rgba(const Color&);
    std::optional<Color> try_parse_html_color_string(std::string_view);

    // returns a color that is the result of converting `color` to HSLA,
    // multiplying its luminance (L) by `factor`, and converting it back to RGBA
    Color multiply_luminance(const Color& color, float factor);

    // when handled as a tuple-like object, a `Color` decomposes into its channels (incl. alpha)

    template<size_t I>
    constexpr const float& get(const Color& c) { return c[I]; }

    template<size_t I>
    constexpr float& get(Color& c) { return c[I]; }

    template<size_t I>
    constexpr float&& get(Color&& c) { return std::move(c[I]); }

    template<size_t I>
    constexpr const float&& get(const Color&& c) { return std::move(c[I]); }
}

// define compile-time size for Color (same as std::array, std::tuple, Vec, etc.)
template<>
struct std::tuple_size<osc::Color> {
    static inline constexpr size_t value = 4;
};

template<size_t I>
struct std::tuple_element<I, osc::Color> {
    using type = float;
};

// define hashing function for colors
template<>
struct std::hash<osc::Color> final {
    size_t operator()(const osc::Color&) const;
};
