#pragma once

#include <liboscar/graphics/color_component.h>
#include <liboscar/graphics/rgb.h>
#include <liboscar/maths/common_functions.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/hash_helpers.h>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <ostream>
#include <tuple>
#include <utility>

namespace osc
{
    template<ColorComponent T>
    struct alignas(std::max(alignof(T), alignof(uint32_t))) Rgba final {

        using value_type = T;
        using reference = value_type&;
        using const_reference = const value_type&;
        using size_type = size_t;
        using iterator = value_type*;
        using const_iterator = const value_type*;

        static constexpr Rgba clear() { return {0.0f, 0.0f, 0.0f, 0.0f}; }

        static constexpr Rgba white()         { return Rgb<T>::white();         }
        static constexpr Rgba lightest_grey() { return Rgb<T>::lightest_grey(); }
        static constexpr Rgba lighter_grey()  { return Rgb<T>::lighter_grey();  }
        static constexpr Rgba light_grey()    { return Rgb<T>::light_grey();    }
        static constexpr Rgba dark_grey()     { return Rgb<T>::dark_grey();     }
        static constexpr Rgba darker_grey()   { return Rgb<T>::darker_grey();   }
        static constexpr Rgba darkest_grey()  { return Rgb<T>::darkest_grey();  }
        static constexpr Rgba black()         { return Rgb<T>::black();         }
        static constexpr Rgba muted_red()     { return Rgb<T>::muted_red();     }
        static constexpr Rgba red()           { return Rgb<T>::red();           }
        static constexpr Rgba muted_green()   { return Rgb<T>::muted_green();   }
        static constexpr Rgba green()         { return Rgb<T>::green();         }
        static constexpr Rgba dark_green()    { return Rgb<T>::dark_green();    }
        static constexpr Rgba muted_blue()    { return Rgb<T>::muted_blue();    }
        static constexpr Rgba blue()          { return Rgb<T>::blue();          }
        static constexpr Rgba cyan()          { return Rgb<T>::cyan();          }
        static constexpr Rgba magenta()       { return Rgb<T>::magenta();       }
        static constexpr Rgba muted_yellow()  { return Rgb<T>::muted_yellow();  }
        static constexpr Rgba yellow()        { return Rgb<T>::yellow();        }
        static constexpr Rgba orange()        { return Rgb<T>::orange();        }
        static constexpr Rgba purple()        { return Rgb<T>::purple();        }

        Rgba() = default;

        explicit constexpr Rgba(value_type v) :
            r{v}, g{v}, b{v}, a(1.0f)
        {}

        constexpr Rgba(value_type v, value_type alpha) :
            r{v}, g{v}, b{v}, a{alpha}
        {}

        explicit constexpr Rgba(const Vector<value_type, 3>& v) :
            r{v.x()}, g{v.y()}, b{v.z()}, a(1.0f)
        {}

        constexpr Rgba(const Vector<value_type, 3>& v, value_type alpha) :
            r{v.x()}, g{v.y()}, b{v.z()}, a{alpha}
        {}

        explicit constexpr Rgba(const Vector<value_type, 4>& v) :
            r{v.x()}, g{v.y()}, b{v.z()}, a{v.w()}
        {}

        template<ColorComponent U>
        requires std::constructible_from<T, const U&>
        explicit constexpr Rgba(const Vector<U, 4>& v) :
            r{static_cast<T>(v.x())},
            g{static_cast<T>(v.y())},
            b{static_cast<T>(v.z())},
            a{static_cast<T>(v.w())}
        {}

        constexpr Rgba(value_type r_, value_type g_, value_type b_, value_type a_) :
            r{r_}, g{g_}, b{b_}, a{a_}
        {}

        constexpr Rgba(value_type r_, value_type g_, value_type b_) :
            r{r_}, g{g_}, b{b_}, a(1.0f)
        {}

        template<ColorComponent U>
        requires std::constructible_from<T, const U&>
        explicit (not (std::convertible_to<U, T>))
        constexpr Rgba(const Rgba<U>& v) :
            r{static_cast<T>(v.r)},
            g{static_cast<T>(v.g)},
            b{static_cast<T>(v.b)},
            a{static_cast<T>(v.a)}
        {}

        constexpr Rgba(const Rgb<T>& rgb, T alpha = T{1.0f}) :
            r{rgb.r},
            g{rgb.g},
            b{rgb.b},
            a{alpha}
        {}

        constexpr reference       operator[](size_type pos)       { return *(begin() + pos); }
        constexpr const_reference operator[](size_type pos) const { return *(begin() + pos); }

        constexpr size_t size() const { return 4; }

        constexpr iterator       begin()       { return &r; }
        constexpr const_iterator begin() const { return &r; }

        constexpr iterator       end()       { return &r + size(); }
        constexpr const_iterator end() const { return &r + size(); }

        friend bool operator==(const Rgba&, const Rgba&) = default;

        constexpr friend Rgba& operator*=(Rgba& lhs, const Rgba& rhs)
        {
            lhs.r *= rhs.r;
            lhs.g *= rhs.g;
            lhs.b *= rhs.b;
            lhs.a *= rhs.a;

            return lhs;
        }

        constexpr friend Rgba operator*(const Rgba& lhs, const Rgba& rhs)
        {
            Rgba copy{lhs};
            copy *= rhs;
            return copy;
        }

        constexpr friend Rgba operator*(value_type lhs, const Rgba& rhs)
        {
            return Rgba{
                lhs * rhs.r,
                lhs * rhs.g,
                lhs * rhs.b,
                lhs * rhs.a,
            };
        }

        constexpr Rgba with_alpha(value_type a_) const
        {
            return Rgba{r, g, b, a_};
        }

        constexpr Rgba with_element(size_type pos, value_type value) const
        {
            Rgba copy{*this};
            copy[pos] = value;
            return copy;
        }

        constexpr Rgb<T> rgb() const
        {
            return {r, g, b};
        }

        value_type r{};
        value_type g{};
        value_type b{};
        value_type a{};
    };

    template<ColorComponent T>
    std::ostream& operator<<(std::ostream& o, const Rgba<T>& rgba)
    {
        return o << "Rgba{r = " << rgba.r << ", g = " << rgba.g << ", b = " << rgba.b << ", a = " << rgba.a << "}";
    }

    // returns a pointer to the first component of the `Rgba<T>`
    template<ColorComponent T>
    constexpr const T* value_ptr(const Rgba<T>& rgba)
    {
        return &rgba.r;
    }

    // returns a mutable pointer to the first component of the `Rgba<T>`
    template<ColorComponent T>
    constexpr T* value_ptr(Rgba<T>& rgba)
    {
        return &rgba.r;
    }

    // when handled as a tuple-like object, a `Rgba<T>` decomposes into its components (incl. alpha)

    template<size_t I, ColorComponent T>
    constexpr const T& get(const Rgba<T>& rgba) { return rgba[I]; }

    template<size_t I, ColorComponent T>
    constexpr T& get(Rgba<T>& rgba) { return rgba[I]; }

    template<size_t I, ColorComponent T>
    constexpr T&& get(Rgba<T>&& rgba) { return std::move(rgba[I]); }

    template<size_t I, ColorComponent T>
    constexpr const T&& get(const Rgba<T>&& rgba) { return std::move(rgba[I]); }

    // returns a `Rgba<U>` containing `op(xv)` for each `xv` component in `x`
    template<ColorComponent T, std::invocable<const T&> UnaryOperation>
    constexpr auto map(const Rgba<T>& x, UnaryOperation op) -> Rgba<decltype(std::invoke(op, x[0]))>
    {
        Rgba<decltype(std::invoke(op, x[0]))> rv{};
        for (size_t i = 0; i < 4; ++i) {
            rv[i] = std::invoke(op, x[i]);
        }
        return rv;
    }

    // returns a `Rgba<U>` containing `op(xv, yv)` for each `(xv, yv)` component in `x` and `y`
    template<ColorComponent T, std::invocable<const T&, const T&> BinaryOperation>
    constexpr auto map(const Rgba<T>& x, const Rgba<T>& y, BinaryOperation op) -> Rgba<decltype(std::invoke(op, x[0], y[0]))>
    {
        Rgba<decltype(std::invoke(op, x[0], y[0]))> rv{};
        for (size_t i = 0; i < 4; ++i) {
            rv[i] = std::invoke(op, x[i], y[i]);
        }
        return rv;
    }

    // returns a `Rgba<T>` containing `lerp(xv, yv, t)` for each `(xv, yv)` component in `x` and `y`
    template<ColorComponent T, typename TInterpolant>
    constexpr auto lerp(const Rgba<T>& x, const Rgba<T>& y, TInterpolant t)
    {
        return map(x, y, [&t](const T& xv, const T& yv) { return lerp(xv, yv, t); });
    }

    // returns an `Rgba<T>` containing `saturate(xv)` for each `xv` component in `x`
    template<ColorComponent T>
    constexpr Rgba<T> saturate(const Rgba<T>& x)
    {
        return map(x, [](const T& xv) { return saturate(xv); });
    }
}

// define compile-time size for `Rgba<T>`
template<osc::ColorComponent T>
struct std::tuple_size<osc::Rgba<T>> {
    static inline constexpr size_t value = 4;
};

template<size_t I, osc::ColorComponent T>
struct std::tuple_element<I, osc::Rgba<T>> {
    using type = T;
};

// define hashing function for `Rgba<T>`
template<osc::ColorComponent T>
struct std::hash<osc::Rgba<T>> final {
    size_t operator()(const osc::Rgba<T>& rgba) const noexcept
    {
        return osc::hash_of(rgba.r, rgba.g, rgba.b, rgba.a);
    }
};
