#pragma once

#include <cstddef>
#include <cstdint>
#include <compare>
#include <limits>
#include <stdexcept>

namespace osc
{
    // Representation of an unsigned normalized floating-point value (i.e. [0, 1])
    // as a single 8-bit byte.
    //
    // Handy for storing LDR colors
    class Unorm8 final {
    public:
        Unorm8() = default;

        consteval Unorm8(int literalValue) :
            m_Value{static_cast<uint8_t>(literalValue)}
        {
            if (literalValue > std::numeric_limits<uint8_t>::max()) {
                throw std::runtime_error{"provided value is too large"};
            }
        }

        constexpr Unorm8(uint8_t rawValue) :
            m_Value{rawValue}
        {}

        constexpr Unorm8(std::byte rawValue) :
            m_Value{static_cast<uint8_t>(rawValue)}
        {}

        constexpr Unorm8(float normalizedValue) :
            m_Value{toNormalizedUint8(normalizedValue)}
        {}

        friend auto operator<=>(Unorm8 const&, Unorm8 const&) = default;

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
            return m_Value;
        }

        constexpr float normalized_value() const
        {
            return (1.0f/255.0f) * static_cast<float>(m_Value);
        }

        constexpr std::byte byte() const
        {
            return static_cast<std::byte>(m_Value);
        }
    private:
        static constexpr uint8_t toNormalizedUint8(float v)
        {
            // care: NaN should return 0.0f
            float const saturated = v > 0.0f ? (v < 1.0f ? v : 1.0f) : 0.0f;
            return static_cast<uint8_t>(255.0f * saturated);
        }

        uint8_t m_Value;
    };
}
