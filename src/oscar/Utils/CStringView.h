#pragma once

#include <oscar/Shims/Cpp20/string_view.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <iosfwd>

namespace osc
{
    // represents a view into a NUL-terminated C string
    class CStringView final {
    public:
        using value_type = std::string_view::value_type;
        using pointer = value_type*;
        using const_pointer = value_type const*;
        using iterator = value_type*;
        using const_iterator = value_type const*;

        constexpr CStringView() = default;

        constexpr CStringView(value_type const* s) :
            m_Data{s ? s : ""}
        {
        }

        constexpr CStringView(std::nullptr_t) :
            CStringView{}
        {
        }

        CStringView(std::string const& s) :
            m_Data{s.c_str()},
            m_Size{s.size()}
        {
        }

        constexpr CStringView(CStringView const&) = default;
        constexpr CStringView(CStringView&&) noexcept = default;
        constexpr CStringView& operator=(CStringView const&) = default;
        constexpr CStringView& operator=(CStringView&&) noexcept = default;

        constexpr size_t size() const
        {
            return m_Size;
        }

        constexpr size_t length() const
        {
            return m_Size;
        }

        [[nodiscard]] constexpr bool empty() const
        {
            return m_Size == 0;
        }

        constexpr value_type const* c_str() const
        {
            return m_Data;
        }

        constexpr operator std::string_view () const
        {
            return std::string_view{m_Data, m_Size};
        }

        constexpr const_iterator begin() const
        {
            return m_Data;
        }

        constexpr const_iterator end() const
        {
            return m_Data + m_Size;
        }

        constexpr const_pointer data() const
        {
            return m_Data;
        }

        constexpr friend bool operator==(CStringView const& lhs, CStringView const& rhs)
        {
            return static_cast<std::string_view>(lhs) == static_cast<std::string_view>(rhs);
        }
        constexpr friend auto operator<=>(CStringView const& lhs, CStringView const& rhs)
        {
            return cpp20::ThreeWayComparison(std::string_view{lhs}, std::string_view{rhs});
        }
    private:
        value_type const* m_Data = "";
        size_t m_Size = std::string_view{m_Data}.size();
    };


    inline std::string to_string(CStringView const& sv)
    {
        return std::string{sv};
    }

    std::ostream& operator<<(std::ostream&, CStringView const&);
    std::string operator+(char const*, CStringView const&);
    std::string operator+(std::string const&, CStringView const&);
}

template<>
struct std::hash<osc::CStringView> final {
    size_t operator()(osc::CStringView const& sv) const
    {
        return std::hash<std::string_view>{}(sv);
    }
};
