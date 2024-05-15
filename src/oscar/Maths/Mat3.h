#pragma once

#include <oscar/Maths/Mat.h>
#include <oscar/Maths/Vec3.h>

#include <cstddef>
#include <cstdint>

namespace osc
{
    // a 3x3 column-major matrix
    template<typename T>
    struct Mat<3, 3, T> {
        using col_type = Vec<3, T>;
        using row_type = Vec<3, T>;
        using transpose_type = Mat<3, 3, T>;
        using type = Mat<3, 3, T>;
        using value_type = col_type;
        using element_type = T;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = col_type&;
        using const_reference = const col_type&;
        using pointer = col_type*;
        using const_pointer = const col_type*;
        using iterator = col_type*;
        using const_iterator = const col_type*;

        constexpr Mat() = default;

        explicit constexpr Mat(T s) :
            value{
                col_type{s, T{}, T{}},
                col_type{T{}, s, T{}},
                col_type{T{}, T{}, s}
            }
        {}

        constexpr Mat(
            T x0, T y0, T z0,
            T x1, T y1, T z1,
            T x2, T y2, T z2) :

            value{
                col_type{x0, y0, z0},
                col_type{x1, y1, z1},
                col_type{x2, y2, z2}
            }
        {}

        constexpr Mat(
            const col_type& v0,
            const col_type& v1,
            const col_type& v2) :

            value{v0, v1, v2}
        {}

        template<
            typename X0, typename Y0, typename Z0,
            typename X1, typename Y1, typename Z1,
            typename X2, typename Y2, typename Z2>
        constexpr Mat(
            X0 x0, Y0 y0, Z0 z0,
            X1 x1, Y1 y1, Z1 z1,
            X2 x2, Y2 y2, Z2 z2) :

            value{
                col_type{x0, y0, z0},
                col_type{x1, y1, z1},
                col_type{x2, y2, z2}
            }
        {}

        template<typename V1, typename V2, typename V3>
        constexpr Mat(
            const Vec<3, V1>& v1,
            const Vec<3, V2>& v2,
            const Vec<3, V3>& v3) :

            value{
                col_type{v1},
                col_type{v2},
                col_type{v3}
            }
        {}

        template<typename U>
        explicit constexpr Mat(const Mat<3, 3, U>& m) :
            value{
                col_type{m[0]},
                col_type{m[1]},
                col_type{m[2]}
            }
        {}

        explicit constexpr Mat(const Mat<4, 4, T>& m) :
            value{
                col_type{m[0]},
                col_type{m[1]},
                col_type{m[2]}
            }
        {}

        template<typename U>
        Mat<3, 3, T>& operator=(const Mat<3, 3, U>& m)
        {
            this->value[0] = m[0];
            this->value[1] = m[1];
            this->value[2] = m[2];
            return *this;
        }

        constexpr size_type size() const { return 3; }
        constexpr pointer data() { return value; }
        constexpr const_pointer data() const { return value; }
        constexpr iterator begin() { return data(); }
        constexpr const_iterator begin() const { return data(); }
        constexpr iterator end() { return data() + size(); }
        constexpr const_iterator end() const { return data() + size(); }
        constexpr reference operator[](size_type pos) { return begin()[pos]; }
        constexpr const_reference operator[](size_type pos) const { return begin()[pos]; }

        friend constexpr bool operator==(const Mat&, const Mat&) = default;

        template<typename U>
        Mat<3, 3, T>& operator+=(U s)
        {
            this->value[0] += s;
            this->value[1] += s;
            this->value[2] += s;
            return *this;
        }

        template<typename U>
        Mat<3, 3, T>& operator+=(const Mat<3, 3, U>& m)
        {
            this->value[0] += m[0];
            this->value[1] += m[1];
            this->value[2] += m[2];
            return *this;
        }

        template<typename U>
        Mat<3, 3, T>& operator-=(U s)
        {
            this->value[0] -= s;
            this->value[1] -= s;
            this->value[2] -= s;
            return *this;
        }

        template<typename U>
        Mat<3, 3, T>& operator-=(const Mat<3, 3, U>& m)
        {
            this->value[0] -= m[0];
            this->value[1] -= m[1];
            this->value[2] -= m[2];
            return *this;
        }

        template<typename U>
        Mat<3, 3, T>& operator*=(U s)
        {
            this->value[0] *= s;
            this->value[1] *= s;
            this->value[2] *= s;
            return *this;
        }

        template<typename U>
        Mat<3, 3, T>& operator*=(const Mat<3, 3, U>& m)
        {
            return (*this = *this * m);
        }

        template<typename U>
        Mat<3, 3, T>& operator/=(U s)
        {
            this->value[0] /= s;
            this->value[1] /= s;
            this->value[2] /= s;
            return *this;
        }

        template<typename U>
        Mat<3, 3, T>& operator/=(const Mat<3, 3, U>& m)
        {
            return *this /= inverse(m);
        }

        Mat<3, 3, T>& operator++()
        {
            ++this->value[0];
            ++this->value[1];
            ++this->value[2];
            return *this;
        }

        Mat<3, 3, T>& operator--()
        {
            --this->value[0];
            --this->value[1];
            --this->value[2];
            return *this;
        }

        Mat<3, 3, T> operator++(int)
        {
            Mat<3, 3, T> copy{*this};
            ++*this;
            return copy;
        }

        Mat<3, 3, T> operator--(int)
        {
            Mat<3, 3, T> copy{*this};
            --*this;
            return copy;
        }

    private:
        col_type value[3];
    };

    template<typename T>
    Mat<3, 3, T> operator+(const Mat<3, 3, T>& m)
    {
        return m;
    }

    template<typename T>
    Mat<3, 3, T> operator-(const Mat<3, 3, T>& m)
    {
        return Mat<3, 3, T>{-m[0], -m[1], -m[2]};
    }

    template<typename T>
    Mat<3, 3, T> operator+(const Mat<3, 3, T>& m, T scalar)
    {
        return Mat<3, 3, T>{m[0] + scalar, m[1] + scalar, m[2] + scalar};
    }

    template<typename T>
    Mat<3, 3, T> operator+(T scalar, const Mat<3, 3, T>& m)
    {
        return Mat<3, 3, T>{scalar + m[0], scalar + m[1], scalar + m[2]};
    }

    template<typename T>
    Mat<3, 3, T> operator+(const Mat<3, 3, T>& m1, const Mat<3, 3, T>& m2)
    {
        return Mat<3, 3, T>{m1[0] + m2[0], m1[1] + m2[1], m1[2] + m2[2]};
    }

    template<typename T>
    Mat<3, 3, T> operator-(const Mat<3, 3, T>& m, T scalar)
    {
        return Mat<3, 3, T>{m[0] - scalar, m[1] - scalar, m[2] - scalar};
    }

    template<typename T>
    Mat<3, 3, T> operator-(T scalar, const Mat<3, 3, T>& m)
    {
        return Mat<3, 3, T>{scalar - m[0], scalar - m[1], scalar - m[2]};
    }

    template<typename T>
    Mat<3, 3, T> operator-(const Mat<3, 3, T>& m1, const Mat<3, 3, T>& m2)
    {
        return Mat<3, 3, T>{m1[0] - m2[0], m1[1] - m2[1], m1[2] - m2[2]};
    }

    template<typename T>
    Mat<3, 3, T> operator*(const Mat<3, 3, T>& m, T scalar)
    {
        return Mat<3, 3, T>{m[0] * scalar, m[1] * scalar, m[2] * scalar};
    }

    template<typename T>
    Mat<3, 3, T> operator*(T scalar, const Mat<3, 3, T>& m)
    {
        return Mat<3, 3, T>{scalar * m[0], scalar * m[1], scalar * m[2]};
    }

    template<typename T>
    typename Mat<3, 3, T>::col_type operator*(const Mat<3, 3, T>& m, const typename Mat<3, 3, T>::row_type& v)
    {
        return typename Mat<3, 3, T>::col_type(
            m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z,
            m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z,
            m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z
        );
    }

    template<typename T>
    typename Mat<3, 3, T>::row_type operator*(const typename Mat<3, 3, T>::col_type& v, const Mat<3, 3, T>& m)
    {
        return typename Mat<3, 3, T>::row_type(
            m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z
        );
    }

    template<typename T>
    Mat<3, 3, T> operator*(const Mat<3, 3, T>& a, const Mat<3, 3, T>& b)
    {
        const T& a00 = a[0][0];
        const T& a01 = a[0][1];
        const T& a02 = a[0][2];
        const T& a10 = a[1][0];
        const T& a11 = a[1][1];
        const T& a12 = a[1][2];
        const T& a20 = a[2][0];
        const T& a21 = a[2][1];
        const T& a22 = a[2][2];

        const T& b00 = b[0][0];
        const T& b01 = b[0][1];
        const T& b02 = b[0][2];
        const T& b10 = b[1][0];
        const T& b11 = b[1][1];
        const T& b12 = b[1][2];
        const T& b20 = b[2][0];
        const T& b21 = b[2][1];
        const T& b22 = b[2][2];

        Mat<3, 3, T> rv;
        rv[0][0] = a00 * b00 + a10 * b01 + a20 * b02;
        rv[0][1] = a01 * b00 + a11 * b01 + a21 * b02;
        rv[0][2] = a02 * b00 + a12 * b01 + a22 * b02;
        rv[1][0] = a00 * b10 + a10 * b11 + a20 * b12;
        rv[1][1] = a01 * b10 + a11 * b11 + a21 * b12;
        rv[1][2] = a02 * b10 + a12 * b11 + a22 * b12;
        rv[2][0] = a00 * b20 + a10 * b21 + a20 * b22;
        rv[2][1] = a01 * b20 + a11 * b21 + a21 * b22;
        rv[2][2] = a02 * b20 + a12 * b21 + a22 * b22;
        return rv;
    }

    template<typename T>
    Mat<3, 3, T> operator/(const Mat<3, 3, T>& m, T scalar)
    {
        return Mat<3, 3, T>{m[0] / scalar, m[1] / scalar, m[2] / scalar};
    }

    template<typename T>
    Mat<3, 3, T> operator/(T scalar, const Mat<3, 3, T>& m)
    {
        return Mat<3, 3, T>{scalar / m[0], scalar / m[1], scalar / m[2]};
    }

    template<typename T>
    typename Mat<3, 3, T>::col_type operator/(const Mat<3, 3, T>& m, const typename Mat<3, 3, T>::row_type& v)
    {
        return inverse(m) * v;
    }

    template<typename T>
    typename Mat<3, 3, T>::row_type operator/(const typename Mat<3, 3, T>::col_type& v, const Mat<3, 3, T>& m)
    {
        return v * inverse(m);
    }

    template<typename T>
    Mat<3, 3, T> operator/(const Mat<3, 3, T>& m1, const Mat<3, 3, T>& m2)
    {
        Mat<3, 3, T> m1_copy{m1};
        return m1_copy /= m2;
    }

    using Mat3 = Mat<3, 3, float>;
    using Mat3f = Mat<3, 3, float>;
    using Mat3d = Mat<3, 3, double>;
    using Mat3i = Mat<3, 3, int>;
    using Mat3z = Mat<3, 3, ptrdiff_t>;
    using Mat3zu = Mat<3, 3, size_t>;
    using Mat3u32 = Mat<3, 3, uint32_t>;

    template<typename T>
    constexpr T identity();

    template<>
    constexpr Mat3 identity<Mat3>()
    {
        return Mat3{1.0f};
    }
}
