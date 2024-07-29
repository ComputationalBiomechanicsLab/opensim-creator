#pragma once

#include <oscar/Maths/Scalar.h>
#include <oscar/Maths/Vec.h>

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace osc
{
    template<ScalarOrBoolean T>
    struct Vec<2, T> {
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
            y{value}
        {}
        constexpr Vec(T x_, T y_) :
            x{x_},
            y{y_}
        {}
        template<typename A, typename B>
        requires std::constructible_from<T, A> and std::constructible_from<T, B>
        explicit (not (std::convertible_to<T, A> and std::convertible_to<T, B>))
        constexpr Vec(A x_, B y_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(y_)}
        {}
        template<ScalarOrBoolean U>
        requires std::constructible_from<T, U>
        explicit (not std::convertible_to<T, U>)
        constexpr Vec(const Vec<2, U>& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)}
        {}
        template<ScalarOrBoolean U>
        requires std::constructible_from<T, U>
        explicit constexpr Vec(const Vec<3, U>& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)}
        {}
        template<ScalarOrBoolean U>
        requires std::constructible_from<T, U>
        explicit constexpr Vec(const Vec<4, U>& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)}
        {}

        template<ScalarOrBoolean U>
        requires std::assignable_from<T, U>
        constexpr Vec& operator=(const Vec<2, U>& v)
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

        friend constexpr bool operator==(const Vec<2, T>&, const Vec<2, T>&) = default;

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec<2, T>& operator+=(U scalar)
        {
            this->x += scalar;
            this->y += scalar;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator+=(const Vec<2, U>& rhs)
        {
            this->x += rhs.x;
            this->y += rhs.y;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator-=(U scalar)
        {
            this->x -= scalar;
            this->y -= scalar;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator-=(const Vec<2, U>& rhs)
        {
            this->x -= rhs.x;
            this->y -= rhs.y;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator*=(U scalar)
        {
            this->x *= scalar;
            this->y *= scalar;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator*=(Vec<2, U> const& rhs)
        {
            this->x *= rhs.x;
            this->y *= rhs.y;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator/=(U scalar)
        {
            this->x /= scalar;
            this->y /= scalar;
            return *this;
        }

        template<Scalar U>
        requires (not std::same_as<T, bool>)
        constexpr Vec& operator/=(const Vec<2, U>& rhs)
        {
            this->x /= rhs.x;
            this->y /= rhs.y;
            return *this;
        }

        constexpr Vec& operator++()
            requires std::incrementable<T>
        {
            ++this->x;
            ++this->y;
            return *this;
        }

        constexpr Vec& operator--()
            requires std::incrementable<T>
        {
            --this->x;
            --this->y;
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
    };

    template<Scalar T>
    constexpr Vec<2, T> operator+(const Vec<2, T>& vec)
    {
        return {+vec.x, +vec.y};
    }

    template<Scalar T>
    constexpr Vec<2, T> operator-(const Vec<2, T>& vec)
    {
        return {-vec.x, -vec.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<2, decltype(T{} + U{})> operator+(const Vec<2, T>& vec, U scalar)
    {
        return {vec.x + scalar, vec.y + scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<2, decltype(T{} + U{})> operator+(T scalar, const Vec<2, U>& vec)
    {
        return {scalar + vec.x, scalar + vec.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<2, decltype(T{} + U{})> operator+(const Vec<2, T>& lhs, const Vec<2, U>& rhs)
    {
        return {lhs.x + rhs.x, lhs.y + rhs.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<2, decltype(T{} - U{})> operator-(const Vec<2, T>& vec, U scalar)
    {
        return {vec.x - scalar, vec.y - scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<2, decltype(T{} - U{})> operator-(T scalar, const Vec<2, U>& vec)
    {
        return {scalar - vec.x, scalar - vec.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<2, decltype(T{} - U{})> operator-(const Vec<2, T>& lhs, const Vec<2, U>& rhs)
    {
        return {lhs.x - rhs.x, lhs.y - rhs.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<2, decltype(T{} * U{})> operator*(const Vec<2, T>& vec, U scalar)
    {
        return {vec.x * scalar, vec.y * scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<2, decltype(T{} * U{})> operator*(T scalar, const Vec<2, U>& vec)
    {
        return {scalar * vec.x, scalar * vec.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<2, decltype(T{} * U{})> operator*(const Vec<2, T>& lhs, const Vec<2, U>& rhs)
    {
        return {lhs.x * rhs.x, lhs.y * rhs.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<2, decltype(T{} / U{})> operator/(const Vec<2, T>& vec, U scalar)
    {
        return {vec.x / scalar, vec.y / scalar};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<2, decltype(T{} / U{})> operator/(T scalar, const Vec<2, U>& vec)
    {
        return {scalar / vec.x, scalar / vec.y};
    }

    template<Scalar T, Scalar U>
    constexpr Vec<2, decltype(T{} / U{})> operator/(const Vec<2, T>& lhs, const Vec<2, U>& rhs)
    {
        return {lhs.x / rhs.x, lhs.y / rhs.y};
    }

    constexpr Vec<2, bool> operator&&(const Vec<2, bool>& lhs, const Vec<2, bool>& rhs)
    {
        return {lhs.x && rhs.x, lhs.y && rhs.y};
    }

    constexpr Vec<2, bool> operator||(const Vec<2, bool>& lhs, const Vec<2, bool>& rhs)
    {
        return {lhs.x || rhs.x, lhs.y || rhs.y};
    }

    using Vec2 = Vec<2, float>;
    using Vec2f = Vec<2, float>;
    using Vec2d = Vec<2, double>;
    using Vec2i = Vec<2, int>;
    using Vec2z = Vec<2, ptrdiff_t>;
    using Vec2uz = Vec<2, size_t>;
    using Vec2u32 = Vec<2, uint32_t>;
}
