#pragma once

#include <liboscar/maths/scalar.h>
#include <liboscar/maths/vector.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

namespace osc
{
    template<ScalarOrBoolean T>
    struct Vector<T, 4> {
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
            z{value},
            w{value}
        {}
        constexpr Vector(T x_, T y_, T z_, T w_) :
            x{x_},
            y{y_},
            z{z_},
            w{w_}
        {}
        template<typename A, typename B, typename C, typename D>
        requires std::constructible_from<T, A> and std::constructible_from<T, B> and std::constructible_from<T, C> and std::constructible_from<T, D>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B> and std::convertible_to<T, C> and std::convertible_to<T, D>))
        constexpr Vector(A x_, B y_, C z_, D w_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(y_)},
            z{static_cast<T>(z_)},
            w{static_cast<T>(w_)}
        {}
        template<ScalarOrBoolean A, typename B, typename C>
        requires std::constructible_from<T, A> and std::constructible_from<T, B> and std::constructible_from<T, C>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B> and std::convertible_to<T, C>))
        constexpr Vector(const Vector<A, 2>& xy_, B z_, C w_) :
            x{static_cast<T>(xy_.x)},
            y{static_cast<T>(xy_.y)},
            z{static_cast<T>(z_)},
            w{static_cast<T>(w_)}
        {}
        template<typename A, ScalarOrBoolean B, typename C>
        requires std::constructible_from<T, A> and std::constructible_from<T, B> and std::constructible_from<T, C>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B> and std::convertible_to<T, C>))
        constexpr Vector(A x_, const Vector<B, 2>& yz_, C w_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(yz_.x)},
            z{static_cast<T>(yz_.y)},
            w{static_cast<T>(w_)}
        {}
        template<typename A, typename B, ScalarOrBoolean C>
        requires std::constructible_from<T, A> and std::constructible_from<T, B> and std::constructible_from<T, C>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B> and std::convertible_to<T, C>))
        constexpr Vector(A x_, B y_, const Vector<C, 2>& zw_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(y_)},
            z{static_cast<T>(zw_.x)},
            w{static_cast<T>(zw_.y)}
        {}
        template<ScalarOrBoolean A, typename B>
        requires std::constructible_from<T, A> and std::constructible_from<T, B>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B>))
        constexpr Vector(const Vector<A, 3>& xyz_, B w_) :
            x{static_cast<T>(xyz_.x)},
            y{static_cast<T>(xyz_.y)},
            z{static_cast<T>(xyz_.z)},
            w{static_cast<T>(w_)}
        {}
        template<typename A, ScalarOrBoolean B>
        requires std::constructible_from<T, A> and std::constructible_from<T, B>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B>))
        constexpr Vector(A x_, const Vector<B, 3>& yzw_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(yzw_.x)},
            z{static_cast<T>(yzw_.y)},
            w{static_cast<T>(yzw_.z)}
        {}
        template<ScalarOrBoolean A, ScalarOrBoolean B>
        requires std::constructible_from<T, A> and std::constructible_from<T, B>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B>))
        constexpr Vector(const Vector<A, 2>& xy_, const Vector<B, 2>& wz_) :
            x{static_cast<T>(xy_.x)},
            y{static_cast<T>(xy_.y)},
            z{static_cast<T>(wz_.x)},
            w{static_cast<T>(wz_.w)}
        {}
        template<ScalarOrBoolean A>
        requires std::constructible_from<T, A>
        explicit constexpr Vector(const Vector<A, 4>& v) :
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

        constexpr friend bool operator==(const Vector&, const Vector&) = default;

        constexpr operator std::span<T, 4>() { return {data(), 4}; }
        constexpr operator std::span<const T, 4>() const { return {data(), 4}; }

        template<ScalarOrBoolean U>
        requires std::assignable_from<T, U>
        constexpr Vector& operator=(const Vector<U, 4>& v)
        {
            this->x = v.x;
            this->y = v.y;
            this->z = v.z;
            this->w = v.w;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator+=(U scalar)
        {
            this->x += static_cast<T>(scalar);
            this->y += static_cast<T>(scalar);
            this->z += static_cast<T>(scalar);
            this->w += static_cast<T>(scalar);
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator+=(const Vector<U, 4>& rhs)
        {
            this->x += static_cast<T>(rhs.x);
            this->y += static_cast<T>(rhs.y);
            this->z += static_cast<T>(rhs.z);
            this->w += static_cast<T>(rhs.w);
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator-=(U scalar)
        {
            this->x -= static_cast<T>(scalar);
            this->y -= static_cast<T>(scalar);
            this->z -= static_cast<T>(scalar);
            this->w -= static_cast<T>(scalar);
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator-=(const Vector<U, 4>& rhs)
        {
            this->x -= static_cast<T>(rhs.x);
            this->y -= static_cast<T>(rhs.y);
            this->z -= static_cast<T>(rhs.z);
            this->w -= static_cast<T>(rhs.w);
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator*=(U scalar)
        {
            this->x *= static_cast<T>(scalar);
            this->y *= static_cast<T>(scalar);
            this->z *= static_cast<T>(scalar);
            this->w *= static_cast<T>(scalar);
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator*=(const Vector<U, 4>& rhs)
        {
            this->x *= static_cast<T>(rhs.x);
            this->y *= static_cast<T>(rhs.y);
            this->z *= static_cast<T>(rhs.z);
            this->w *= static_cast<T>(rhs.w);
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator/=(U scalar)
        {
            this->x /= static_cast<T>(scalar);
            this->y /= static_cast<T>(scalar);
            this->z /= static_cast<T>(scalar);
            this->w /= static_cast<T>(scalar);
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator/=(const Vector<U, 4>& rhs)
        {
            this->x /= static_cast<T>(rhs.x);
            this->y /= static_cast<T>(rhs.y);
            this->z /= static_cast<T>(rhs.z);
            this->w /= static_cast<T>(rhs.w);
            return *this;
        }

        constexpr Vector& operator++()
            requires std::incrementable<T>
        {
            ++this->x;
            ++this->y;
            ++this->z;
            ++this->w;
            return *this;
        }

        constexpr Vector& operator--()
            requires std::incrementable<T>
        {
            --this->x;
            --this->y;
            --this->z;
            --this->w;
            return *this;
        }

        constexpr Vector operator++(int)
            requires std::incrementable<T>
        {
            Vector copy{*this};
            ++copy;
            return copy;
        }

        constexpr Vector operator--(int)
            requires std::incrementable<T>
        {
            Vector copy{*this};
            --copy;
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
        T w{};
    };

    template<Scalar T>
    constexpr Vector<T, 4> operator+(const Vector<T, 4>& vec)
    {
        return {+vec.x, +vec.y, +vec.z, +vec.w};
    }

    template<Scalar T>
    constexpr Vector<T, 4> operator-(const Vector<T, 4>& vec)
    {
        return {-vec.x, -vec.y, -vec.z, -vec.w};
    }


    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} + U{}), 4> operator+(const Vector<T, 4>& vec, U scalar)
    {
        return {vec.x + scalar, vec.y + scalar, vec.z + scalar, vec.w + scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} + U{}), 4> operator+(T scalar, const Vector<U, 4>& vec)
    {
        return {scalar + vec.x, scalar + vec.y, scalar + vec.z, scalar + vec.w};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} + U{}), 4> operator+(const Vector<T, 4>& lhs, const Vector<U, 4>& rhs)
    {
        return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} - U{}), 4> operator-(const Vector<T, 4>& vec, U scalar)
    {
        return {vec.x - scalar, vec.y - scalar, vec.z - scalar, vec.w - scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} - U{}), 4> operator-(T scalar, const Vector<U, 4>& vec)
    {
        return {scalar - vec.x, scalar - vec.y, scalar - vec.z, scalar - vec.w};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} - U{}), 4> operator-(const Vector<T, 4>& lhs, const Vector<U, 4>& rhs)
    {
        return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} * U{}), 4> operator*(const Vector<T, 4>& vec, U scalar)
    {
        return {vec.x * scalar, vec.y * scalar, vec.z * scalar, vec.w * scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} * U{}), 4> operator*(T scalar, const Vector<U, 4>& vec)
    {
        return {scalar * vec.x, scalar * vec.y, scalar * vec.z, scalar * vec.w};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} * U{}), 4> operator*(const Vector<T, 4>& lhs, const Vector<U, 4>& rhs)
    {
        return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} / U{}), 4> operator/(const Vector<T, 4>& vec, U scalar)
    {
        return {vec.x / scalar, vec.y / scalar, vec.z / scalar, vec.w / scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} / U{}), 4> operator/(T scalar, const Vector<U, 4>& vec)
    {
        return {scalar / vec.x, scalar / vec.y, scalar / vec.z, scalar / vec.w};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} / U{}), 4> operator/(const Vector<T, 4>& lhs, const Vector<U, 4>& rhs)
    {
        return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w};
    }

    constexpr Vector<bool, 4> operator&&(const Vector<bool, 4>& lhs, const Vector<bool, 4>& rhs)
    {
        return Vector<bool, 4>{lhs.x && rhs.x, lhs.y && rhs.y, lhs.z && rhs.z, lhs.w && rhs.w};
    }

    constexpr Vector<bool, 4> operator||(const Vector<bool, 4>& lhs, const Vector<bool, 4>& rhs)
    {
        return Vector<bool, 4>{lhs.x || rhs.x, lhs.y || rhs.y, lhs.z || rhs.z, lhs.w || rhs.w};
    }

    using Vector4  = Vector<float, 4>;
    using Vector4f = Vector<float, 4>;
    using Vector4d = Vector<double, 4>;
    using Vector4i = Vector<int, 4>;
    using Vector4z = Vector<ptrdiff_t, 4>;
    using Vector4uz = Vector<size_t, 4>;
    using Vector4u32 = Vector<uint32_t, 4>;
}
