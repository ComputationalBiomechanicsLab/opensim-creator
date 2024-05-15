#pragma once

#include <oscar/Maths/Vec.h>

#include <cstddef>
#include <cstdint>

namespace osc
{
    template<typename T>
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
        constexpr explicit Vec(T scalar) :
            x{scalar},
            y{scalar},
            z{scalar}
        {}
        constexpr Vec(T x_, T y_, T z_) :
            x{x_},
            y{y_},
            z{z_}
        {}
        template<typename X, typename Y, typename Z>
        constexpr Vec(X x_, Y y_, Z z_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(y_)},
            z{static_cast<T>(z_)}
        {}
        template<typename A, typename B>
        constexpr Vec(const Vec<2, A>& xy_, B z_) :
            x{static_cast<T>(xy_.x)},
            y{static_cast<T>(xy_.y)},
            z{static_cast<T>(z_)}
        {}
        template<typename A, typename B>
        constexpr Vec(A x_, const Vec<2, B>& yz_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(yz_.y)},
            z{static_cast<T>(yz_.z)}
        {}
        template<typename U>
        constexpr Vec(const Vec<4, U>& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)},
            z{static_cast<T>(v.z)}
        {}
        template<typename U>
        constexpr Vec(const Vec<3, U>& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)},
            z{static_cast<T>(v.z)}
        {}

        template<typename U>
        constexpr Vec& operator=(const Vec<3, U>& v)
        {
            this->x = static_cast<T>(v.x);
            this->y = static_cast<T>(v.y);
            this->z = static_cast<T>(v.z);
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

        template<typename U>
        constexpr Vec& operator+=(U scalar)
        {
            this->x += static_cast<T>(scalar);
            this->y += static_cast<T>(scalar);
            this->z += static_cast<T>(scalar);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator+=(const Vec<3, U>& rhs)
        {
            this->x += static_cast<T>(rhs.x);
            this->y += static_cast<T>(rhs.y);
            this->z += static_cast<T>(rhs.z);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator-=(U scalar)
        {
            this->x -= static_cast<T>(scalar);
            this->y -= static_cast<T>(scalar);
            this->z -= static_cast<T>(scalar);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator-=(const Vec<3, U>& rhs)
        {
            this->x -= static_cast<T>(rhs.x);
            this->y -= static_cast<T>(rhs.y);
            this->z -= static_cast<T>(rhs.z);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator*=(U scalar)
        {
            this->x *= static_cast<T>(scalar);
            this->y *= static_cast<T>(scalar);
            this->z *= static_cast<T>(scalar);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator*=(const Vec<3, U>& rhs)
        {
            this->x *= static_cast<T>(rhs.x);
            this->y *= static_cast<T>(rhs.y);
            this->z *= static_cast<T>(rhs.z);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator/=(U scalar)
        {
            this->x /= static_cast<T>(scalar);
            this->y /= static_cast<T>(scalar);
            this->z /= static_cast<T>(scalar);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator/=(const Vec<3, U>& rhs)
        {
            this->x /= static_cast<T>(rhs.x);
            this->y /= static_cast<T>(rhs.y);
            this->z /= static_cast<T>(rhs.z);
            return *this;
        }

        constexpr Vec& operator++()
        {
            ++this->x;
            ++this->y;
            ++this->z;
            return *this;
        }

        constexpr Vec& operator--()
        {
            --this->x;
            --this->y;
            --this->z;
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
        T z{};
    };

    template<typename T>
    constexpr Vec<3, T> operator+(const Vec<3, T>& vec)
    {
        return vec;
    }

    template<typename T>
    constexpr Vec<3, T> operator-(const Vec<3, T>& vec)
    {
        return Vec<3, T>{-vec.x, -vec.y, -vec.z};
    }


    template<typename T>
    constexpr Vec<3, T> operator+(const Vec<3, T>& vec, T scalar)
    {
        return Vec<3, T>{vec.x + scalar, vec.y + scalar, vec.z + scalar};
    }

    template<typename T>
    constexpr Vec<3, T> operator+(T scalar, const Vec<3, T>& vec)
    {
        return Vec<3, T>{scalar + vec.x, scalar + vec.y, scalar + vec.z};
    }

    template<typename T>
    constexpr Vec<3, T> operator+(const Vec<3, T>& lhs, const Vec<3, T>& rhs)
    {
        return Vec<3, T>{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
    }


    template<typename T>
    constexpr Vec<3, T> operator-(const Vec<3, T>& vec, T scalar)
    {
        return Vec<3, T>{vec.x - scalar, vec.y - scalar, vec.z - scalar};
    }

    template<typename T>
    constexpr Vec<3, T> operator-(T scalar, const Vec<3, T>& vec)
    {
        return Vec<3, T>{scalar - vec.x, scalar - vec.y, scalar - vec.z};
    }

    template<typename T>
    constexpr Vec<3, T> operator-(const Vec<3, T>& lhs, const Vec<3, T>& rhs)
    {
        return Vec<3, T>{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
    }


    template<typename T>
    constexpr Vec<3, T> operator*(const Vec<3, T>& vec, T scalar)
    {
        return Vec<3, T>{vec.x * scalar, vec.y * scalar, vec.z * scalar};
    }

    template<typename T>
    constexpr Vec<3, T> operator*(T scalar, const Vec<3, T>& vec)
    {
        return Vec<3, T>{scalar * vec.x, scalar * vec.y, scalar * vec.z};
    }

    template<typename T>
    constexpr Vec<3, T> operator*(const Vec<3, T>& lhs, const Vec<3, T>& rhs)
    {
        return Vec<3, T>{lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
    }

    template<typename T>
    constexpr Vec<3, T> operator/(const Vec<3, T>& vec, T scalar)
    {
        return Vec<3, T>{vec.x / scalar, vec.y / scalar, vec.z / scalar};
    }

    template<typename T>
    constexpr Vec<3, T> operator/(T scalar, const Vec<3, T>& vec)
    {
        return Vec<3, T>{scalar / vec.x, scalar / vec.y, scalar / vec.z};
    }

    template<typename T>
    constexpr Vec<3, T> operator/(const Vec<3, T>& lhs, const Vec<3, T>& rhs)
    {
        return Vec<3, T>{lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
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
