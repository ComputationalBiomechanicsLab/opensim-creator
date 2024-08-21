#pragma once

#include <compare>
#include <concepts>
#include <limits>
#include <stdexcept>

namespace osc
{
    // A normalized signed integer that can be used to store a floating-point
    // number in the (clamped) range [-1.0f, 1.0f]
    //
    // see: https://www.khronos.org/opengl/wiki/Normalized_Integer
    template<std::signed_integral T>
    class Snorm final {
    public:
        constexpr Snorm() = default;

        consteval Snorm(int literal) :
            value_{static_cast<T>(literal)}
        {
            if (literal < std::numeric_limits<T>::min() or
                literal > std::numeric_limits<T>::max()) {

                throw std::runtime_error{"provided value is out of range"};
            }
        }

        constexpr Snorm(T raw_value) :
            value_{raw_value}
        {}

        constexpr Snorm(float normalized_value) :
            value_{to_normalized_int(normalized_value)}
        {}

        friend auto operator<=>(const Snorm&, const Snorm&) = default;

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
            // remapping signed integers is tricker than unsigned ones, because
            // `|MIN| > |MAX|`
            // 
            // this implementation follows OpenGL 4.2+'s convention of mapping
            // the integer range `[-MAX, MAX]` onto `[-1.0f, 1.0f]`, with
            // the edge-case (MIN) mapping onto `-1.0f`, which ensures `0` maps
            // onto `0.0f`
            //
            // see: https://www.khronos.org/opengl/wiki/Normalized_Integer

            const float fvalue = static_cast<float>(value_) / static_cast<float>(std::numeric_limits<T>::max());
            return fvalue >= -1.0f ? fvalue : -1.0f;
        }

    private:
        static constexpr T to_normalized_int(float v)
        {
            const float saturated = v > -1.0f ? (v < 1.0f ? v : 1.0f) : -1.0f;
            return static_cast<T>(127.0f * saturated);
        }

        T value_ = 0;
    };
}
