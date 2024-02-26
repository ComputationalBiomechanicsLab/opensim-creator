#pragma once

#include <oscar/Maths/Vec.h>

#include <cstddef>
#include <cstdint>

namespace osc
{
    // a 3-element vector
    //
    // implementation initially adapted from `glm::vec3`
    template<typename T>
    struct Vec<3, T> {
        using value_type = T;
        using element_type = T;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = T&;
        using const_reference = T const&;
        using pointer = T*;
        using const_pointer = T const*;
        using iterator = T*;
        using const_iterator = T const*;

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
        constexpr Vec(Vec<2, A> const& xy_, B z_) :
            x{static_cast<T>(xy_.x)},
            y{static_cast<T>(xy_.y)},
            z{static_cast<T>(z_)}
        {}
        template<typename A, typename B>
        constexpr Vec(A x_, Vec<2, B> const& yz_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(yz_.y)},
            z{static_cast<T>(yz_.z)}
        {}
        template<typename U>
        constexpr Vec(Vec<4, U> const& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)},
            z{static_cast<T>(v.z)}
        {}
        template<typename U>
        constexpr Vec(Vec<3, U> const& v) :
            x{static_cast<T>(v.x)},
            y{static_cast<T>(v.y)},
            z{static_cast<T>(v.z)}
        {}

        template<typename U>
        constexpr Vec& operator=(Vec<3, U> const& v)
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
        constexpr reference operator[](size_type i) { return begin()[i]; }
        constexpr const_reference operator[](size_type i) const { return begin()[i]; }

        friend constexpr bool operator==(Vec const&, Vec const&) = default;

        template<typename U>
        constexpr Vec& operator+=(U scalar)
        {
            this->x += static_cast<T>(scalar);
            this->y += static_cast<T>(scalar);
            this->z += static_cast<T>(scalar);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator+=(Vec<3, U> const& v)
        {
            this->x += static_cast<T>(v.x);
            this->y += static_cast<T>(v.y);
            this->z += static_cast<T>(v.z);
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
        constexpr Vec& operator-=(Vec<3, U> const& v)
        {
            this->x -= static_cast<T>(v.x);
            this->y -= static_cast<T>(v.y);
            this->z -= static_cast<T>(v.z);
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
        constexpr Vec& operator*=(Vec<3, U> const& v)
        {
            this->x *= static_cast<T>(v.x);
            this->y *= static_cast<T>(v.y);
            this->z *= static_cast<T>(v.z);
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
        constexpr Vec& operator/=(Vec<3, U> const& v)
        {
            this->x /= static_cast<T>(v.x);
            this->y /= static_cast<T>(v.y);
            this->z /= static_cast<T>(v.z);
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
        constexpr Vec with_element(size_type i, U scalar) const
        {
            Vec copy{*this};
            copy[i] = static_cast<T>(scalar);
            return copy;
        }

        T x{};
        T y{};
        T z{};
    };

    template<typename T>
    constexpr Vec<3, T> operator+(Vec<3, T> const& v)
    {
        return v;
    }

    template<typename T>
    constexpr Vec<3, T> operator-(Vec<3, T> const& v)
    {
        return Vec<3, T>{-v.x, -v.y, -v.z};
    }


    template<typename T>
    constexpr Vec<3, T> operator+(Vec<3, T> const& v, T scalar)
    {
        return Vec<3, T>{v.x + scalar, v.y + scalar, v.z + scalar};
    }

    template<typename T>
    constexpr Vec<3, T> operator+(T scalar, Vec<3, T> const& v)
    {
        return Vec<3, T>{scalar + v.x, scalar + v.y, scalar + v.z};
    }

    template<typename T>
    constexpr Vec<3, T> operator+(Vec<3, T> const& v1, Vec<3, T> const& v2)
    {
        return Vec<3, T>{v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
    }


    template<typename T>
    constexpr Vec<3, T> operator-(Vec<3, T> const& v, T scalar)
    {
        return Vec<3, T>{v.x - scalar, v.y - scalar, v.z - scalar};
    }

    template<typename T>
    constexpr Vec<3, T> operator-(T scalar, Vec<3, T> const& v)
    {
        return Vec<3, T>{scalar - v.x, scalar - v.y, scalar - v.z};
    }

    template<typename T>
    constexpr Vec<3, T> operator-(Vec<3, T> const& v1, Vec<3, T> const& v2)
    {
        return Vec<3, T>{v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
    }


    template<typename T>
    constexpr Vec<3, T> operator*(Vec<3, T> const& v, T scalar)
    {
        return Vec<3, T>{v.x * scalar, v.y * scalar, v.z * scalar};
    }

    template<typename T>
    constexpr Vec<3, T> operator*(T scalar, Vec<3, T> const& v)
    {
        return Vec<3, T>{scalar * v.x, scalar * v.y, scalar * v.z};
    }

    template<typename T>
    constexpr Vec<3, T> operator*(Vec<3, T> const& v1, Vec<3, T> const& v2)
    {
        return Vec<3, T>{v1.x * v2.x, v1.y * v2.y, v1.z * v2.z};
    }

    template<typename T>
    constexpr Vec<3, T> operator/(Vec<3, T> const& v, T scalar)
    {
        return Vec<3, T>{v.x / scalar, v.y / scalar, v.z / scalar};
    }

    template<typename T>
    constexpr Vec<3, T> operator/(T scalar, Vec<3, T> const& v)
    {
        return Vec<3, T>{scalar / v.x, scalar / v.y, scalar / v.z};
    }

    template<typename T>
    constexpr Vec<3, T> operator/(Vec<3, T> const& v1, Vec<3, T> const& v2)
    {
        return Vec<3, T>{v1.x / v2.x, v1.y / v2.y, v1.z / v2.z};
    }

    constexpr Vec<3, bool> operator&&(Vec<3, bool> const& v1, Vec<3, bool> const& v2)
    {
        return Vec<3, bool>{v1.x && v2.x, v1.y && v2.y, v1.z && v2.z};
    }

    constexpr Vec<3, bool> operator||(Vec<3, bool> const& v1, Vec<3, bool> const& v2)
    {
        return Vec<3, bool>{v1.x || v2.x, v1.y || v2.y, v1.z || v2.z};
    }

    using Vec3  = Vec<3, float>;
    using Vec3f = Vec<3, float>;
    using Vec3d = Vec<3, double>;
    using Vec3i = Vec<3, int>;
    using Vec3z = Vec<3, ptrdiff_t>;
    using Vec3uz = Vec<3, size_t>;
    using Vec3u32 = Vec<3, uint32_t>;
}
