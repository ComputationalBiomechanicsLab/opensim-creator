#pragma once

#include <liboscar/maths/Scalar.h>
#include <liboscar/maths/Vector.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

namespace osc
{
    template<ScalarOrBoolean T>
    struct Vector<T, 2> {
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
        constexpr Vector(const Vector<U, 2>& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)}
        {}
        template<ScalarOrBoolean U>
        requires std::constructible_from<T, U>
        explicit constexpr Vector(const Vector<U, 3>& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)}
        {}
        template<ScalarOrBoolean U>
        requires std::constructible_from<T, U>
        explicit constexpr Vector(const Vector<U, 4>& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)}
        {}

        template<ScalarOrBoolean U>
        requires std::assignable_from<T, U>
        constexpr Vector& operator=(const Vector<U, 2>& v)
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

        friend constexpr bool operator==(const Vector<T, 2>&, const Vector<T, 2>&) = default;

        constexpr operator std::span<T, 2>() { return {data(), 2}; }
        constexpr operator std::span<const T, 2>() const { return {data(), 2}; }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector<T, 2>& operator+=(U scalar)
        {
            this->x += scalar;
            this->y += scalar;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vector& operator+=(const Vector<U, 2>& rhs)
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
        constexpr Vector& operator-=(const Vector<U, 2>& rhs)
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
        constexpr Vector& operator*=(Vector<U, 2> const& rhs)
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
        constexpr Vector& operator/=(const Vector<U, 2>& rhs)
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
    constexpr Vector<T, 2> operator+(const Vector<T, 2>& vec)
    {
        return {+vec.x, +vec.y};
    }

    template<Scalar T>
    constexpr Vector<T, 2> operator-(const Vector<T, 2>& vec)
    {
        return {-vec.x, -vec.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} + U{}), 2> operator+(const Vector<T, 2>& vec, U scalar)
    {
        return {vec.x + scalar, vec.y + scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} + U{}), 2> operator+(T scalar, const Vector<U, 2>& vec)
    {
        return {scalar + vec.x, scalar + vec.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} + U{}), 2> operator+(const Vector<T, 2>& lhs, const Vector<U, 2>& rhs)
    {
        return {lhs.x + rhs.x, lhs.y + rhs.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} - U{}), 2> operator-(const Vector<T, 2>& vec, U scalar)
    {
        return {vec.x - scalar, vec.y - scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} - U{}), 2> operator-(T scalar, const Vector<U, 2>& vec)
    {
        return {scalar - vec.x, scalar - vec.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} - U{}), 2> operator-(const Vector<T, 2>& lhs, const Vector<U, 2>& rhs)
    {
        return {lhs.x - rhs.x, lhs.y - rhs.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} * U{}), 2> operator*(const Vector<T, 2>& vec, U scalar)
    {
        return {vec.x * scalar, vec.y * scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} * U{}), 2> operator*(T scalar, const Vector<U, 2>& vec)
    {
        return {scalar * vec.x, scalar * vec.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} * U{}), 2> operator*(const Vector<T, 2>& lhs, const Vector<U, 2>& rhs)
    {
        return {lhs.x * rhs.x, lhs.y * rhs.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} / U{}), 2> operator/(const Vector<T, 2>& vec, U scalar)
    {
        return {vec.x / scalar, vec.y / scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} / U{}), 2> operator/(T scalar, const Vector<U, 2>& vec)
    {
        return {scalar / vec.x, scalar / vec.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vector<decltype(T{} / U{}), 2> operator/(const Vector<T, 2>& lhs, const Vector<U, 2>& rhs)
    {
        return {lhs.x / rhs.x, lhs.y / rhs.y};
    }

    constexpr Vector<bool, 2> operator&&(const Vector<bool, 2>& lhs, const Vector<bool, 2>& rhs)
    {
        return {lhs.x && rhs.x, lhs.y && rhs.y};
    }

    constexpr Vector<bool, 2> operator||(const Vector<bool, 2>& lhs, const Vector<bool, 2>& rhs)
    {
        return {lhs.x || rhs.x, lhs.y || rhs.y};
    }

    using Vector2 = Vector<float, 2>;
    using Vector2f = Vector<float, 2>;
    using Vector2d = Vector<double, 2>;
    using Vector2i = Vector<int, 2>;
    using Vector2z = Vector<ptrdiff_t, 2>;
    using Vector2uz = Vector<size_t, 2>;
    using Vector2u32 = Vector<uint32_t, 2>;
}
