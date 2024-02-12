#pragma once

#include <oscar/Shims/Cpp20/bit.h>

#include <iosfwd>
#include <string>

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
            m_Value{value > 1 ? uint32_t(1) << (cpp20::bit_width(static_cast<unsigned>(value))-1) : 1}
        {
        }

        constexpr int32_t getI32() const
        {
            return static_cast<int32_t>(m_Value);
        }

        constexpr uint32_t getU32() const
        {
            return m_Value;
        }

        constexpr AntiAliasingLevel& operator++()
        {
            m_Value <<= 1;
            return *this;
        }

        friend auto operator<=>(AntiAliasingLevel const&, AntiAliasingLevel const&) = default;
    private:
        uint32_t m_Value = 1;
    };

    std::ostream& operator<<(std::ostream&, AntiAliasingLevel);
    std::string to_string(AntiAliasingLevel);
}
