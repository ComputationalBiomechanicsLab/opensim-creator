#pragma once

#include <oscar/Maths/Scalar.h>
#include <oscar/Maths/Vec.h>

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace osc
{
    template<ScalarOrBoolean T>
    struct Vec<3, T> {
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

        constexpr Vec() = default;
        explicit constexpr Vec(T value) :
            x{value},
            y{value},
            z{value}
        {}
        constexpr Vec(T x_, T y_, T z_) :
            x{x_},
            y{y_},
            z{z_}
        {}
        template<typename X, typename Y, typename Z>
        requires std::constructible_from<T, X> and std::constructible_from<T, Y> and std::constructible_from<T, Z>
        explicit (not (std::convertible_to<T, X> and std::convertible_to<T, Y> and std::convertible_to<T, Z>))
        constexpr Vec(X x_, Y y_, Z z_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(y_)},
            z{static_cast<T>(z_)}
        {}
        template<ScalarOrBoolean A, typename B>
        requires std::constructible_from<T, A> and std::constructible_from<T, B>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B>))
        constexpr Vec(const Vec<2, A>& xy_, B z_) :
            x{static_cast<T>(xy_.x)},
            y{static_cast<T>(xy_.y)},
            z{static_cast<T>(z_)}
        {}
        template<typename A, ScalarOrBoolean B>
        requires std::constructible_from<T, A> and std::constructible_from<T, B>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B>))
        constexpr Vec(A x_, const Vec<2, B>& yz_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(yz_.y)},
            z{static_cast<T>(yz_.z)}
        {}
        template<ScalarOrBoolean U>
        requires std::constructible_from<T, U>
        explicit constexpr Vec(const Vec<4, U>& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)},
            z{static_cast<T>(v.z)}
        {}
        template<ScalarOrBoolean U>
        requires std::constructible_from<T, U>
        explicit (not (std::convertible_to<T, U>))
        constexpr Vec(const Vec<3, U>& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)},
            z{static_cast<T>(v.z)}
        {}

        template<ScalarOrBoolean U>
        requires std::assignable_from<T, U>
        constexpr Vec& operator=(const Vec<3, U>& v)
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

        friend constexpr bool operator==(const Vec&, const Vec&) = default;

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator+=(U scalar)
        {
            this->x += scalar;
            this->y += scalar;
            this->z += scalar;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator+=(const Vec<3, U>& rhs)
        {
            this->x += rhs.x;
            this->y += rhs.y;
            this->z += rhs.z;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator-=(U scalar)
        {
            this->x -= scalar;
            this->y -= scalar;
            this->z -= scalar;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator-=(const Vec<3, U>& rhs)
        {
            this->x -= rhs.x;
            this->y -= rhs.y;
            this->z -= rhs.z;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator*=(U scalar)
        {
            this->x *= scalar;
            this->y *= scalar;
            this->z *= scalar;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator*=(const Vec<3, U>& rhs)
        {
            this->x *= rhs.x;
            this->y *= rhs.y;
            this->z *= rhs.z;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator/=(U scalar)
        {
            this->x /= scalar;
            this->y /= scalar;
            this->z /= scalar;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator/=(const Vec<3, U>& rhs)
        {
            this->x /= rhs.x;
            this->y /= rhs.y;
            this->z /= rhs.z;
            return *this;
        }

        constexpr Vec& operator++()
            requires std::incrementable<T>
        {
            ++this->x;
            ++this->y;
            ++this->z;
            return *this;
        }

        constexpr Vec& operator--()
            requires std::incrementable<T>
        {
            --this->x;
            --this->y;
            --this->z;
            return *this;
        }

        constexpr Vec operator++(int)
            requires std::incrementable<T>
        {
            Vec copy{*this};
            ++(*this);
            return copy;
        }

        constexpr Vec operator--(int)
            requires std::incrementable<T>
        {
            Vec copy{*this};
            --(*this);
            return copy;
        }

        template<typename U>
        requires std::constructible_from<T, U>
        constexpr Vec with_element(size_type pos, U value) const
        {
            Vec copy{*this};
            copy[pos] = static_cast<T>(value);
            return copy;
        }

        T x{};
        T y{};
        T z{};
    };

    template<Scalar T>
    constexpr Vec<3, T> operator+(const Vec<3, T>& vec)
    {
        return {+vec.x, +vec.y, +vec.z};
    }

    template<Scalar T>
    constexpr Vec<3, T> operator-(const Vec<3, T>& vec)
    {
        return {-vec.x, -vec.y, -vec.z};
    }


    template<Scalar T, Scalar U>
    constexpr Vec<3, decltype(T{} + U{})> operator+(const Vec<3, T>& vec, U scalar)
    {
        return {vec.x + scalar, vec.y + scalar, vec.z + scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<3, decltype(T{} + U{})> operator+(T scalar, const Vec<3, U>& vec)
    {
        return {scalar + vec.x, scalar + vec.y, scalar + vec.z};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<3, decltype(T{} + U{})> operator+(const Vec<3, T>& lhs, const Vec<3, U>& rhs)
    {
        return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
    }


    template<Scalar T, Scalar U>
    constexpr Vec<3, decltype(T{} - U{})> operator-(const Vec<3, T>& vec, U scalar)
    {
        return {vec.x - scalar, vec.y - scalar, vec.z - scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<3, decltype(T{} - U{})> operator-(T scalar, const Vec<3, U>& vec)
    {
        return {scalar - vec.x, scalar - vec.y, scalar - vec.z};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<3, decltype(T{} - U{})> operator-(const Vec<3, T>& lhs, const Vec<3, U>& rhs)
    {
        return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
    }


    template<Scalar T, Scalar U>
    constexpr Vec<3, decltype(T{} * U{})> operator*(const Vec<3, T>& vec, U scalar)
    {
        return {vec.x * scalar, vec.y * scalar, vec.z * scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<3, decltype(T{} * U{})> operator*(T scalar, const Vec<3, U>& vec)
    {
        return {scalar * vec.x, scalar * vec.y, scalar * vec.z};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<3, decltype(T{} * U{})> operator*(const Vec<3, T>& lhs, const Vec<3, U>& rhs)
    {
        return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<3, decltype(T{} / U{})> operator/(const Vec<3, T>& vec, U scalar)
    {
        return {vec.x / scalar, vec.y / scalar, vec.z / scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<3, decltype(T{} / U{})> operator/(T scalar, const Vec<3, U>& vec)
    {
        return {scalar / vec.x, scalar / vec.y, scalar / vec.z};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<3, decltype(T{} / U{})> operator/(const Vec<3, T>& lhs, const Vec<3, U>& rhs)
    {
        return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
    }

    constexpr Vec<3, bool> operator&&(const Vec<3, bool>& lhs, const Vec<3, bool>& rhs)
    {
        return Vec<3, bool>{lhs.x && rhs.x, lhs.y && rhs.y, lhs.z && rhs.z};
    }

    constexpr Vec<3, bool> operator||(const Vec<3, bool>& lhs, const Vec<3, bool>& rhs)
    {
        return Vec<3, bool>{lhs.x || rhs.x, lhs.y || rhs.y, lhs.z || rhs.z};
    }

    using Vec3  = Vec<3, float>;
    using Vec3f = Vec<3, float>;
    using Vec3d = Vec<3, double>;
    using Vec3i = Vec<3, int>;
    using Vec3z = Vec<3, ptrdiff_t>;
    using Vec3uz = Vec<3, size_t>;
    using Vec3u32 = Vec<3, uint32_t>;
}
