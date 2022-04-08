#pragma once

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <iostream>

namespace osc
{
    // evaluate the length of a string at compile-time
    constexpr std::size_t StrLen(char const* s) noexcept
    {
        std::size_t len = 0;
        while (*s++ != char{0})
        {
            ++len;
        }
        return len;
    }

    // represents a view into a NUL-terminated C string
    class CStringView final {
    public:
        constexpr CStringView() noexcept : m_Data{nullptr}, m_Size{0} {}
        constexpr CStringView(CStringView const&) noexcept = default;
        constexpr CStringView& operator=(CStringView const&) noexcept = default;
        constexpr CStringView(char const* s) : m_Data{s}, m_Size{StrLen(s)} {}
        CStringView(std::string const& s) : m_Data{s.c_str()}, m_Size{s.size()} {}
        constexpr CStringView(std::nullptr_t) = delete;

        constexpr std::size_t size() const noexcept { return m_Size; }
        constexpr std::size_t length() const noexcept { return m_Size; }
        constexpr bool empty() const noexcept { return m_Size == 0; }
        constexpr char const* c_str() const noexcept { return m_Data; }
        constexpr operator std::string_view () const noexcept { return std::string_view{m_Data, m_Size}; }

    private:
        char const* m_Data;
        std::size_t m_Size;
    };

    inline std::string to_string(CStringView const& sv)
    {
        return std::string{sv};
    }

    inline std::ostream& operator<<(std::ostream& o, CStringView const& sv)
    {
        return o << std::string_view{sv};
    }

    inline std::string operator+(char const* c, CStringView const& sv)
    {
        return c + to_string(sv);
    }

    inline std::string operator+(std::string const& s, CStringView const& sv)
    {
        return s + to_string(sv);
    }
}

namespace std
{
    template<>
    struct hash<osc::CStringView> {
        std::size_t operator()(osc::CStringView const& sv) const
        {
            return std::hash<std::string_view>{}(sv);
        }
    };
}
