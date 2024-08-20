#pragma once

#include <compare>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace osc
{
    // Representation of a signed normalized floating-point value (i.e. [-1.0f, 1.0f])
    //
    // see: https://www.khronos.org/opengl/wiki/Normalized_Integer
    class Snorm8 final {
    public:
        constexpr Snorm8() = default;

        consteval Snorm8(int literal) :
            value_{static_cast<int8_t>(literal)}
        {
            if (literal > std::numeric_limits<int8_t>::max()) {
                throw std::runtime_error{"provided value is too large"};
            }
        }

        constexpr Snorm8(int8_t raw_value) :
            value_{raw_value}
        {}

        constexpr Snorm8(float normalized_value) :
            value_{to_normalized_int8(normalized_value)}
        {}

        friend auto operator<=>(const Snorm8&, const Snorm8&) = default;

        explicit constexpr operator float() const
        {
            return normalized_value();
        }

        explicit constexpr operator int8_t() const
        {
            return raw_value();
        }

        constexpr int8_t raw_value() const
        {
            return value_;
        }

        constexpr float normalized_value() const
        {
            // remapping signed integers is tricker than unsigned ones, because
            // `|min| != |max|`, this implementation follows OpenGL 4.2+'s
            // convention of mapping the integer range `[-127, 127]` onto
            // `[-1.0f, 1.0f]`, with the edge-case (-128) mapping onto `-1.0f` so
            // that `0 -> 0.0f`
            //
            // see: https://www.khronos.org/opengl/wiki/Normalized_Integer

            const float fvalue = static_cast<float>(value_) / 127.0f;
            return fvalue >= -1.0f ? fvalue : -1.0f;
        }

    private:
        static constexpr int8_t to_normalized_int8(float v)
        {
            const float saturated = v > -1.0f ? (v < 1.0f ? v : 1.0f) : -1.0f;
            return static_cast<int8_t>(127.0f * saturated);
        }

        int8_t value_ = 0;
    };
}
