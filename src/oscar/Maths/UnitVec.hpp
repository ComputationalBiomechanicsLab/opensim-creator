#pragma once

#include <oscar/Maths/LengthType.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Qualifier.hpp>
#include <oscar/Maths/Vec.hpp>

#include <glm/detail/qualifier.hpp>

#include <concepts>
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
    template<
        LengthType L,
        typename T,
        Qualifier Q = glm::defaultp
    >
    class UnitVec final {
    public:
        using type = UnitVec<L, T, Q>;
        using value_type = typename Vec<L, T, Q>::value_type;
        using length_type = typename Vec<L, T, Q>::length_type;
        static inline constexpr T nan_type = std::numeric_limits<T>::has_signaling_NaN ? std::numeric_limits<T>::signaling_NaN() : std::numeric_limits<T>::quiet_NaN();

        static constexpr length_type length()
        {
            return L;
        }

        // constructs a `UnitVec` containing NaNs
        constexpr UnitVec() = default;

        // constructs a `UnitVec` via copy/move (trivial)
        constexpr UnitVec(UnitVec const&) = default;
        constexpr UnitVec(UnitVec&&) noexcept = default;

        // constructs a UnitVec by normalizing the provided `Vec`
        explicit UnitVec(Vec<L, T, Q> const& v) :
            m_Data{Normalize(v)}
        {}

        // constructs a UnitVec by normalizing the provided `Vec`
        explicit UnitVec(Vec<L, T, Q>&& v) :
            m_Data{Normalize(std::move(v))}
        {}

        // constructs a `UnitVec` by constructing the underlying `Vec`, followed by normalizing it
        template<typename... Args>
        explicit UnitVec(Args&&... args)
            requires std::constructible_from<Vec<L, T, Q>, Args&&...> :
            UnitVec{Vec<L, T, Q>{std::forward<Args>(args)...}}
        {}

        // copy/move assignment (trivial)
        constexpr UnitVec& operator=(UnitVec const&) = default;
        constexpr UnitVec& operator=(UnitVec&&) noexcept = default;

        // implicit conversion to the underlying (normalized) vector
        constexpr operator Vec<L, T, Q> const& () const
        {
            return m_Data;
        }

        constexpr T const& operator[](length_type index) const
        {
            return m_Data[index];
        }

        constexpr UnitVec operator+() const
        {
            return {+m_Data, AlreadyNormalized{}};
        }

        constexpr UnitVec operator-() const
        {
            return {-m_Data, AlreadyNormalized{}};
        }

        friend constexpr bool operator==(UnitVec const&, UnitVec const&) = default;

    private:
        // tag struct that says "the provided vector is already normalized, so skip normalization"
        struct AlreadyNormalized final {};

        constexpr UnitVec(Vec<L, T, Q> const& v, AlreadyNormalized) :
            m_Data{v}
        {}

        Vec<L, T, Q> m_Data{nan_type};
    };
}
