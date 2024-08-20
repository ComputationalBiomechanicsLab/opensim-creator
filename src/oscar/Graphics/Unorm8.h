#pragma once

#include <oscar/Maths/Scalar.h>

#include <cstddef>
#include <cstdint>
#include <compare>
#include <limits>
#include <stdexcept>

namespace osc
{
    // Representation of an unsigned normalized floating-point value (i.e. [0, 1])
    // as a single 8-bit byte. Handy for storing LDR colors.
    //
    // see: https://www.khronos.org/opengl/wiki/Normalized_Integer
    class Unorm8 final {
    public:
        constexpr Unorm8() = default;

        consteval Unorm8(int literal) :
            value_{static_cast<uint8_t>(literal)}
        {
            if (literal > std::numeric_limits<uint8_t>::max()) {
                throw std::runtime_error{"provided value is too large"};
            }
        }

        constexpr Unorm8(uint8_t raw_value) :
            value_{raw_value}
        {}

        constexpr Unorm8(std::byte raw_value) :
            value_{static_cast<uint8_t>(raw_value)}
        {}

        constexpr Unorm8(float normalized_value) :
            value_{to_normalized_uint8(normalized_value)}
        {}

        friend auto operator<=>(const Unorm8&, const Unorm8&) = default;

        explicit constexpr operator float() const
        {
            return normalized_value();
        }

        explicit constexpr operator uint8_t() const
        {
            return raw_value();
        }

        constexpr uint8_t raw_value() const
        {
            return value_;
        }

        constexpr float normalized_value() const
        {
            return (1.0f/255.0f) * static_cast<float>(value_);
        }

        constexpr std::byte byte() const
        {
            return static_cast<std::byte>(value_);
        }
    private:
        static constexpr uint8_t to_normalized_uint8(float v)
        {
            // care: NaN should return 0.0f
            const float saturated = v > 0.0f ? (v < 1.0f ? v : 1.0f) : 0.0f;
            return static_cast<uint8_t>(255.0f * saturated);
        }

        uint8_t value_ = 0;
    };

    // tag `Unorm8` as scalar-like, so that other parts of the codebase (e.g.
    // vectors, matrices) accept it
    template<>
    struct IsScalar<Unorm8> final {
        static constexpr bool value = true;
    };
}
