#pragma once

#include <liboscar/Maths/Scalar.h>
#include <liboscar/Maths/Vector.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

namespace osc
{
    template<ScalarOrBoolean T>
    struct Vector<T, 3> {
        using value_type = T;
        using element_type = T;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using iterator = T*;
        using const_iterator = const T*;

        constexpr Vector() = default;
        explicit constexpr Vector(T value) :
            x{value},
            y{value},
            z{value}
        {}
        constexpr Vector(T x_, T y_, T z_) :
            x{x_},
            y{y_},
            z{z_}
        {}
        template<typename X, typename Y, typename Z>
        requires std::constructible_from<T, X> and std::constructible_from<T, Y> and std::constructible_from<T, Z>
        explicit (not (std::convertible_to<T, X> and std::convertible_to<T, Y> and std::convertible_to<T, Z>))
        constexpr Vector(X x_, Y y_, Z z_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(y_)},
            z{static_cast<T>(z_)}
        {}
        template<ScalarOrBoolean A, typename B>
        requires std::constructible_from<T, A> and std::constructible_from<T, B>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B>))
        constexpr Vector(const Vector<A, 2>& xy_, B z_) :
            x{static_cast<T>(xy_.x)},
            y{static_cast<T>(xy_.y)},
            z{static_cast<T>(z_)}
        {}
        template<typename A, ScalarOrBoolean B>
        requires std::constructible_from<T, A> and std::constructible_from<T, B>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B>))
        constexpr Vector(A x_, const Vector<B, 2>& yz_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(yz_.y)},
            z{static_cast<T>(yz_.z)}
        {}
        template<ScalarOrBoolean U>
        requires std::constructible_from<T, U>
        explicit constexpr Vector(const Vector<U, 4>& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)},
            z{static_cast<T>(v.z)}
        {}
        template<ScalarOrBoolean U>
        requires std::constructible_from<T, U>
        explicit (not (std::convertible_to<T, U>))
        constexpr Vector(const Vector<U, 3>& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)},
            z{static_cast<T>(v.z)}
        {}

        template<ScalarOrBoolean U>
        requires std::assignable_from<T, U>
        constexpr Vector& operator=(const Vector<U, 3>& v)
        {
            this->x = v.x;
            this->y = v.y;
            this->z = v.z;
            return *this;
        }

        constexpr size_type size() const { return 3; }
        constexpr pointer data() { return &x; }
        constexpr const_pointer data() const { return &x; }
        constexpr iterator begin() { return data(); }
        constexpr const_iterator begin() const { return data(); }
        constexpr iterator end() { return data() + size(); }
        constexpr const_iterator end() const { return data() + size(); }
        constexpr reference operator[](size_type pos) { return begin()[pos]; }
        constexpr const_reference operator[](size_type pos) const { return begin()[pos]; }

        friend constexpr bool operator==(const Vector&, const Vector&) = default;

        constexpr operator std::span<T, 3>() { return {data(), 3}; }
        constexpr operator std::span<const T, 3>() const { return {data(), 3}; }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator+=(U scalar)
        {
            this->x += scalar;
            this->y += scalar;
            this->z += scalar;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator+=(const Vector<U, 3>& rhs)
        {
            this->x += rhs.x;
            this->y += rhs.y;
            this->z += rhs.z;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator-=(U scalar)
        {
            this->x -= scalar;
            this->y -= scalar;
            this->z -= scalar;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator-=(const Vector<U, 3>& rhs)
        {
            this->x -= rhs.x;
            this->y -= rhs.y;
            this->z -= rhs.z;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator*=(U scalar)
        {
            this->x *= scalar;
            this->y *= scalar;
            this->z *= scalar;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator*=(const Vector<U, 3>& rhs)
        {
            this->x *= rhs.x;
            this->y *= rhs.y;
            this->z *= rhs.z;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator/=(U scalar)
        {
            this->x /= scalar;
            this->y /= scalar;
            this->z /= scalar;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator/=(const Vector<U, 3>& rhs)
        {
            this->x /= rhs.x;
            this->y /= rhs.y;
            this->z /= rhs.z;
            return *this;
        }

        constexpr Vector& operator++()
            requires std::incrementable<T>
        {
            ++this->x;
            ++this->y;
            ++this->z;
            return *this;
        }

        constexpr Vector& operator--()
            requires std::incrementable<T>
        {
            --this->x;
            --this->y;
            --this->z;
            return *this;
        }

        constexpr Vector operator++(int)
            requires std::incrementable<T>
        {
            Vector copy{*this};
            ++(*this);
            return copy;
        }

        constexpr Vector operator--(int)
            requires std::incrementable<T>
        {
            Vector copy{*this};
            --(*this);
            return copy;
        }

        template<typename U>
        requires std::constructible_from<T, U>
        constexpr Vector with_element(size_type pos, U value) const
        {
            Vector copy{*this};
            copy[pos] = static_cast<T>(value);
            return copy;
        }

        T x{};
        T y{};
        T z{};
    };

    template<Scalar T>
    constexpr Vector<T, 3> operator+(const Vector<T, 3>& vec)
    {
        return {+vec.x, +vec.y, +vec.z};
    }

    template<Scalar T>
    constexpr Vector<T, 3> operator-(const Vector<T, 3>& vec)
    {
        return {-vec.x, -vec.y, -vec.z};
    }


    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} + U{}), 3> operator+(const Vector<T, 3>& vec, U scalar)
    {
        return {vec.x + scalar, vec.y + scalar, vec.z + scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} + U{}), 3> operator+(T scalar, const Vector<U, 3>& vec)
    {
        return {scalar + vec.x, scalar + vec.y, scalar + vec.z};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} + U{}), 3> operator+(const Vector<T, 3>& lhs, const Vector<U, 3>& rhs)
    {
        return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
    }


    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} - U{}), 3> operator-(const Vector<T, 3>& vec, U scalar)
    {
        return {vec.x - scalar, vec.y - scalar, vec.z - scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} - U{}), 3> operator-(T scalar, const Vector<U, 3>& vec)
    {
        return {scalar - vec.x, scalar - vec.y, scalar - vec.z};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} - U{}), 3> operator-(const Vector<T, 3>& lhs, const Vector<U, 3>& rhs)
    {
        return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
    }


    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} * U{}), 3> operator*(const Vector<T, 3>& vec, U scalar)
    {
        return {vec.x * scalar, vec.y * scalar, vec.z * scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} * U{}), 3> operator*(T scalar, const Vector<U, 3>& vec)
    {
        return {scalar * vec.x, scalar * vec.y, scalar * vec.z};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} * U{}), 3> operator*(const Vector<T, 3>& lhs, const Vector<U, 3>& rhs)
    {
        return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} / U{}), 3> operator/(const Vector<T, 3>& vec, U scalar)
    {
        return {vec.x / scalar, vec.y / scalar, vec.z / scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} / U{}), 3> operator/(T scalar, const Vector<U, 3>& vec)
    {
        return {scalar / vec.x, scalar / vec.y, scalar / vec.z};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} / U{}), 3> operator/(const Vector<T, 3>& lhs, const Vector<U, 3>& rhs)
    {
        return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
    }

    constexpr Vector<bool, 3> operator&&(const Vector<bool, 3>& lhs, const Vector<bool, 3>& rhs)
    {
        return Vector<bool, 3>{lhs.x && rhs.x, lhs.y && rhs.y, lhs.z && rhs.z};
    }

    constexpr Vector<bool, 3> operator||(const Vector<bool, 3>& lhs, const Vector<bool, 3>& rhs)
    {
        return Vector<bool, 3>{lhs.x || rhs.x, lhs.y || rhs.y, lhs.z || rhs.z};
    }

    using Vector3  = Vector<float, 3>;
    using Vector3f = Vector<float, 3>;
    using Vector3d = Vector<double, 3>;
    using Vector3i = Vector<int, 3>;
    using Vector3z = Vector<ptrdiff_t, 3>;
    using Vector3uz = Vector<size_t, 3>;
    using Vector3u32 = Vector<uint32_t, 3>;
}
