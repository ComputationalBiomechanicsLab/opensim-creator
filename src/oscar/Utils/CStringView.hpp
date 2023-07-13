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
    constexpr size_t StrLen(char const* s) noexcept
    {
        if (s == nullptr)
        {
            return 0;
        }

        size_t len = 0;
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
        constexpr CStringView(char const* s) noexcept : m_Data{s}, m_Size{StrLen(s)} {}
        constexpr CStringView(std::nullptr_t) noexcept : CStringView{} {}
        CStringView(std::string const& s) noexcept : m_Data{s.c_str()}, m_Size{s.size()} {}
        constexpr CStringView& operator=(CStringView const&) noexcept = default;

        // explicitly necessary because arrays might otherwise decay into pointers
	template<size_t N>
        static constexpr CStringView FromArray(char const (&s)[N]) noexcept
	{
            return {&s[0], N};
	}

        constexpr size_t size() const noexcept { return m_Size; }
        constexpr size_t length() const noexcept { return m_Size; }
        constexpr bool empty() const noexcept { return m_Size == 0; }
        constexpr char const* c_str() const noexcept { return m_Data; }
        constexpr operator std::string_view () const noexcept { return std::string_view{m_Data, m_Size}; }
        constexpr char const* begin() const noexcept { return m_Data; }
        constexpr char const* end() const noexcept { return m_Data + m_Size; }

    private:
        constexpr CStringView(char const* data_, size_t size_) noexcept : m_Data{data_}, m_Size{size_} {}

        friend bool operator<(CStringView const&, CStringView const&);
        friend bool operator==(CStringView const&, CStringView const&);
        friend bool operator!=(CStringView const&, CStringView const&);

        char const* m_Data;
        size_t m_Size;
    };

    inline bool operator<(CStringView const& a, CStringView const& b)
    {
        return static_cast<std::string_view>(a) < static_cast<std::string_view>(b);
    }

    inline bool operator==(CStringView const& a, CStringView const& b)
    {
        return static_cast<std::string_view>(a) == static_cast<std::string_view>(b);
    }

    inline bool operator!=(CStringView const& a, CStringView const& b)
    {
        return static_cast<std::string_view>(a) != static_cast<std::string_view>(b);
    }

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
        size_t operator()(osc::CStringView const& sv) const
        {
            return std::hash<std::string_view>{}(sv);
        }
    };
}
