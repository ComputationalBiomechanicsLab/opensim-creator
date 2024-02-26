#pragma once

#include <oscar/Maths/Vec.h>

#include <cstddef>
#include <cstdint>

namespace osc
{
    // a 4-element vector
    //
    // implementation initially adapted from `glm::vec4`
    template<typename T>
    struct Vec<4, T> {
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
            z{scalar},
            w{scalar}
        {}
        constexpr Vec(T x_, T y_, T z_, T w_) :
            x{x_},
            y{y_},
            z{z_},
            w{w_}
        {}
        template<typename X, typename Y, typename Z, typename W>
        constexpr Vec(X x_, Y y_, Z z_, W w_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(y_)},
            z{static_cast<T>(z_)},
            w{static_cast<T>(w_)}
        {}
        template<typename A, typename B, typename C>
        constexpr Vec(Vec<2, A> const& xy_, B z_, C w_) :
            x{static_cast<T>(xy_.x)},
            y{static_cast<T>(xy_.y)},
            z{static_cast<T>(z_)},
            w{static_cast<T>(w_)}
        {}
        template<typename A, typename B, typename C>
        constexpr Vec(A x_, Vec<2, B> const& yz_, C w_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(yz_.x)},
            z{static_cast<T>(yz_.y)},
            w{static_cast<T>(w_)}
        {}
        template<typename A, typename B, typename C>
        constexpr Vec(A x_, B y_, Vec<2, C> const& zw_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(y_)},
            z{static_cast<T>(zw_.x)},
            w{static_cast<T>(zw_.y)}
        {}
        template<typename A, typename B>
        constexpr Vec(Vec<3, A> const& xyz_, B w_) :
            x{static_cast<T>(xyz_.x)},
            y{static_cast<T>(xyz_.y)},
            z{static_cast<T>(xyz_.z)},
            w{static_cast<T>(w_)}
        {}
        template<typename A, typename B>
        constexpr Vec(A x_, Vec<3, B> const& yzw_) :
            x{static_cast<T>(x_)},
            y{static_cast<T>(yzw_.x)},
            z{static_cast<T>(yzw_.y)},
            w{static_cast<T>(yzw_.z)}
        {}
        template<typename A, typename B>
        constexpr Vec(Vec<2, A> const& xy_, Vec<2, B> const& wz_) :
            x{static_cast<T>(xy_.x)},
            y{static_cast<T>(xy_.y)},
            z{static_cast<T>(wz_.x)},
            w{static_cast<T>(wz_.w)}
        {}
        template<typename U>
        constexpr explicit Vec(Vec<4, U> const& v) :
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
        constexpr reference operator[](size_type i) { return begin()[i]; }
        constexpr const_reference operator[](size_type i) const { return begin()[i]; }

        constexpr friend bool operator==(Vec const&, Vec const&) = default;

        template<typename U>
        constexpr Vec& operator=(Vec<4, U> const& v)
        {
            this->x = static_cast<T>(v.x);
            this->y = static_cast<T>(v.y);
            this->z = static_cast<T>(v.z);
            this->w = static_cast<T>(v.w);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator+=(U scalar)
        {
            this->x += static_cast<T>(scalar);
            this->y += static_cast<T>(scalar);
            this->z += static_cast<T>(scalar);
            this->w += static_cast<T>(scalar);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator+=(Vec<4, U> const& v)
        {
            this->x += static_cast<T>(v.x);
            this->y += static_cast<T>(v.y);
            this->z += static_cast<T>(v.z);
            this->w += static_cast<T>(v.w);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator-=(U scalar)
        {
            this->x -= static_cast<T>(scalar);
            this->y -= static_cast<T>(scalar);
            this->z -= static_cast<T>(scalar);
            this->w -= static_cast<T>(scalar);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator-=(Vec<4, U> const& v)
        {
            this->x -= static_cast<T>(v.x);
            this->y -= static_cast<T>(v.y);
            this->z -= static_cast<T>(v.z);
            this->w -= static_cast<T>(v.w);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator*=(U scalar)
        {
            this->x *= static_cast<T>(scalar);
            this->y *= static_cast<T>(scalar);
            this->z *= static_cast<T>(scalar);
            this->w *= static_cast<T>(scalar);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator*=(Vec<4, U> const& v)
        {
            this->x *= static_cast<T>(v.x);
            this->y *= static_cast<T>(v.y);
            this->z *= static_cast<T>(v.z);
            this->w *= static_cast<T>(v.w);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator/=(U scalar)
        {
            this->x /= static_cast<T>(scalar);
            this->y /= static_cast<T>(scalar);
            this->z /= static_cast<T>(scalar);
            this->w /= static_cast<T>(scalar);
            return *this;
        }

        template<typename U>
        constexpr Vec& operator/=(Vec<4, U> const& v)
        {
            this->x /= static_cast<T>(v.x);
            this->y /= static_cast<T>(v.y);
            this->z /= static_cast<T>(v.z);
            this->w /= static_cast<T>(v.w);
            return *this;
        }

        constexpr Vec& operator++()
        {
            ++this->x;
            ++this->y;
            ++this->z;
            ++this->w;
            return *this;
        }

        constexpr Vec& operator--()
        {
            --this->x;
            --this->y;
            --this->z;
            --this->w;
            return *this;
        }

        constexpr Vec operator++(int)
        {
            Vec copy{*this};
            ++copy;
            return copy;
        }

        constexpr Vec operator--(int)
        {
            Vec copy{*this};
            --copy;
            return copy;
        }

        T x{};
        T y{};
        T z{};
        T w{};
    };

    template<typename T>
    constexpr Vec<4, T> operator+(Vec<4, T> const& v)
    {
        return v;
    }

    template<typename T>
    constexpr Vec<4, T> operator-(Vec<4, T> const& v)
    {
        return Vec<4, T>{-v.x, -v.y, -v.z, -v.w};
    }


    template<typename T>
    constexpr Vec<4, T> operator+(Vec<4, T> const& v, T scalar)
    {
        return Vec<4, T>{v.x + scalar, v.y + scalar, v.z + scalar, v.w + scalar};
    }

    template<typename T>
    constexpr Vec<4, T> operator+(T scalar, Vec<4, T> const& v)
    {
        return Vec<4, T>{scalar + v.x, scalar + v.y, scalar + v.z, scalar + v.w};
    }

    template<typename T>
    constexpr Vec<4, T> operator+(Vec<4, T> const& v1, Vec<4, T> const& v2)
    {
        return Vec<4, T>{v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w};
    }

    template<typename T>
    constexpr Vec<4, T> operator-(Vec<4, T> const& v, T scalar)
    {
        return Vec<4, T>{v.x - scalar, v.y - scalar, v.z - scalar, v.w - scalar};
    }

    template<typename T>
    constexpr Vec<4, T> operator-(T scalar, Vec<4, T> const& v)
    {
        return Vec<4, T>{scalar - v.x, scalar - v.y, scalar - v.z, scalar - v.w};
    }

    template<typename T>
    constexpr Vec<4, T> operator-(Vec<4, T> const& v1, Vec<4, T> const& v2)
    {
        return Vec<4, T>{v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w};
    }

    template<typename T>
    constexpr Vec<4, T> operator*(Vec<4, T> const& v, T scalar)
    {
        return Vec<4, T>{v.x * scalar, v.y * scalar, v.z * scalar, v.w * scalar};
    }

    template<typename T>
    constexpr Vec<4, T> operator*(T scalar, Vec<4, T> const& v)
    {
        return Vec<4, T>{scalar * v.x, scalar * v.y, scalar * v.z, scalar * v.w};
    }

    template<typename T>
    constexpr Vec<4, T> operator*(Vec<4, T> const& v1, Vec<4, T> const& v2)
    {
        return Vec<4, T>{v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w};
    }

    template<typename T>
    constexpr Vec<4, T> operator/(Vec<4, T> const& v, T scalar)
    {
        return Vec<4, T>{v.x / scalar, v.y / scalar, v.z / scalar, v.w / scalar};
    }

    template<typename T>
    constexpr Vec<4, T> operator/(T scalar, Vec<4, T> const& v)
    {
        return Vec<4, T>{scalar / v.x, scalar / v.y, scalar / v.z, scalar / v.w};
    }

    template<typename T>
    constexpr Vec<4, T> operator/(Vec<4, T> const& v1, Vec<4, T> const& v2)
    {
        return Vec<4, T>{v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w};
    }

    constexpr Vec<4, bool> operator&&(Vec<4, bool> const& v1, Vec<4, bool> const& v2)
    {
        return Vec<4, bool>{v1.x && v2.x, v1.y && v2.y, v1.z && v2.z, v1.w && v2.w};
    }

    constexpr Vec<4, bool> operator||(Vec<4, bool> const& v1, Vec<4, bool> const& v2)
    {
        return Vec<4, bool>{v1.x || v2.x, v1.y || v2.y, v1.z || v2.z, v1.w || v2.w};
    }

    using Vec4  = Vec<4, float>;
    using Vec4f = Vec<4, float>;
    using Vec4d = Vec<4, double>;
    using Vec4i = Vec<4, int>;
    using Vec4z = Vec<4, ptrdiff_t>;
    using Vec4uz = Vec<4, size_t>;
    using Vec4u32 = Vec<4, uint32_t>;
}
