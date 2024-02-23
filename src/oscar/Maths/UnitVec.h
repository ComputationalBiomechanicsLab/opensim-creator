#pragma once

#include <oscar/Maths/Constants.h>
#include <oscar/Maths/Vec.h>

#include <concepts>
#include <cstddef>
#include <limits>
#include <utility>

namespace osc
{

    // A vector that has either:
    //
    // - A length of one (to within a very small tolerance), or
    // - All components are NaN (default construction - you should overwrite it)
    //
    // Inspired by Simbody's `SimTK::UnitVec` class
    template<size_t L, std::floating_point T>
    class UnitVec final {
    public:
        using value_type = T;
        using element_type = T;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = T const&;
        using const_reference = T const&;
        using pointer = T const*;
        using const_pointer = T const*;
        using iterator = T const*;
        using const_iterator = T const*;
        using type = UnitVec<2, T>;

        static constexpr size_type length() { return L; }

        // helper: constructs a UnitVec with arguments that are assumed to produce
        // a normalized underlying vector without checking
        template<typename... Args>
        constexpr static UnitVec<L, T> AlreadyNormalized(Args&&... args)
            requires std::constructible_from<Vec<L, T>, Args&&...>
        {
            return UnitVec<L, T>(AlreadyNormalizedTag{}, std::forward<Args>(args)...);
        }

        // constructs a `UnitVec` containing NaNs
        constexpr UnitVec() = default;

        // constructs a `UnitVec` by constructing the underlying `Vec`, followed by normalizing it
        template<typename... Args>
        UnitVec(Args&&... args)
            requires std::constructible_from<Vec<L, T>, Args&&...> :

            m_Data{normalize(Vec<L, T>{std::forward<Args>(args)...})}
        {}

        // implicit conversion to the underlying (normalized) vector
        constexpr operator Vec<L, T> const& () const { return m_Data; }

        constexpr size_type size() const { return length(); }
        constexpr const_pointer data() const { return m_Data.data(); }
        constexpr const_iterator begin() const { return data(); }
        constexpr const_iterator end() const { return data() + size(); }
        constexpr const_reference operator[](size_type i) const { return begin()[i]; }

        constexpr UnitVec operator+() const
        {
            return UnitVec<3, T>::AlreadyNormalized(+m_Data);
        }

        constexpr UnitVec operator-() const
        {
            return UnitVec<3, T>::AlreadyNormalized(-m_Data);
        }

        friend constexpr bool operator==(UnitVec const&, UnitVec const&) = default;

    private:
        // tag struct that says "the provided data is already normalized, so skip normalization"
        struct AlreadyNormalizedTag final {};

        // constructs a `UnitVec` by constructing the underlying `Vec` and skips normalization
        template<typename... Args>
        constexpr explicit UnitVec(AlreadyNormalizedTag, Args&&... args)
            requires std::constructible_from<Vec<L, T>, Args&&...> :
            m_Data{Vec<L, T>{std::forward<Args>(args)...}}
        {}

        Vec<L, T> m_Data{quiet_nan_v<T>};
    };

    template<size_t L, std::floating_point T>
    constexpr Vec<L, T> operator*(T s, UnitVec<L, T> const& v)
    {
        return s * static_cast<Vec<L, T> const&>(v);
    }
}
