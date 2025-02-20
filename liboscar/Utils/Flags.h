#pragma once

#include <bit>
#include <concepts>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <type_traits>

namespace osc
{
    // a helper class that stores `OR` combinations of flag-like enum values
    //
    // note, the templated enum class must:
    //
    // - have a `None` member that is equal to zero (i.e. `std::to_underlying(None) == 0`)
    // - store the flags densely, with no gaps
    // - have a `NUM_FLAGS` member that contains the count of densely-stored flags (e.g. 1<<(NUM_FLAGS-1) is the highest flag)
    template<typename TEnum>
    requires std::is_enum_v<TEnum>
    class Flags final {
    public:
        using underlying_type = std::underlying_type_t<TEnum>;

        static constexpr Flags from_underlying(underlying_type v)
        {
            return Flags{v};
        }

        constexpr Flags() = default;
        constexpr Flags(TEnum flag) : value_{static_cast<underlying_type>(flag)} {}
        constexpr Flags(std::initializer_list<TEnum> flags)
        {
            for (TEnum flag : flags) {
                value_ |= static_cast<underlying_type>(flag);
            }
        }

        constexpr bool operator!() const
        {
            return !static_cast<bool>(*this);
        }

        explicit constexpr operator bool () const
        {
            return value_ != 0;
        }

        friend constexpr bool operator==(const Flags&, const Flags&) = default;

        friend constexpr Flags operator&(const Flags& lhs, const Flags& rhs)
        {
            return Flags{static_cast<underlying_type>(lhs.value_ & rhs.value_)};
        }

        friend constexpr Flags operator|(const Flags& lhs, const Flags& rhs)
        {
            return Flags{static_cast<underlying_type>(lhs.value_ | rhs.value_)};
        }

        constexpr Flags& operator|=(const Flags& rhs)
        {
            value_ |= rhs.value_;
            return *this;
        }

        constexpr TEnum lowest_set() const
        {
            if (value_ != 0) {
                const auto unsigned_val = static_cast<std::make_unsigned_t<underlying_type>>(value_);
                return static_cast<TEnum>(underlying_type{1} << std::countr_zero(unsigned_val));
            }
            return static_cast<TEnum>(underlying_type{0});
        }

        constexpr Flags with(TEnum flag) const
        {
            return Flags{static_cast<underlying_type>(value_ | static_cast<underlying_type>(flag))};
        }

        constexpr Flags without(TEnum flag) const
        {
            return Flags{static_cast<underlying_type>(value_ & ~static_cast<underlying_type>(flag))};
        }

        constexpr bool get(TEnum flag) const
        {
            return static_cast<bool>(*this & flag);
        }

        constexpr void set(TEnum flag, bool v)
        {
            *this = v ? this->with(flag) : this->without(flag);
        }

        constexpr Flags with_flag_values_swapped(TEnum flag0, TEnum flag1) const
        {
            Flags copy{*this};
            copy = (*this & flag0) ? copy.with(flag1) : copy.without(flag1);
            copy = (*this & flag1) ? copy.with(flag0) : copy.without(flag0);
            return copy;
        }

        constexpr underlying_type underlying_value() const
        {
            return value_;
        }

    private:
        explicit constexpr Flags(underlying_type value) : value_{value} {}

        underlying_type value_{};
    };

    template<class TEnum>
    requires std::is_enum_v<TEnum>
    constexpr auto to_underlying(const Flags<TEnum>& e) noexcept
    {
        return e.underlying_value();
    }
}

template<typename T>
struct std::hash<osc::Flags<T>> final {
    size_t operator()(const osc::Flags<T>& flags) const noexcept
    {
        return inner_hasher_(flags.underlying_value());
    }
private:
    std::hash<typename osc::Flags<T>::underlying_type> inner_hasher_{};
};
