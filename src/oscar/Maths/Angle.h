#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <compare>
#include <ostream>
#include <numbers>
#include <string_view>
#include <type_traits>

namespace osc
{
    // satisfied if `T` has the appropriate shape to be used as a "unit type trait" (e.g. radians)
    template<typename T>
    concept AngularUnitTraits = requires(T) {
        { T::radians_per_rep } -> std::convertible_to<double>;
        { T::unit_label } -> std::convertible_to<std::string_view>;
    };

    // a floating point number of type `Rep`, expressed in the given `Units`
    template<std::floating_point Rep, AngularUnitTraits Units>
    class Angle final {
    public:
        using value_type = Rep;

        constexpr Angle() = default;

        // explicitly constructs the angle from a raw value in the given units
        template<std::convertible_to<Rep> Rep2>
        constexpr explicit Angle(const Rep2& value_) :
            m_Value{static_cast<Rep>(value_)}
        {}

        // implicitly constructs from an angle expressed in other units
        template<AngularUnitTraits Units2>
        constexpr Angle(const Angle<Rep, Units2>& other) :
            m_Value{static_cast<Rep>(other.count() * (Units2::radians_per_rep/Units::radians_per_rep))}
        {}

        // explicitly constructs from an angle expressed of type `Rep2` expressed in other units
        template<std::convertible_to<Rep> Rep2, AngularUnitTraits Units2>
        explicit constexpr Angle(const Angle<Rep2, Units2>& other) :
            m_Value{static_cast<Rep>(other.count() * (Units2::radians_per_rep/Units::radians_per_rep))}
        {}

        // returns the underlying floating point representation of the angle
        constexpr Rep count() const { return m_Value; }

        constexpr Angle operator+() const { return Angle{*this}; }

        constexpr Angle operator-() const { return Angle{-m_Value}; }

        constexpr friend auto operator<=>(const Angle&, const Angle&) = default;

        constexpr friend Angle& operator+=(Angle& lhs, const Angle& rhs)
        {
            lhs.m_Value += rhs.m_Value;
            return lhs;
        }

        constexpr friend Angle& operator-=(Angle& lhs, const Angle& rhs)
        {
            lhs.m_Value -= rhs.m_Value;
            return lhs;
        }

        // scalar multiplication (both lhs and rhs)
        template<std::convertible_to<Rep> Rep2>
        constexpr friend Angle operator*(const Rep2& scalar, const Angle& rhs)
        {
            return Angle{static_cast<Rep>(scalar) * rhs.m_Value};
        }
        template<std::convertible_to<Rep> Rep2>
        constexpr friend Angle operator*(const Angle& lhs, const Rep2& scalar)
        {
            return Angle{lhs.m_Value * static_cast<Rep>(scalar)};
        }

        // scalar division (only on the rhs: reciporical angular units aren't supported)
        template<std::convertible_to<Rep> Rep2>
        constexpr friend Angle operator/(const Angle& lhs, const Rep2& scalar)
        {
            return Angle{lhs.m_Value / static_cast<Rep>(scalar)};
        }
    private:
        Rep m_Value{0};
    };

    // heterogeneously adds two angles (e.g. `90_deg + 1_turn`) by first converting them to a common angle type
    template<
        std::floating_point Rep1,
        AngularUnitTraits Units1,
        std::floating_point Rep2,
        AngularUnitTraits Units2
    >
    constexpr typename std::common_type_t<Angle<Rep1, Units1>, Angle<Rep2, Units2>> operator+(
        const Angle<Rep1, Units1>& lhs,
        const Angle<Rep2, Units2>& rhs)
    {
        using CA = std::common_type_t<Angle<Rep1, Units1>, Angle<Rep2, Units2>>;
        return CA{CA{lhs}.count() + CA{rhs}.count()};
    }

    // heterogeneously subtracts two angles (e.g. `90_deg - 1_turn`) by first converting them to a common angle type
    template<
        std::floating_point Rep1,
        AngularUnitTraits Units1,
        std::floating_point Rep2,
        AngularUnitTraits Units2
    >
    constexpr typename std::common_type_t<Angle<Rep1, Units1>, Angle<Rep2, Units2>> operator-(
        const Angle<Rep1, Units1>& lhs,
        const Angle<Rep2, Units2>& rhs)
    {
        using CA = std::common_type_t<Angle<Rep1, Units1>, Angle<Rep2, Units2>>;
        return CA{CA{lhs}.count() - CA{rhs}.count()};
    }

    // heterogeneously compares two angles (e.g. `360_deg == 1_turn`) by first converting them to a common type
    template<
        std::floating_point Rep1,
        AngularUnitTraits Units1,
        std::floating_point Rep2,
        AngularUnitTraits Units2
    >
    constexpr bool operator==(const Angle<Rep1, Units1>& lhs, const Angle<Rep2, Units2>& rhs)
    {
        using CA = std::common_type_t<Angle<Rep1, Units1>, Angle<Rep2, Units2>>;
        return CA{lhs} == CA{rhs};
    }

    // heterogeneously three-way compares two angles (e.g. `90_deg < 3_rad`) by first converting them to a common angle type
    template<
        std::floating_point Rep1,
        AngularUnitTraits Units1,
        std::floating_point Rep2,
        AngularUnitTraits Units2
    >
    constexpr auto operator<=>(const Angle<Rep1, Units1>& lhs, const Angle<Rep2, Units2>& rhs)
    {
        using CA = std::common_type_t<Angle<Rep1, Units1>, Angle<Rep2, Units2>>;
        return CA{lhs} <=> CA{rhs};
    }

    // writes the angle's value, followed by a space, followed by its units (use `.count()` if you just want the value)
    template<std::floating_point Rep, AngularUnitTraits Units>
    std::ostream& operator<<(std::ostream& o, const Angle<Rep, Units>& v)
    {
        return o << v.count() << ' ' << Units::unit_label;
    }
}

// a specialization of `std::common_type` for `osc::Angle`s
//
// (similar to how it's specialized for `std::chrono::duration`)
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


// unit trait implementations for common units (rad, deg, turn)
namespace osc
{
    // radians

    struct RadianAngularUnitTraits final {
        static inline constexpr double radians_per_rep = 1.0;
        static inline constexpr std::string_view unit_label = "rad";
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

    // degrees

    struct DegreesAngularUnitTraits final {
        static inline constexpr double radians_per_rep = std::numbers::pi_v<double>/180.0;
        static inline constexpr std::string_view unit_label = "deg";
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

    // turns

    struct TurnsAngularUnitTraits final {
        static inline constexpr double radians_per_rep = 2.0*std::numbers::pi_v<double>;
        static inline constexpr std::string_view unit_label = "turn";
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

// common mathematical functions, and algorithms, for angles
//
// note: if a generic algorithm is already available in `<algorithm>`, `<ranges>`, or
// `<oscar/Utils/Algorithms.h>`, then it shouldn't be here: these are specializations
// for cases that can't be generically handled (e.g. heterogeneous unit coercion)
namespace osc
{
    // `mod` (homogeneous and heterogeneous)
    template<
        std::floating_point Rep1,
        AngularUnitTraits Units1,
        std::floating_point Rep2,
        AngularUnitTraits Units2
    >
    auto mod(Angle<Rep1, Units1> x, Angle<Rep2, Units2> y) -> std::common_type_t<decltype(x), decltype(y)>
    {
        using CA = std::common_type_t<decltype(x), decltype(y)>;
        return CA{std::fmod(CA{x}.count(), CA{y}.count())};
    }

    // heterogeneous `min`
    //
    // note: homogeneous `min` is provided via `std::ranges::min` or `osc::min` algorithms
    template<
        std::floating_point Rep1,
        AngularUnitTraits Units1,
        std::floating_point Rep2,
        AngularUnitTraits Units2
    >
    requires (not std::is_same_v<Units1, Units2>)
    constexpr auto min(Angle<Rep1, Units1> x, Angle<Rep2, Units2> y) -> std::common_type_t<decltype(x), decltype(y)>
    {
        using CA = std::common_type_t<decltype(x), decltype(y)>;
        return CA{std::min(CA{x}.count(), CA{y}.count())};
    }

    // heterogeneous `max`
    //
    // note: homogeneous `max` is provided via `std::ranges::max` or `osc::max` algorithms
    template<
        std::floating_point Rep1,
        AngularUnitTraits Units1,
        std::floating_point Rep2,
        AngularUnitTraits Units2
    >
    requires (not std::is_same_v<Units1, Units2>)
    constexpr auto max(Angle<Rep1, Units1> x, Angle<Rep2, Units2> y) -> std::common_type_t<decltype(x), decltype(y)>
    {
        using CA = std::common_type_t<decltype(x), decltype(y)>;
        return CA{std::max(CA{x}.count(), CA{y}.count())};
    }

    // heterogeneous `clamp`
    //
    // note: homogeneous `clamp` is provided via `std::ranges::clamp` or `osc::clamp` algorithms
    template<
        std::floating_point Rep,
        AngularUnitTraits Units,
        std::convertible_to<Angle<Rep, Units>> AngleMin,
        std::convertible_to<Angle<Rep, Units>> AngleMax
    >
    requires (
        not std::is_same_v<Angle<Rep, Units>, AngleMin> or
        not std::is_same_v<Angle<Rep, Units>, AngleMax> or
        not std::is_same_v<AngleMin, AngleMax>
    )
    constexpr Angle<Rep, Units> clamp(
        const Angle<Rep, Units>& v,
        const AngleMin& min,
        const AngleMax& max)
    {
        return std::clamp(v, Angle<Rep, Units>{min}, Angle<Rep, Units>{max});
    }
}
