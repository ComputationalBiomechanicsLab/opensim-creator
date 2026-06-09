#pragma once

#include <liboscar/graphics/color_component.h>
#include <liboscar/maths/vector.h>

#include <concepts>
#include <cstddef>
#include <utility>

namespace osc
{
    template<ColorComponent T>
    struct Rgb final {

        using value_type = T;
        using reference = value_type&;
        using const_reference = const value_type&;
        using size_type = size_t;
        using iterator = value_type*;
        using const_iterator = const value_type*;

        static constexpr Rgb white()         { return {1.f,   1.f,   1.f  }; }
        static constexpr Rgb lightest_grey() { return {0.95f, 0.95f, 0.95f}; }
        static constexpr Rgb lighter_grey()  { return {0.85f, 0.85f, 0.85f}; }
        static constexpr Rgb light_grey()    { return {0.70f, 0.70f, 0.70f}; }
        static constexpr Rgb dark_grey()     { return {0.50f, 0.50f, 0.50f}; }
        static constexpr Rgb darker_grey()   { return {0.35f, 0.35f, 0.35f}; }
        static constexpr Rgb darkest_grey()  { return {0.05f, 0.05f, 0.05f}; }
        static constexpr Rgb black()         { return {0.f,   0.f,   0.f  }; }
        static constexpr Rgb muted_red()     { return {1.f,   0.5f,  0.5f }; }
        static constexpr Rgb red()           { return {1.f,   0.f,   0.f  }; }
        static constexpr Rgb muted_green()   { return {0.5f,  1.f,   0.5f }; }
        static constexpr Rgb green()         { return {0.f,   1.f,   0.f  }; }
        static constexpr Rgb dark_green()    { return {0.f,   0.6f,  0.f  }; }
        static constexpr Rgb muted_blue()    { return {0.06f, 0.53f, 0.98f}; }
        static constexpr Rgb blue()          { return {0.f,   0.f,   1.f  }; }
        static constexpr Rgb cyan()          { return {0.f,   1.f,   1.f  }; }
        static constexpr Rgb magenta()       { return {1.f,   0.f,   1.f  }; }
        static constexpr Rgb muted_yellow()  { return {1.f,   1.f,   0.6f }; }
        static constexpr Rgb yellow()        { return {1.f,   1.f,   0.f  }; }
        static constexpr Rgb orange()        { return {1.f,   0.65f, 0.f  }; }
        static constexpr Rgb purple()        { return {0.75f, 0.33f, 0.93f}; }

        Rgb() = default;

        explicit constexpr Rgb(value_type v) : r{v}, g{v}, b{v} {}

        constexpr Rgb(value_type r_, value_type g_, value_type b_) : r{r_}, g{g_}, b{b_} {}

        explicit constexpr Rgb(const Vector<value_type, 3>& v) : r{v.x()}, g{v.y()}, b{v.z()} {}

        template<ColorComponent U>
        requires std::constructible_from<T, const U&>
        explicit constexpr Rgb(const Vector<U, 3>& v) :
            r{static_cast<T>(v.x())},
            g{static_cast<T>(v.y())},
            b{static_cast<T>(v.z())}
        {}

        template<ColorComponent U>
        requires std::constructible_from<T, const U&>
        explicit (not (std::convertible_to<U, T>))
        constexpr Rgb(const Rgb<U>& v) :
            r{static_cast<T>(v.r)},
            g{static_cast<T>(v.g)},
            b{static_cast<T>(v.b)}
        {}

        constexpr reference       operator[](size_type pos)       { return *(begin() + pos); }
        constexpr const_reference operator[](size_type pos) const { return *(begin() + pos); }

        constexpr size_t size() const { return 3; }

        constexpr iterator       begin()       { return &r; }
        constexpr const_iterator begin() const { return &r; }

        constexpr iterator       end()       { return &r + size(); }
        constexpr const_iterator end() const { return &r + size(); }

        friend bool operator==(const Rgb&, const Rgb&) = default;

        value_type r{};
        value_type g{};
        value_type b{};
    };

    template<size_t I, typename T>
    constexpr const T& get(const Rgb<T>& v) { return v[I]; }

    template<size_t I, typename T>
    constexpr T& get(Rgb<T>& v) { return v[I]; }

    template<size_t I, typename T>
    constexpr T&& get(Rgb<T>&& v) { return std::move(v[I]); }

    template<size_t I, typename T>
    constexpr const T&& get(const Rgb<T>&& v) { return std::move(v[I]); }
}

template<osc::ColorComponent T>
struct std::tuple_size<osc::Rgb<T>> {
    static inline constexpr size_t value = 3;
};

template<size_t I, osc::ColorComponent T>
struct std::tuple_element<I, osc::Rgb<T>> {
    using type = T;
};
