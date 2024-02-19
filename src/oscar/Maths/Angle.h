#pragma once

#include <concepts>
#include <compare>
#include <numbers>
#include <type_traits>

namespace osc
{
    template<typename T>
    concept AngularUnitTraits = requires(T) {
        { T::radians_per_rep } -> std::convertible_to<double>;
    };

    /**
     * Represents a floating point of type `Rep`, which is expressed in the given
     * `Units`
     */
    template<std::floating_point Rep, AngularUnitTraits Units>
    class Angle final {
    public:
        using value_type = Rep;

        constexpr Angle() = default;

        // explicitly constructs the angle from a raw value in the given units
        template<std::convertible_to<Rep> Rep2>
        constexpr explicit Angle(Rep2 const& value_) :
            m_Value{static_cast<Rep>(value_)}
        {}

        // implicit unit-conversion constructor
        template<AngularUnitTraits Units2>
        constexpr Angle(Angle<Rep, Units2> const& other) :
            m_Value{static_cast<Rep>(other.count() * (Units2::radians_per_rep/Units::radians_per_rep))}
        {}

        // explicit rep+unit conversion constructor
        template<std::convertible_to<Rep> Rep2, AngularUnitTraits Units2>
        explicit constexpr Angle(Angle<Rep2, Units2> const& other) :
            m_Value{static_cast<Rep>(other.count() * (Units2::radians_per_rep/Units::radians_per_rep))}
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

        // scalar multiplication (both lhs and rhs)
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

        // scalar division (only rhs: reciporical units aren't supported)
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
        AngularUnitTraits Units1,
        std::floating_point Rep2,
        AngularUnitTraits Units2
    >
    constexpr typename std::common_type_t<Angle<Rep1, Units1>, Angle<Rep2, Units2>> operator+(
        Angle<Rep1, Units1> const& lhs,
        Angle<Rep2, Units2> const& rhs)
    {
        using CA = std::common_type_t<Angle<Rep1, Units1>, Angle<Rep2, Units2>>;
        return CA{CA{lhs}.count() + CA{rhs}.count()};
    }

    template<
        std::floating_point Rep1,
        AngularUnitTraits Units1,
        std::floating_point Rep2,
        AngularUnitTraits Units2
    >
    constexpr typename std::common_type_t<Angle<Rep1, Units1>, Angle<Rep2, Units2>> operator-(
        Angle<Rep1, Units1> const& lhs,
        Angle<Rep2, Units2> const& rhs)
    {
        using CA = std::common_type_t<Angle<Rep1, Units1>, Angle<Rep2, Units2>>;
        return CA{CA{lhs}.count() - CA{rhs}.count()};
    }

    // converting equality comparison (e.g. 360_deg == 1_turn)
    template<
        std::floating_point Rep1,
        AngularUnitTraits Units1,
        std::floating_point Rep2,
        AngularUnitTraits Units2
    >
    constexpr bool operator==(Angle<Rep1, Units1> const& lhs, Angle<Rep2, Units2> const& rhs)
    {
        using CA = std::common_type_t<Angle<Rep1, Units1>, Angle<Rep2, Units2>>;
        return CA{lhs} == CA{rhs};
    }

    // converting ordering comparison (e.g. 180_deg < 1_turn)
    template<
        std::floating_point Rep1,
        AngularUnitTraits Units1,
        std::floating_point Rep2,
        AngularUnitTraits Units2
    >
    constexpr auto operator<=>(Angle<Rep1, Units1> const& lhs, Angle<Rep2, Units2> const& rhs)
    {
        using CA = std::common_type_t<Angle<Rep1, Units1>, Angle<Rep2, Units2>>;
        return CA{lhs} <=> CA{rhs};
    }

    // radians support

    struct RadianAngularUnitTraits final {
        static inline constexpr double radians_per_rep = 1.0;
    };

    template<typename T>
    using RadiansT = Angle<T, RadianAngularUnitTraits>;
    using Radians = RadiansT<float>;
    using Radiansd = RadiansT<double>;

    namespace literals
    {
        constexpr Radians operator""_rad(long double radians) { return Radians{radians}; }
        constexpr Radians operator""_rad(unsigned long long int radians) { return Radians{radians}; }
    }

    // degrees support

    struct DegreesAngularUnitTraits final {
        static inline constexpr double radians_per_rep = std::numbers::pi_v<double>/180.0;
    };

    template<typename T>
    using DegreesT = Angle<T, DegreesAngularUnitTraits>;
    using Degrees = DegreesT<float>;
    using Degreesd = DegreesT<double>;

    namespace literals
    {
        constexpr Degrees operator""_deg(long double degrees) { return Degrees{degrees}; }
        constexpr Degrees operator""_deg(unsigned long long int degrees) { return Degrees{degrees};}
    }

    // turn support

    struct TurnsAngularUnitTraits final {
        static inline constexpr double radians_per_rep = 2.0*std::numbers::pi_v<double>;
    };

    template<typename T>
    using TurnsT = Angle<T, TurnsAngularUnitTraits>;
    using Turns = TurnsT<float>;
    using Turnsd = TurnsT<double>;

    namespace literals
    {
        constexpr Turns operator""_turn(long double turns) { return Turns{turns}; }
        constexpr Turns operator""_turn(unsigned long long int turns) { return Turns{turns}; }
    }
}

// define a specialization for `std::common_type` (similarly to how it's done for std::chrono::duration)
template<
    std::floating_point Rep1,
    osc::AngularUnitTraits Units1,
    std::floating_point Rep2,
    osc::AngularUnitTraits Units2
>
struct std::common_type<osc::Angle<Rep1, Units1>, osc::Angle<Rep2, Units2>> {
    // the units of the common type is the "largest" of either
    using units = typename std::conditional_t<(Units1::radians_per_rep > Units2::radians_per_rep), Units1, Units2>;
    using type = osc::Angle<std::common_type_t<Rep1, Rep2>, units>;
};
