#pragma once

#include <algorithm>
#include <format>
#include <optional>
#include <ostream>
#include <type_traits>
#include <utility>

namespace osc
{
    /// Manages a value, or a sentinel value, which usually means "no value".
    /// Useful as an alternative to `std::optional<T>` when trying to save
    /// memory - and when there is an obvious sentinel value.
    template<typename T, T Sentinel>
    class ValueOrSentinel final {
    public:
        using value_type = T;

        static constexpr T sentinel() { return Sentinel; }

        constexpr ValueOrSentinel() = default;

        template<class U = std::remove_cv_t<T>>
        requires std::constructible_from<T, U&&>
        constexpr ValueOrSentinel(U&& value) : value_(std::forward<U>(value)) {}

        friend constexpr bool operator==(ValueOrSentinel, ValueOrSentinel) = default;

        template<typename U>
        friend constexpr bool operator==(const ValueOrSentinel& lhs, const U& rhs) { return *lhs == rhs; }

        template<typename U>
        friend constexpr bool operator==(const U& lhs, const ValueOrSentinel& rhs) { return lhs == *rhs; }

        explicit constexpr operator bool () const { return value_ != Sentinel; }

        constexpr const T&  operator*() const&  noexcept { return value_; }
        constexpr       T&  operator*()      &  noexcept { return value_; }
        constexpr const T&& operator*() const&& noexcept { return std::move(value_); }
        constexpr       T&& operator*()      && noexcept { return std::move(value_); }
        constexpr const T&  value()     const&           { return *this ? value_            : throw std::bad_optional_access{}; }
        constexpr       T&  value()          &           { return *this ? value_            : throw std::bad_optional_access{}; }
        constexpr const T&& value()     const&&          { return *this ? std::move(value_) : throw std::bad_optional_access{}; }
        constexpr       T&& value()          &&          { return *this ? std::move(value_) : throw std::bad_optional_access{}; }

    private:
        T value_ = Sentinel;
    };

    template<typename T, T Sentinel>
    std::ostream& operator<<(std::ostream& lhs, const ValueOrSentinel<T, Sentinel>& rhs)
    {
        return lhs << std::format("{}", rhs);
    }
}

template<typename T, T Sentinel>
struct std::formatter<osc::ValueOrSentinel<T, Sentinel>> {
    template<class ParseCtx>
    constexpr auto parse(ParseCtx& ctx) { return inner_.parse(ctx); }

    template<class FmtCtx>
    auto format(const osc::ValueOrSentinel<T, Sentinel>& v, FmtCtx& ctx) const
    {
        auto it = std::ranges::copy(std::string_view{"ValueOrSentinel("}, ctx.out()).out;
        it = inner_.format(*v, ctx);
        *it++ = ')';
        return it;
    }
private:
    std::formatter<T> inner_;
};
