#pragma once

#include <concepts>
namespace osc { template<size_t, std::floating_point> class UnitVec; }

#include <oscar/Maths/Constants.h>
#include <oscar/Maths/GeometricFunctions.h>
#include <oscar/Maths/Vec.h>

#include <cstddef>
#include <limits>

namespace osc
{
    /**
     * A vector that has either:
     *
     * - A length of one (to within a very small tolerance), or
     * - All components are NaN (default construction - you should overwrite it)
     *
     * Inspired by Simbody's `SimTK::UnitVec` class
     */
    template<size_t L, std::floating_point T>
    class UnitVec final {
    public:
        using type = UnitVec<L, T>;
        using value_type = typename Vec<L, T>::value_type;
        using size_type = typename Vec<L, T>::size_type;

        // tag struct that says "the provided data is already normalized, so skip normalization"
        struct AlreadyNormalized final {};

        static constexpr size_type length() { return L; }

        // constructs a `UnitVec` containing NaNs
        constexpr UnitVec() = default;

        // constructs a `UnitVec` by constructing the underlying `Vec`, followed by normalizing it
        template<typename... Args>
        explicit UnitVec(Args&&... args)
            requires std::constructible_from<Vec<L, T>, Args&&...> :
            m_Data{normalize(Vec<L, T>{std::forward<Args>(args)...})}
        {}

        // constructs a `UnitVec` by constructing the underlying `Vec` and skips normalization
        template<typename... Args>
        constexpr explicit UnitVec(AlreadyNormalized, Args&&... args)
            requires std::constructible_from<Vec<L, T>, Args&&...> :
            m_Data{Vec<L, T>{std::forward<Args>(args)...}}
        {}

        // implicit conversion to the underlying (normalized) vector
        constexpr operator Vec<L, T> const& () const
        {
            return m_Data;
        }

        constexpr T const& operator[](size_type index) const
        {
            return m_Data[index];
        }

        constexpr UnitVec operator+() const
        {
            return UnitVec{AlreadyNormalized{}, +m_Data};
        }

        constexpr UnitVec operator-() const
        {
            return UnitVec{AlreadyNormalized{}, -m_Data};
        }

        friend constexpr bool operator==(UnitVec const&, UnitVec const&) = default;

    private:
        Vec<L, T> m_Data{quiet_nan<T>};
    };

    template<size_t L, std::floating_point T>
    constexpr Vec<L, T> operator*(T s, UnitVec<L, T> const& v)
    {
        return s * static_cast<Vec<L, T> const&>(v);
    }
}
