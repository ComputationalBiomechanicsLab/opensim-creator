#pragma once

#include <oscar/Maths/Scalar.h>

#include <compare>
#include <concepts>
#include <limits>
#include <stdexcept>

namespace osc
{
    // A normalized unsigned integer that can be used to store a floating-point
    // number in the (clamped) range [0.0f, 1.0f]
    //
    // see: https://www.khronos.org/opengl/wiki/Normalized_Integer
    template<std::unsigned_integral T>
    class Unorm {
    public:
        using value_type = T;

        constexpr Unorm() = default;

        consteval Unorm(int literal) :
            value_{static_cast<T>(literal)}
        {
            if (literal < std::numeric_limits<T>::min() or
                literal > std::numeric_limits<T>::max()) {

                throw std::runtime_error{"provided value is out of range"};
            }
        }

        constexpr Unorm(T raw_value) :
            value_{raw_value}
        {}

        constexpr Unorm(float normalized_value) :
            value_{to_normalized_uint(normalized_value)}
        {}

        friend auto operator<=>(const Unorm&, const Unorm&) = default;

        explicit constexpr operator float() const
        {
            return normalized_value();
        }

        explicit constexpr operator T() const
        {
            return raw_value();
        }

        constexpr T raw_value() const
        {
            return value_;
        }

        constexpr float normalized_value() const
        {
            return (1.0f/static_cast<float>(std::numeric_limits<T>::max())) * static_cast<float>(value_);
        }

    private:
        static constexpr T to_normalized_uint(float v)
        {
            const float saturated = v > 0.0f ? (v < 1.0f ? v : 1.0f) : 0.0f;
            return static_cast<T>(static_cast<float>(std::numeric_limits<T>::max()) * saturated);
        }

        T value_ = 0;
    };

    // tag `Unorm<T>` as scalar-like, so that other parts of the codebase (e.g.
    // vectors, matrices) accept it
    template<std::unsigned_integral T>
    struct IsScalar<Unorm<T>> final {
        static constexpr bool value = true;
    };
}
