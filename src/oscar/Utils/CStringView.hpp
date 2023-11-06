#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <iostream>

namespace osc
{
    // represents a view into a NUL-terminated C string
    class CStringView final {
    public:
        using value_type = std::string_view::value_type;

        // factory function for constructing a CStringView from an array of known
        // compile-time size
        template<size_t N>
        static constexpr CStringView FromArray(char const (&s)[N]) noexcept
        {
            return {&s[0], N};
        }

        constexpr CStringView() noexcept :
            m_Data{""},
            m_Size{0}
        {
        }
        constexpr CStringView(char const* s) noexcept :
            m_Data{s ? s : ""},
            m_Size{std::string_view{m_Data}.size()}
        {
        }
        constexpr CStringView(std::nullptr_t) noexcept :
            CStringView{}
        {
        }
        CStringView(std::string const& s) noexcept :
            m_Data{s.c_str()},
            m_Size{s.size()}
        {
        }
        constexpr CStringView(CStringView const&) noexcept = default;
        constexpr CStringView& operator=(CStringView const&) noexcept = default;

        constexpr size_t size() const noexcept { return m_Size; }
        constexpr size_t length() const noexcept { return m_Size; }
        constexpr bool empty() const noexcept { return m_Size == 0; }
        constexpr char const* c_str() const noexcept { return m_Data; }
        constexpr operator std::string_view () const noexcept { return std::string_view{m_Data, m_Size}; }
        constexpr char const* begin() const noexcept { return m_Data; }
        constexpr char const* end() const noexcept { return m_Data + m_Size; }

        constexpr friend auto operator<=>(CStringView const& lhs, CStringView const& rhs)
        {
            // manual implementation because MacOS's stdlib doesn't have <=> yet
            int v = static_cast<std::string_view>(lhs).compare(static_cast<std::string_view>(rhs));
            return v < 0 ? std::strong_ordering::less : v == 0 ? std::strong_ordering::equal : std::strong_ordering::greater;
        }
        constexpr friend bool operator==(CStringView const& lhs, CStringView const& rhs)
        {
            return static_cast<std::string_view>(lhs) == static_cast<std::string_view>(rhs);
        }
    private:
        constexpr CStringView(char const* data_, size_t size_) noexcept : m_Data{data_}, m_Size{size_} {}

        char const* m_Data;
        size_t m_Size;
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

template<>
struct std::hash<osc::CStringView> final {
    size_t operator()(osc::CStringView const& sv) const
    {
        return std::hash<std::string_view>{}(sv);
    }
};
