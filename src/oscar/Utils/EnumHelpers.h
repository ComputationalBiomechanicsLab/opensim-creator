#pragma once

#include <oscar/Shims/Cpp23/utility.h>

#include <concepts>
#include <cstddef>
#include <functional>
#include <iterator>
#include <type_traits>

namespace osc
{
    template<typename TEnum>
    concept DenselyPackedOptionsEnum = requires(TEnum v) {
        requires std::is_enum_v<TEnum>;
        TEnum::NUM_OPTIONS;
    };

    template<DenselyPackedOptionsEnum TEnum>
    constexpr size_t NumOptions()
    {
        return static_cast<size_t>(TEnum::NUM_OPTIONS);
    }

    template<DenselyPackedOptionsEnum TEnum, TEnum... TEnumOptions>
    struct OptionList {
        static_assert(sizeof...(TEnumOptions) == NumOptions<TEnum>());
    };

    template<DenselyPackedOptionsEnum TEnum>
    constexpr size_t ToIndex(TEnum v)
    {
        return cpp23::to_underlying(v);
    }

    template<DenselyPackedOptionsEnum TEnum, typename Proj = std::identity>
    class DenselyPackedOptionsIterable {
    public:
        class Iterator {
        public:
            using difference_type = ptrdiff_t;
            using value_type = decltype(Proj{}(std::declval<TEnum>()));
            using pointer = void;
            using reference = value_type;
            using iterator_category = std::forward_iterator_tag;

            Iterator() = default;
            Iterator(Proj& proj_, TEnum current_) : m_Proj{&proj_}, m_Current{current_} {}

            friend bool operator==(Iterator const&, Iterator const&) = default;

            value_type operator*() const
            {
                return std::invoke(*m_Proj, m_Current);
            }
            Iterator& operator++()
            {
                m_Current = static_cast<TEnum>(cpp23::to_underlying(m_Current) + 1);
                return *this;
            }
            Iterator operator++(int)
            {
                Iterator copy{*this};
                ++(*this);
                return copy;
            }
        private:
            TEnum m_Current = static_cast<TEnum>(0);
            Proj* m_Proj = nullptr;
        };

        constexpr DenselyPackedOptionsIterable(Proj proj = {}) :
            m_Proj{proj}
        {}

        Iterator begin() { return Iterator{m_Proj, static_cast<TEnum>(0)}; }
        Iterator end() { return Iterator{m_Proj, TEnum::NUM_OPTIONS}; }

    private:
        Proj m_Proj;
    };

    template<DenselyPackedOptionsEnum TEnum, typename Proj = std::identity>
    constexpr DenselyPackedOptionsIterable<TEnum, Proj> make_option_iterable(Proj&& proj)
    {
        return DenselyPackedOptionsIterable<TEnum, Proj>(std::forward<Proj>(proj));
    }

    template<typename TEnum>
    concept FlagsEnum = requires(TEnum v) {
        requires std::is_enum_v<TEnum>;
        TEnum::NUM_FLAGS;
    };

    template<FlagsEnum TEnum>
    constexpr size_t NumFlags()
    {
        return static_cast<size_t>(TEnum::NUM_FLAGS);
    }
}
