#pragma once

#include <oscar/Maths/Scalar.h>
#include <oscar/Maths/Vec.h>

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace osc
{
    template<ScalarOrBoolean T>
    struct Vec<4, T> {
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
        constexpr explicit Vec(T value) :
            x{value},
            y{value},
            z{value},
            w{value}
        {}
        constexpr Vec(T x_, T y_, T z_, T w_) :
            x{x_},
            y{y_},
            z{z_},
            w{w_}
        {}
        template<typename A, typename B, typename C, typename D>
        requires std::constructible_from<T, A> and std::constructible_from<T, B> and std::constructible_from<T, C> and std::constructible_from<T, D>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B> and std::convertible_to<T, C> and std::convertible_to<T, D>))
        constexpr Vec(A x_, B y_, C z_, D w_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(y_)},
            z{static_cast<T>(z_)},
            w{static_cast<T>(w_)}
        {}
        template<ScalarOrBoolean A, typename B, typename C>
        requires std::constructible_from<T, A> and std::constructible_from<T, B> and std::constructible_from<T, C>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B> and std::convertible_to<T, C>))
        constexpr Vec(const Vec<2, A>& xy_, B z_, C w_) :
            x{static_cast<T>(xy_.x)},
            y{static_cast<T>(xy_.y)},
            z{static_cast<T>(z_)},
            w{static_cast<T>(w_)}
        {}
        template<typename A, ScalarOrBoolean B, typename C>
        requires std::constructible_from<T, A> and std::constructible_from<T, B> and std::constructible_from<T, C>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B> and std::convertible_to<T, C>))
        constexpr Vec(A x_, const Vec<2, B>& yz_, C w_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(yz_.x)},
            z{static_cast<T>(yz_.y)},
            w{static_cast<T>(w_)}
        {}
        template<typename A, typename B, ScalarOrBoolean C>
        requires std::constructible_from<T, A> and std::constructible_from<T, B> and std::constructible_from<T, C>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B> and std::convertible_to<T, C>))
        constexpr Vec(A x_, B y_, const Vec<2, C>& zw_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(y_)},
            z{static_cast<T>(zw_.x)},
            w{static_cast<T>(zw_.y)}
        {}
        template<ScalarOrBoolean A, typename B>
        requires std::constructible_from<T, A> and std::constructible_from<T, B>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B>))
        constexpr Vec(const Vec<3, A>& xyz_, B w_) :
            x{static_cast<T>(xyz_.x)},
            y{static_cast<T>(xyz_.y)},
            z{static_cast<T>(xyz_.z)},
            w{static_cast<T>(w_)}
        {}
        template<typename A, ScalarOrBoolean B>
        requires std::constructible_from<T, A> and std::constructible_from<T, B>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B>))
        constexpr Vec(A x_, const Vec<3, B>& yzw_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(yzw_.x)},
            z{static_cast<T>(yzw_.y)},
            w{static_cast<T>(yzw_.z)}
        {}
        template<ScalarOrBoolean A, ScalarOrBoolean B>
        requires std::constructible_from<T, A> and std::constructible_from<T, B>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B>))
        constexpr Vec(const Vec<2, A>& xy_, const Vec<2, B>& wz_) :
            x{static_cast<T>(xy_.x)},
            y{static_cast<T>(xy_.y)},
            z{static_cast<T>(wz_.x)},
            w{static_cast<T>(wz_.w)}
        {}
        template<ScalarOrBoolean A>
        requires std::constructible_from<T, A>
        explicit constexpr Vec(const Vec<4, A>& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)},
            z{static_cast<T>(v.z)},
            w{static_cast<T>(v.w)}
        {}

        constexpr size_type size() const { return 4; }
        constexpr pointer data() { return &x; }
        constexpr const_pointer data() const { return &x; }
        constexpr iterator begin() { return data(); }
        constexpr const_iterator begin() const { return data(); }
        constexpr iterator end() { return data() + size(); }
        constexpr const_iterator end() const { return data() + size(); }
        constexpr reference operator[](size_type pos) { return begin()[pos]; }
        constexpr const_reference operator[](size_type pos) const { return begin()[pos]; }

        constexpr friend bool operator==(const Vec&, const Vec&) = default;

        template<ScalarOrBoolean U>
        requires std::assignable_from<T, U>
        constexpr Vec& operator=(const Vec<4, U>& v)
        {
            this->x = v.x;
            this->y = v.y;
            this->z = v.z;
            this->w = v.w;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator+=(U scalar)
        {
            this->x += static_cast<T>(scalar);
            this->y += static_cast<T>(scalar);
            this->z += static_cast<T>(scalar);
            this->w += static_cast<T>(scalar);
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator+=(const Vec<4, U>& rhs)
        {
            this->x += static_cast<T>(rhs.x);
            this->y += static_cast<T>(rhs.y);
            this->z += static_cast<T>(rhs.z);
            this->w += static_cast<T>(rhs.w);
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator-=(U scalar)
        {
            this->x -= static_cast<T>(scalar);
            this->y -= static_cast<T>(scalar);
            this->z -= static_cast<T>(scalar);
            this->w -= static_cast<T>(scalar);
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator-=(const Vec<4, U>& rhs)
        {
            this->x -= static_cast<T>(rhs.x);
            this->y -= static_cast<T>(rhs.y);
            this->z -= static_cast<T>(rhs.z);
            this->w -= static_cast<T>(rhs.w);
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator*=(U scalar)
        {
            this->x *= static_cast<T>(scalar);
            this->y *= static_cast<T>(scalar);
            this->z *= static_cast<T>(scalar);
            this->w *= static_cast<T>(scalar);
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator*=(const Vec<4, U>& rhs)
        {
            this->x *= static_cast<T>(rhs.x);
            this->y *= static_cast<T>(rhs.y);
            this->z *= static_cast<T>(rhs.z);
            this->w *= static_cast<T>(rhs.w);
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator/=(U scalar)
        {
            this->x /= static_cast<T>(scalar);
            this->y /= static_cast<T>(scalar);
            this->z /= static_cast<T>(scalar);
            this->w /= static_cast<T>(scalar);
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator/=(const Vec<4, U>& rhs)
        {
            this->x /= static_cast<T>(rhs.x);
            this->y /= static_cast<T>(rhs.y);
            this->z /= static_cast<T>(rhs.z);
            this->w /= static_cast<T>(rhs.w);
            return *this;
        }

        constexpr Vec& operator++()
            requires std::incrementable<T>
        {
            ++this->x;
            ++this->y;
            ++this->z;
            ++this->w;
            return *this;
        }

        constexpr Vec& operator--()
            requires std::incrementable<T>
        {
            --this->x;
            --this->y;
            --this->z;
            --this->w;
            return *this;
        }

        constexpr Vec operator++(int)
            requires std::incrementable<T>
        {
            Vec copy{*this};
            ++copy;
            return copy;
        }

        constexpr Vec operator--(int)
            requires std::incrementable<T>
        {
            Vec copy{*this};
            --copy;
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
        T w{};
    };

    template<Scalar T>
    constexpr Vec<4, T> operator+(const Vec<4, T>& vec)
    {
        return {+vec.x, +vec.y, +vec.z, +vec.w};
    }

    template<Scalar T>
    constexpr Vec<4, T> operator-(const Vec<4, T>& vec)
    {
        return {-vec.x, -vec.y, -vec.z, -vec.w};
    }


    template<Scalar T, Scalar U>
    constexpr Vec<4, decltype(T{} + U{})> operator+(const Vec<4, T>& vec, U scalar)
    {
        return {vec.x + scalar, vec.y + scalar, vec.z + scalar, vec.w + scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<4, decltype(T{} + U{})> operator+(T scalar, const Vec<4, U>& vec)
    {
        return {scalar + vec.x, scalar + vec.y, scalar + vec.z, scalar + vec.w};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<4, decltype(T{} + U{})> operator+(const Vec<4, T>& lhs, const Vec<4, U>& rhs)
    {
        return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<4, decltype(T{} - U{})> operator-(const Vec<4, T>& vec, U scalar)
    {
        return {vec.x - scalar, vec.y - scalar, vec.z - scalar, vec.w - scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<4, decltype(T{} - U{})> operator-(T scalar, const Vec<4, U>& vec)
    {
        return {scalar - vec.x, scalar - vec.y, scalar - vec.z, scalar - vec.w};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<4, decltype(T{} - U{})> operator-(const Vec<4, T>& lhs, const Vec<4, U>& rhs)
    {
        return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<4, decltype(T{} * U{})> operator*(const Vec<4, T>& vec, U scalar)
    {
        return {vec.x * scalar, vec.y * scalar, vec.z * scalar, vec.w * scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<4, decltype(T{} * U{})> operator*(T scalar, const Vec<4, U>& vec)
    {
        return {scalar * vec.x, scalar * vec.y, scalar * vec.z, scalar * vec.w};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<4, decltype(T{} * U{})> operator*(const Vec<4, T>& lhs, const Vec<4, U>& rhs)
    {
        return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<4, decltype(T{} / U{})> operator/(const Vec<4, T>& vec, U scalar)
    {
        return {vec.x / scalar, vec.y / scalar, vec.z / scalar, vec.w / scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<4, decltype(T{} / U{})> operator/(T scalar, const Vec<4, U>& vec)
    {
        return {scalar / vec.x, scalar / vec.y, scalar / vec.z, scalar / vec.w};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<4, decltype(T{} / U{})> operator/(const Vec<4, T>& lhs, const Vec<4, U>& rhs)
    {
        return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w};
    }

    constexpr Vec<4, bool> operator&&(const Vec<4, bool>& lhs, const Vec<4, bool>& rhs)
    {
        return Vec<4, bool>{lhs.x && rhs.x, lhs.y && rhs.y, lhs.z && rhs.z, lhs.w && rhs.w};
    }

    constexpr Vec<4, bool> operator||(const Vec<4, bool>& lhs, const Vec<4, bool>& rhs)
    {
        return Vec<4, bool>{lhs.x || rhs.x, lhs.y || rhs.y, lhs.z || rhs.z, lhs.w || rhs.w};
    }

    using Vec4  = Vec<4, float>;
    using Vec4f = Vec<4, float>;
    using Vec4d = Vec<4, double>;
    using Vec4i = Vec<4, int>;
    using Vec4z = Vec<4, ptrdiff_t>;
    using Vec4uz = Vec<4, size_t>;
    using Vec4u32 = Vec<4, uint32_t>;
}
