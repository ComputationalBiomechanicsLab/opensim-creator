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
        using const_pointer = const value_type*;
        using iterator = value_type*;
        using const_iterator = const value_type*;

        constexpr CStringView() = default;

        constexpr CStringView(const value_type* s) :
            m_Data{s ? s : ""}
        {}

        constexpr CStringView(std::nullptr_t) :
            CStringView{}
        {}

        CStringView(const std::string& s) :
            m_Data{s.c_str()},
            m_Size{s.size()}
        {}

        constexpr CStringView(const CStringView&) = default;
        constexpr CStringView(CStringView&&) noexcept = default;
        constexpr CStringView& operator=(const CStringView&) = default;
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

        constexpr const value_type* c_str() const
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

        constexpr friend bool operator==(const CStringView& lhs, const CStringView& rhs)
        {
            return static_cast<std::string_view>(lhs) == static_cast<std::string_view>(rhs);
        }
        constexpr friend auto operator<=>(const CStringView& lhs, const CStringView& rhs)
        {
            return cpp20::ThreeWayComparison(std::string_view{lhs}, std::string_view{rhs});
        }
    private:
        const value_type* m_Data = "";
        size_t m_Size = std::string_view{m_Data}.size();
    };


    inline std::string to_string(const CStringView& sv)
    {
        return std::string{sv};
    }

    std::ostream& operator<<(std::ostream&, const CStringView&);
    std::string operator+(const char*, const CStringView&);
    std::string operator+(const std::string&, const CStringView&);
}

template<>
struct std::hash<osc::CStringView> final {
    size_t operator()(const osc::CStringView& sv) const
    {
        return std::hash<std::string_view>{}(sv);
    }
};
