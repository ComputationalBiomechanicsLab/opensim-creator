#pragma once

#include <oscar/Shims/Cpp23/utility.h>

#include <functional>
#include <iterator>
#include <optional>
#include <type_traits>
#include <utility>

namespace osc
{
    // satisfied if an enum contains a `NUM_OPTIONS` member
    //
    // - enums that use `NUM_OPTIONS` are assumed to have their first value at
    //   zero, their last value at `NUM_OPTIONS-1`, and all options one-after-another
    //   with no gaps
    template<typename TEnum>
    concept DenselyPackedOptionsEnum = requires(TEnum v) {
        requires std::is_enum_v<TEnum>;
        TEnum::NUM_OPTIONS;
    };

    // returns the number of options in `TEnum` (effectively, returns `NUM_OPTIONS`)
    template<DenselyPackedOptionsEnum TEnum>
    constexpr size_t num_options()
    {
        return static_cast<size_t>(TEnum::NUM_OPTIONS);
    }

    // a compile-time list of options in `TEnum`
    //
    // - the caller must instantiate this template with each option
    template<DenselyPackedOptionsEnum TEnum, TEnum... TEnumOptions>
    struct OptionList {
        static_assert(sizeof...(TEnumOptions) == num_options<TEnum>());
    };

    // returns the value of `v` casted to a `size_t` (i.e. `v` should probably satisfy
    // `DenselyPackedOptionsEnum` for this to work
    template<DenselyPackedOptionsEnum TEnum>
    constexpr size_t to_index(TEnum v)
    {
        return static_cast<size_t>(cpp23::to_underlying(v));
    }

    // if `pos` is within the range of densely-packed enum options, returns the enum member
    // that has an integer value equal to `pos`; otherwise, returns `std::nullopt`
    template<DenselyPackedOptionsEnum TEnum>
    constexpr std::optional<TEnum> from_index(size_t pos)
    {
        if (pos < num_options<TEnum>()) {
            return static_cast<TEnum>(pos);
        }
        else {
            return std::nullopt;
        }
    }

    // an iterable adaptor for a `DenselyPackedOptionsEnum` that, when iterated, emits
    // each enum option projected via `Proj`
    template<
        DenselyPackedOptionsEnum TEnum,
        typename Proj = std::identity
    >
    class DenselyPackedOptionsIterable final {
    public:
        class Iterator {
        public:
            using difference_type = ptrdiff_t;
            using value_type = std::remove_cvref_t<decltype(Proj{}(std::declval<TEnum>()))>;
            using pointer = void;
            using reference = value_type;
            using iterator_category = std::forward_iterator_tag;

            Iterator() = default;
            Iterator(const Proj& proj, TEnum current) :
                current_{current},
                proj_{&proj}
            {}

            friend bool operator==(const Iterator&, const Iterator&) = default;

            value_type operator*() const
            {
                return std::invoke(*proj_, current_);
            }

            Iterator& operator++()
            {
                current_ = static_cast<TEnum>(cpp23::to_underlying(current_) + 1);
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator copy{*this};
                ++(*this);
                return copy;
            }
        private:
            TEnum current_ = static_cast<TEnum>(0);
            const Proj* proj_ = nullptr;
        };

        using value_type = decltype(Proj{}(std::declval<TEnum>()));
        using iterator = Iterator;
        using const_iterator = Iterator;

        constexpr DenselyPackedOptionsIterable(Proj proj = {}) :
            proj_{proj}
        {}

        auto front() const { return std::invoke(proj_, static_cast<TEnum>(0)); }
        auto back() const { return std::invoke(proj_, static_cast<TEnum>(cpp23::to_underlying(TEnum::NUM_OPTIONS)-1)); }
        Iterator begin() const { return Iterator{proj_, static_cast<TEnum>(0)}; }
        Iterator end() const { return Iterator{proj_, TEnum::NUM_OPTIONS}; }

    private:
        Proj proj_;
    };

    // returns a `DenselyPackedOptionsIterable` that, when iterated, projects each enum value via `proj`
    template<DenselyPackedOptionsEnum TEnum, typename Proj = std::identity>
    constexpr DenselyPackedOptionsIterable<TEnum, Proj> make_option_iterable(Proj&& proj = {})
    {
        return DenselyPackedOptionsIterable<TEnum, Proj>(std::forward<Proj>(proj));
    }

    // satisfied if the given enum contains a `NUM_FLAGS` member
    //
    // - enums that use `NUM_FLAGS` are assumed to have their first flag at 1<<0, their last
    //   flag at 1<<NUM_FLAGS-1, and all flags one-bit-next-to-the-other (i.e. dense)
    template<typename TEnum>
    concept FlagsEnum = requires(TEnum v) {
        requires std::is_enum_v<TEnum>;
        TEnum::NUM_FLAGS;
    };

    // returns the number of flags in `TEnum`
    template<FlagsEnum TEnum>
    constexpr size_t num_flags()
    {
        return static_cast<size_t>(TEnum::NUM_FLAGS);
    }
}
