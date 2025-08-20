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
    struct Vector<2, T> {
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
            y{value}
        {}
        constexpr Vector(T x_, T y_) :
            x{x_},
            y{y_}
        {}
        template<typename A, typename B>
        requires std::constructible_from<T, A> and std::constructible_from<T, B>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B>))
        constexpr Vector(A x_, B y_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(y_)}
        {}
        template<ScalarOrBoolean U>
        requires std::constructible_from<T, U>
        explicit (not std::convertible_to<T, U>)
        constexpr Vector(const Vector<2, U>& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)}
        {}
        template<ScalarOrBoolean U>
        requires std::constructible_from<T, U>
        explicit constexpr Vector(const Vector<3, U>& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)}
        {}
        template<ScalarOrBoolean U>
        requires std::constructible_from<T, U>
        explicit constexpr Vector(const Vector<4, U>& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)}
        {}

        template<ScalarOrBoolean U>
        requires std::assignable_from<T, U>
        constexpr Vector& operator=(const Vector<2, U>& v)
        {
            this->x = v.x;
            this->y = v.y;
            return *this;
        }

        constexpr size_type size() const { return 2; }
        constexpr pointer data() { return &x; }
        constexpr const_pointer data() const { return &x; }
        constexpr iterator begin() { return data(); }
        constexpr const_iterator begin() const { return data(); }
        constexpr iterator end() { return data() + size(); }
        constexpr const_iterator end() const { return data() + size(); }
        constexpr reference operator[](size_type pos) { return begin()[pos]; }
        constexpr const_reference operator[](size_type pos) const { return begin()[pos]; }

        friend constexpr bool operator==(const Vector<2, T>&, const Vector<2, T>&) = default;

        constexpr operator std::span<T, 2>() { return {data(), 2}; }
        constexpr operator std::span<const T, 2>() const { return {data(), 2}; }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector<2, T>& operator+=(U scalar)
        {
            this->x += scalar;
            this->y += scalar;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator+=(const Vector<2, U>& rhs)
        {
            this->x += rhs.x;
            this->y += rhs.y;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator-=(U scalar)
        {
            this->x -= scalar;
            this->y -= scalar;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator-=(const Vector<2, U>& rhs)
        {
            this->x -= rhs.x;
            this->y -= rhs.y;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator*=(U scalar)
        {
            this->x *= scalar;
            this->y *= scalar;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator*=(Vector<2, U> const& rhs)
        {
            this->x *= rhs.x;
            this->y *= rhs.y;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator/=(U scalar)
        {
            this->x /= scalar;
            this->y /= scalar;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator/=(const Vector<2, U>& rhs)
        {
            this->x /= rhs.x;
            this->y /= rhs.y;
            return *this;
        }

        constexpr Vector& operator++()
            requires std::incrementable<T>
        {
            ++this->x;
            ++this->y;
            return *this;
        }

        constexpr Vector& operator--()
            requires std::incrementable<T>
        {
            --this->x;
            --this->y;
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
    };

    template<Scalar T>
    constexpr Vector<2, T> operator+(const Vector<2, T>& vec)
    {
        return {+vec.x, +vec.y};
    }

    template<Scalar T>
    constexpr Vector<2, T> operator-(const Vector<2, T>& vec)
    {
        return {-vec.x, -vec.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<2, decltype(T{} + U{})> operator+(const Vector<2, T>& vec, U scalar)
    {
        return {vec.x + scalar, vec.y + scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<2, decltype(T{} + U{})> operator+(T scalar, const Vector<2, U>& vec)
    {
        return {scalar + vec.x, scalar + vec.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<2, decltype(T{} + U{})> operator+(const Vector<2, T>& lhs, const Vector<2, U>& rhs)
    {
        return {lhs.x + rhs.x, lhs.y + rhs.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<2, decltype(T{} - U{})> operator-(const Vector<2, T>& vec, U scalar)
    {
        return {vec.x - scalar, vec.y - scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<2, decltype(T{} - U{})> operator-(T scalar, const Vector<2, U>& vec)
    {
        return {scalar - vec.x, scalar - vec.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<2, decltype(T{} - U{})> operator-(const Vector<2, T>& lhs, const Vector<2, U>& rhs)
    {
        return {lhs.x - rhs.x, lhs.y - rhs.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<2, decltype(T{} * U{})> operator*(const Vector<2, T>& vec, U scalar)
    {
        return {vec.x * scalar, vec.y * scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<2, decltype(T{} * U{})> operator*(T scalar, const Vector<2, U>& vec)
    {
        return {scalar * vec.x, scalar * vec.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<2, decltype(T{} * U{})> operator*(const Vector<2, T>& lhs, const Vector<2, U>& rhs)
    {
        return {lhs.x * rhs.x, lhs.y * rhs.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<2, decltype(T{} / U{})> operator/(const Vector<2, T>& vec, U scalar)
    {
        return {vec.x / scalar, vec.y / scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<2, decltype(T{} / U{})> operator/(T scalar, const Vector<2, U>& vec)
    {
        return {scalar / vec.x, scalar / vec.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<2, decltype(T{} / U{})> operator/(const Vector<2, T>& lhs, const Vector<2, U>& rhs)
    {
        return {lhs.x / rhs.x, lhs.y / rhs.y};
    }

    constexpr Vector<2, bool> operator&&(const Vector<2, bool>& lhs, const Vector<2, bool>& rhs)
    {
        return {lhs.x && rhs.x, lhs.y && rhs.y};
    }

    constexpr Vector<2, bool> operator||(const Vector<2, bool>& lhs, const Vector<2, bool>& rhs)
    {
        return {lhs.x || rhs.x, lhs.y || rhs.y};
    }

    using Vector2 = Vector<2, float>;
    using Vector2f = Vector<2, float>;
    using Vector2d = Vector<2, double>;
    using Vector2i = Vector<2, int>;
    using Vector2z = Vector<2, ptrdiff_t>;
    using Vector2uz = Vector<2, size_t>;
    using Vector2u32 = Vector<2, uint32_t>;
}
