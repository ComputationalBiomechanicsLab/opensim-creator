#pragma once

#include <compare>
#include <cstddef>
#include <string>
#include <string_view>
#include <iosfwd>

namespace osc
{
    // represents a readonly view into a NUL-terminated C string
    class CStringView final {
    public:
        using value_type = std::string_view::value_type;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using iterator = value_type*;
        using const_iterator = const value_type*;
        using size_type = std::string_view::size_type;

        constexpr CStringView() = default;

        constexpr CStringView(const value_type* s) :
            data_{s ? s : ""}
        {}

        constexpr CStringView(const value_type* s, size_type count) :
            data_{s},
            size_{count}
        {}

        constexpr CStringView(std::nullptr_t) :
            CStringView{}
        {}

        CStringView(const std::string& s) :
            data_{s.c_str()},
            size_{s.size()}
        {}

        constexpr CStringView(const CStringView&) = default;
        constexpr CStringView(CStringView&&) noexcept = default;
        constexpr CStringView& operator=(const CStringView&) = default;
        constexpr CStringView& operator=(CStringView&&) noexcept = default;

        constexpr size_type size() const
        {
            return size_;
        }

        constexpr size_type length() const
        {
            return size_;
        }

        [[nodiscard]] constexpr bool empty() const
        {
            return size_ == 0;
        }

        constexpr const value_type* c_str() const
        {
            return data_;
        }

        constexpr operator std::string_view () const
        {
            return std::string_view{data_, size_};
        }

        constexpr const_iterator begin() const
        {
            return data_;
        }

        constexpr const_iterator end() const
        {
            return data_ + size_;
        }

        constexpr const_pointer data() const
        {
            return data_;
        }

        constexpr friend bool operator==(const CStringView& lhs, const CStringView& rhs)
        {
            return static_cast<std::string_view>(lhs) == static_cast<std::string_view>(rhs);
        }
        constexpr friend auto operator<=>(const CStringView& lhs, const CStringView& rhs)
        {
            return static_cast<std::string_view>(lhs) <=> static_cast<std::string_view>(rhs);
        }
    private:
        const value_type* data_ = "";
        size_type size_ = std::string_view{data_}.size();
    };

    namespace literals
    {
        constexpr CStringView operator""_cs(const char* str, size_t len)
        {
            return CStringView{str, len};
        }
    }

    inline std::string to_string(const CStringView& cstrv)
    {
        return std::string{cstrv};
    }

    std::ostream& operator<<(std::ostream&, const CStringView&);
    std::string operator+(const char*, const CStringView&);
    std::string operator+(const std::string&, const CStringView&);
}

template<>
struct std::hash<osc::CStringView> final {
    size_t operator()(const osc::CStringView& cstrv) const
    {
        return std::hash<std::string_view>{}(cstrv);
    }
};
