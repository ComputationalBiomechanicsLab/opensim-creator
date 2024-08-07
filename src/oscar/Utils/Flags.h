#pragma once

#include <bit>
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

        constexpr Flags() = default;
        constexpr Flags(TEnum flag) : value_{static_cast<underlying_type>(flag)} {}
        constexpr Flags(std::initializer_list<TEnum> flags)
        {
            for (TEnum flag : flags) {
                *this |= flag;
            }
        }

        constexpr bool operator!() const
        {
            return !static_cast<bool>(*this);
        }

        constexpr operator bool () const
        {
            return value_ != 0;
        }

        friend constexpr bool operator==(const Flags&, const Flags&) = default;

        friend constexpr Flags operator&(const Flags& lhs, const Flags& rhs)
        {
            return Flags{lhs.value_ & rhs.value_};
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

    private:
        explicit constexpr Flags(underlying_type value) : value_{value} {}

        underlying_type value_{};
    };
}
