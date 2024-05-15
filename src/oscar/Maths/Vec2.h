#pragma once

#include <oscar/Maths/Vec.h>

#include <cstddef>
#include <cstdint>

namespace osc
{
    template<typename T>
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
        constexpr explicit Vec(T scalar) :
            x{scalar},
            y{scalar}
        {}
        constexpr Vec(T x_, T y_) :
            x{x_},
            y{y_}
        {}
        template<typename A, typename B>
        constexpr Vec(A x_, B y_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(y_)}
        {}
        template<typename U>
        constexpr Vec(const Vec<2, U>& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)}
        {}
        template<typename U>
        constexpr Vec(const Vec<3, U>& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)}
        {}
        template<typename U>
        constexpr Vec(const Vec<4, U>& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)}
        {}

        template<typename U>
        constexpr Vec& operator=(const Vec<2, U>& v)
        {
            this->x = static_cast<T>(v.x);
            this->y = static_cast<T>(v.y);
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

        template<typename U>
        constexpr Vec<2, T>& operator+=(U scalar)
        {
            this->x += static_cast<T>(scalar);
            this->y += static_cast<T>(scalar);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator+=(const Vec<2, U>& rhs)
        {
            this->x += static_cast<T>(rhs.x);
            this->y += static_cast<T>(rhs.y);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator-=(U scalar)
        {
            this->x -= static_cast<T>(scalar);
            this->y -= static_cast<T>(scalar);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator-=(const Vec<2, U>& rhs)
        {
            this->x -= static_cast<T>(rhs.x);
            this->y -= static_cast<T>(rhs.y);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator*=(U scalar)
        {
            this->x *= static_cast<T>(scalar);
            this->y *= static_cast<T>(scalar);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator*=(Vec<2, U> const& rhs)
        {
            this->x *= static_cast<T>(rhs.x);
            this->y *= static_cast<T>(rhs.y);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator/=(U scalar)
        {
            this->x /= static_cast<T>(scalar);
            this->y /= static_cast<T>(scalar);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator/=(const Vec<2, U>& rhs)
        {
            this->x /= static_cast<T>(rhs.x);
            this->y /= static_cast<T>(rhs.y);
            return *this;
        }

        constexpr Vec& operator++()
        {
            ++this->x;
            ++this->y;
            return *this;
        }

        constexpr Vec& operator--()
        {
            --this->x;
            --this->y;
            return *this;
        }

        constexpr Vec operator++(int)
        {
            Vec copy{*this};
            ++(*this);
            return copy;
        }

        constexpr Vec operator--(int)
        {
            Vec copy{*this};
            --(*this);
            return copy;
        }

        template<typename U>
        constexpr Vec with_element(size_type pos, U scalar) const
        {
            Vec copy{*this};
            copy[pos] = static_cast<T>(scalar);
            return copy;
        }

        T x{};
        T y{};
    };

    template<typename T>
    constexpr Vec<2, T> operator+(const Vec<2, T>& vec)
    {
        return vec;
    }

    template<typename T>
    constexpr Vec<2, T> operator-(const Vec<2, T>& vec)
    {
        return Vec<2, T>{-vec.x, -vec.y};
    }

    template<typename T>
    constexpr Vec<2, T> operator+(const Vec<2, T>& vec, T scalar)
    {
        return Vec<2, T>{vec.x + scalar, vec.y + scalar};
    }

    template<typename T>
    constexpr Vec<2, T> operator+(T scalar, const Vec<2, T>& vec)
    {
        return Vec<2, T>{scalar + vec.x, scalar + vec.y};
    }

    template<typename T>
    constexpr Vec<2, T> operator+(const Vec<2, T>& lhs, const Vec<2, T>& rhs)
    {
        return Vec<2, T>{lhs.x + rhs.x, lhs.y + rhs.y};
    }

    template<typename T>
    constexpr Vec<2, T> operator-(const Vec<2, T>& vec, T scalar)
    {
        return Vec<2, T>{vec.x - scalar, vec.y - scalar};
    }

    template<typename T>
    constexpr Vec<2, T> operator-(T scalar, const Vec<2, T>& vec)
    {
        return Vec<2, T>{scalar - vec.x, scalar - vec.y};
    }

    template<typename T>
    constexpr Vec<2, T> operator-(const Vec<2, T>& lhs, const Vec<2, T>& rhs)
    {
        return Vec<2, T>{lhs.x - rhs.x, lhs.y - rhs.y};
    }

    template<typename T>
    constexpr Vec<2, T> operator*(const Vec<2, T>& vec, T scalar)
    {
        return Vec<2, T>{vec.x * scalar, vec.y * scalar};
    }

    template<typename T>
    constexpr Vec<2, T> operator*(T scalar, const Vec<2, T>& vec)
    {
        return Vec<2, T>{scalar * vec.x, scalar * vec.y};
    }

    template<typename T>
    constexpr Vec<2, T> operator*(const Vec<2, T>& lhs, const Vec<2, T>& rhs)
    {
        return Vec<2, T>{lhs.x * rhs.x, lhs.y * rhs.y};
    }

    template<typename T>
    constexpr Vec<2, T> operator/(const Vec<2, T>& vec, T scalar)
    {
        return Vec<2, T>{vec.x / scalar, vec.y / scalar};
    }

    template<typename T>
    constexpr Vec<2, T> operator/(T scalar, const Vec<2, T>& vec)
    {
        return Vec<2, T>{scalar / vec.x, scalar / vec.y};
    }

    template<typename T>
    constexpr Vec<2, T> operator/(const Vec<2, T>& lhs, const Vec<2, T>& rhs)
    {
        return Vec<2, T>{lhs.x / rhs.x, lhs.y / rhs.y};
    }

    constexpr Vec<2, bool> operator&&(const Vec<2, bool>& lhs, const Vec<2, bool>& rhs)
    {
        return Vec<2, bool>{lhs.x && rhs.x, lhs.y && rhs.y};
    }

    constexpr Vec<2, bool> operator||(const Vec<2, bool>& lhs, const Vec<2, bool>& rhs)
    {
        return Vec<2, bool>{lhs.x || rhs.x, lhs.y || rhs.y};
    }

    using Vec2 = Vec<2, float>;
    using Vec2f = Vec<2, float>;
    using Vec2d = Vec<2, double>;
    using Vec2i = Vec<2, int>;
    using Vec2z = Vec<2, ptrdiff_t>;
    using Vec2uz = Vec<2, size_t>;
    using Vec2u32 = Vec<2, uint32_t>;
}
