#pragma once

#include <cmath>
#include <concepts>
#include <compare>
#include <numbers>
#include <type_traits>

namespace osc
{
    /**
     * Represents a count of ticks of type `Rep` and a tick period, which is a
     * compile-time fraction of a radian (e.g. a degree would have a tick period
     * of `pi/180`, and a radian would have a tick period of `1`)
     */
    template<std::floating_point Rep, double Period = 1.0>
    class Angle final {
    public:
        constexpr Angle() = default;

        // explicitly constructs the angle from a raw value in the given units
        template<std::convertible_to<Rep> Rep2>
        constexpr explicit Angle(Rep2 const& value_) :
            m_Value{static_cast<Rep>(value_)}
        {}


        // converting constructor
        template<std::convertible_to<Rep> Rep2, double Period2>
        constexpr Angle(Angle<Rep2, Period2> const& other) :
            m_Value{static_cast<Rep>(other.count() * (Period2/Period))}
        {}

        constexpr Rep count() const
        {
            return m_Value;
        }

        constexpr Angle operator+() const
        {
            return Angle{*this};
        }

        constexpr Angle operator-() const
        {
            return Angle{-m_Value};
        }

        constexpr friend auto operator<=>(Angle const&, Angle const&) = default;

        constexpr friend Angle& operator+=(Angle& lhs, Angle const& rhs)
        {
            lhs.m_Value += rhs.m_Value;
            return lhs;
        }

        constexpr friend Angle& operator-=(Angle& lhs, Angle const& rhs)
        {
            lhs.m_Value -= rhs.m_Value;
            return lhs;
        }

        template<std::convertible_to<Rep> Rep2>
        constexpr friend Angle operator*(Rep2 const& scalar, Angle const& rhs)
        {
            return Angle{static_cast<Rep>(scalar) * rhs.m_Value};
        }

        template<std::convertible_to<Rep> Rep2>
        constexpr friend Angle operator*(Angle const& lhs, Rep2 const& scalar)
        {
            return Angle{lhs.m_Value * static_cast<Rep>(scalar)};
        }

        template<std::convertible_to<Rep> Rep2>
        constexpr friend Angle operator/(Angle const& lhs, Rep2 const& scalar)
        {
            return Angle{lhs.m_Value / static_cast<Rep>(scalar)};
        }
    private:
        Rep m_Value;
    };

    template<
        std::floating_point Rep1,
        double Period1,
        std::floating_point Rep2,
        double Period2
    >
    constexpr typename std::common_type_t<Angle<Rep1, Period1>, Angle<Rep2, Period2>> operator+(
        Angle<Rep1, Period1> const& lhs,
        Angle<Rep2, Period2> const& rhs)
    {
        using CA = std::common_type_t<Angle<Rep1, Period1>, Angle<Rep2, Period2>>;
        return CA{CA{lhs}.count() + CA{rhs}.count()};
    }

    template<typename Rep1, double Period1, typename Rep2, double Period2>
    constexpr typename std::common_type_t<Angle<Rep1, Period1>, Angle<Rep2, Period2>> operator-(
        Angle<Rep1, Period1> const& lhs,
        Angle<Rep2, Period2> const& rhs)
    {
        using CA = std::common_type_t<Angle<Rep1, Period1>, Angle<Rep2, Period2>>;
        return CA{CA{lhs}.count() - CA{rhs}.count()};
    }

    using Radians = Angle<float>;
    using Radiansd = Angle<double>;
    using Degrees = Angle<float, std::numbers::pi_v<double>/180.0>;
    using Degreesd = Angle<double, std::numbers::pi_v<double>/180.0>;

    namespace literals
    {
        constexpr Radians operator""_rad(long double radians)
        {
            return Radians{radians};
        }

        constexpr Radians operator""_rad(unsigned long long int radians)
        {
            return Radians{radians};
        }

        constexpr Degrees operator""_deg(long double degrees)
        {
            return Degrees{degrees};
        }

        constexpr Degrees operator""_deg(unsigned long long int degrees)
        {
            return Degrees{degrees};
        }
    }
}

template<typename Rep1, double Period1, typename Rep2, double Period2>
struct std::common_type<osc::Angle<Rep1, Period1>, osc::Angle<Rep2, Period2>> {
    // the "common period" is the largest of the two
    static inline constexpr double period = Period1 > Period2 ? Period1 : Period2;
    using type = osc::Angle<std::common_type_t<Rep1, Rep2>, period>;
};
