#pragma once

#include <oscar/Maths/Constants.h>
#include <oscar/Maths/Vec.h>

#include <concepts>
#include <cstddef>
#include <limits>
#include <utility>

namespace osc
{
    // A wrapper around a `Vec` that has either a length of one (to
    // within a very small tolerance) or all components set to NaNs
    template<size_t L, std::floating_point T>
    class UnitVec final {
    public:
        using value_type = T;
        using element_type = T;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = const T&;
        using const_reference = const T&;
        using pointer = const T*;
        using const_pointer = const T*;
        using iterator = const T*;
        using const_iterator = const T*;

        // returns a `UnitVec` by constructing the underlying vector and assuming that it is already normalized
        template<typename... Args>
        requires std::constructible_from<Vec<L, T>, Args&&...>
        static constexpr UnitVec<L, T> already_normalized(Args&&... args)
        {
            return UnitVec<L, T>(AlreadyNormalizedTag{}, std::forward<Args>(args)...);
        }

        // returns a unit vector with X = T{1.0}
        static constexpr UnitVec<L, T> along_x()
            requires (L >= 1)
        {
            Vec<L, T> v{};
            v.x = T{1.0};
            return already_normalized(v);
        }

        // returns a unit vector with Y = T{1.0}
        static constexpr UnitVec<L, T> along_y()
            requires (L >= 2)
        {
            Vec<L, T> v{};
            v.y = T{1.0};
            return already_normalized(v);
        }

        // returns a unit vector with Z = T{1.0}
        static constexpr UnitVec<L, T> along_z()
            requires (L >= 3)
        {
            Vec<L, T> v{};
            v.z = T{1.0};
            return already_normalized(v);
        }

        // constructs a `UnitVec` containing NaNs
        constexpr UnitVec() = default;

        // constructs a `UnitVec` by constructing the underlying `Vec`, followed by normalizing it
        template<typename... Args>
        requires std::constructible_from<Vec<L, T>, Args&&...>
        explicit UnitVec(Args&&... args) :
            data_{normalize(Vec<L, T>{std::forward<Args>(args)...})}
        {}

        // implicit conversion to the underlying (normalized) vector
        constexpr operator const Vec<L, T>& () const { return data_; }

        // explicit conversion to the underlying (normalized) vector
        //
        // this is sometimes necessary when (e.g.) the compiler can't deduce the conversion
        constexpr const Vec<L, T>& unwrap() const { return data_; }

        constexpr size_type size() const { return L; }
        constexpr const_pointer data() const { return data_.data(); }
        constexpr const_iterator begin() const { return data(); }
        constexpr const_iterator end() const { return data() + size(); }
        constexpr const_reference operator[](size_type i) const { return begin()[i]; }

        constexpr UnitVec operator+() const
        {
            return UnitVec<L, T>::already_normalized(+data_);
        }

        constexpr UnitVec operator-() const
        {
            return UnitVec<L, T>::already_normalized(-data_);
        }

        friend constexpr bool operator==(const UnitVec&, const UnitVec&) = default;

    private:
        // tag struct that says "the provided data is already normalized, so skip normalization"
        struct AlreadyNormalizedTag final {};

        // constructs a `UnitVec` by constructing the underlying `Vec` and skips normalization
        template<typename... Args>
        requires std::constructible_from<Vec<L, T>, Args&&...>
        constexpr explicit UnitVec(AlreadyNormalizedTag, Args&&... args) :
            data_{Vec<L, T>{std::forward<Args>(args)...}}
        {}

        Vec<L, T> data_{quiet_nan_v<T>};
    };

    template<size_t L, std::floating_point T>
    constexpr Vec<L, T> operator*(T scalar, const UnitVec<L, T>& v)
    {
        return scalar * static_cast<const Vec<L, T>&>(v);
    }
}
