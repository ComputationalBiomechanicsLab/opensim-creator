#pragma once

#include <oscar/Maths/CommonFunctions.h>

#include <compare>
#include <concepts>
#include <ostream>

namespace osc
{
    // validated wrapper for "a floating point value that lies within the closed interval [0.0, 1.0]"
    template<std::floating_point T>
    class Normalized final {
    public:
        constexpr Normalized() = default;
        constexpr Normalized(T v) : m_Value{saturate(v)} {}

        friend bool operator==(Normalized const&, Normalized const&) = default;
        friend auto operator<=>(Normalized const&, Normalized const&) = default;

        constexpr T const& get() const { return m_Value; }
        constexpr operator T const& () const { return m_Value; }
    private:
        T m_Value{};
    };

    template<std::floating_point T>
    std::ostream& operator<<(std::ostream& o, Normalized<T> const& v)
    {
        return o << v.get();
    }
}
