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
        constexpr Normalized(T value) : value_{saturate(value)} {}

        friend bool operator==(const Normalized&, const Normalized&) = default;
        friend auto operator<=>(const Normalized&, const Normalized&) = default;

        constexpr const T& get() const { return value_; }
        constexpr operator const T& () const { return value_; }
    private:
        T value_{};
    };

    template<std::floating_point T>
    std::ostream& operator<<(std::ostream& o, const Normalized<T>& v)
    {
        return o << v.get();
    }
}
