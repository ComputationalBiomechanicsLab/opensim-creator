#pragma once

#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/SharedPreHashedString.h>

#include <compare>
#include <concepts>
#include <cstddef>
#include <string_view>

namespace osc
{
    // immutable, globally unique string with fast hashing+equality
    class StringName final : public SharedPreHashedString {
    public:
        StringName() = default;
        explicit StringName(std::string_view);
        explicit StringName(const char* s) : StringName{std::string_view{s}} {}
        explicit StringName(std::nullptr_t) = delete;
        StringName(const StringName&) = default;
        StringName(StringName&&) noexcept = default;
        StringName& operator=(const StringName&) = default;
        StringName& operator=(StringName&&) noexcept = default;
        ~StringName() noexcept;

        using SharedPreHashedString::operator CStringView;
        using SharedPreHashedString::operator std::string_view;
        using SharedPreHashedString::operator[];

        friend bool operator==(const StringName&, const StringName&) = default;

        template<typename StringViewLike>
        requires std::constructible_from<std::string_view, StringViewLike>
        friend bool operator==(const StringName& lhs, const StringViewLike& rhs)
        {
            return std::string_view{lhs} == rhs;
        }

        template<typename StringViewLike>
        requires std::constructible_from<std::string_view, StringViewLike>
        friend bool operator==(const StringViewLike& lhs, const StringName& rhs)
        {
            return lhs == std::string_view{rhs};
        }

        friend auto operator<=>(const StringName&, const StringName&) = default;

        template<typename StringViewLike>
        requires std::constructible_from<std::string_view, StringViewLike>
        friend auto operator<=>(const StringName& lhs, const StringViewLike& rhs)
        {
            return std::string_view{lhs} <=> rhs;
        }

        template<typename StringViewLike>
        requires std::constructible_from<std::string_view, StringViewLike>
        friend auto operator<=>(const StringViewLike& lhs, const StringName& rhs)
        {
            return lhs <=> std::string_view{rhs};
        }
    };
}

template<>
struct std::hash<osc::StringName> final {
    size_t operator()(const osc::StringName& sn) const
    {
        return std::hash<osc::SharedPreHashedString>{}(sn);
    }
};
