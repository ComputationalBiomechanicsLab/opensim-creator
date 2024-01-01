#pragma once

#include <cstddef>
#include <cstdint>
#include <compare>

namespace osc::detail
{
    // CPU representation of an 8-bit unsigned integer that represents
    // a "normalized" (i.e. [0, 1]) floating-point value
    class Unorm8 final {
    public:
        Unorm8() = default;
        explicit constexpr Unorm8(std::byte rawValue) : m_Value{static_cast<uint8_t>(rawValue)} {}
        explicit constexpr Unorm8(float normalizedValue) : m_Value{toNormalizedUint8(normalizedValue)} {}

        friend auto operator<=>(Unorm8 const&, Unorm8 const&) = default;

        explicit operator float() const { return normalized(); }
        explicit operator uint8_t() const { return raw(); }

        std::byte byte() const { return static_cast<std::byte>(m_Value); }
        uint8_t raw() const { return m_Value; }
        float normalized() const { return (1.0f/255.0f) * static_cast<float>(m_Value); };
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
