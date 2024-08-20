#pragma once

#include <bit>
#include <compare>
#include <concepts>
#include <cstdint>
#include <iosfwd>

namespace osc
{
    class AntiAliasingLevel final {
    public:
        static constexpr AntiAliasingLevel min()
        {
            return AntiAliasingLevel{1};
        }

        static constexpr AntiAliasingLevel none()
        {
            return AntiAliasingLevel{1};
        }

        constexpr AntiAliasingLevel() = default;

        explicit constexpr AntiAliasingLevel(int value) :
            value_{value > 1 ? uint32_t(1) << (std::bit_width(static_cast<unsigned>(value))-1) : 1}
        {}

        template<std::integral T>
        constexpr T get_as() const
        {
            return static_cast<int32_t>(value_);
        }

        constexpr AntiAliasingLevel& operator++()
        {
            value_ <<= 1;
            return *this;
        }

        friend auto operator<=>(const AntiAliasingLevel&, const AntiAliasingLevel&) = default;
    private:
        uint32_t value_ = 1;
    };

    std::ostream& operator<<(std::ostream&, AntiAliasingLevel);
}
