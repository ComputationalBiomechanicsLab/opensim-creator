#pragma once

#include <oscar/Maths/Mat.h>
#include <oscar/Maths/Vec4.h>

#include <cstddef>
#include <cstdint>

namespace osc
{
    // a 4x4 column-major matrix
    template<typename T>
    struct Mat<4, 4, T> {
        using col_type = Vec<4, T>;
        using row_type = Vec<4, T>;
        using transpose_type = Mat<4, 4, T>;
        using type = Mat<4, 4, T>;
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
                col_type{s, T{}, T{}, T{}},
                col_type{T{}, s, T{}, T{}},
                col_type{T{}, T{}, s, T{}},
                col_type{T{}, T{}, T{}, s}
            }
        {}

        constexpr Mat(
            const T& x0, const T& y0, const T& z0, const T& w0,
            const T& x1, const T& y1, const T& z1, const T& w1,
            const T& x2, const T& y2, const T& z2, const T& w2,
            const T& x3, const T& y3, const T& z3, const T& w3) :

            value{
                col_type{x0, y0, z0, w0},
                col_type{x1, y1, z1, w1},
                col_type{x2, y2, z2, w2},
                col_type{x3, y3, z3, w3}
            }
        {}

        constexpr Mat(
            const col_type& v0,
            const col_type& v1,
            const col_type& v2,
            const col_type& v3) :

            value{v0, v1, v2, v3}
        {}

        template<
            typename X0, typename Y0, typename Z0, typename W0,
            typename X1, typename Y1, typename Z1, typename W1,
            typename X2, typename Y2, typename Z2, typename W2,
            typename X3, typename Y3, typename Z3, typename W3>
        constexpr Mat(
            const X0& x0, const Y0& y0, const Z0& z0, const W0& w0,
            const X1& x1, const Y1& y1, const Z1& z1, const W1& w1,
            const X2& x2, const Y2& y2, const Z2& z2, const W2& w2,
            const X3& x3, const Y3& y3, const Z3& z3, const W3& w3) :

            value{
                col_type{x0, y0, z0, w0},
                col_type{x1, y1, z1, w1},
                col_type{x2, y2, z2, w2},
                col_type{x3, y3, z3, w3}
            }
        {}

        template<typename V1, typename V2, typename V3, typename V4>
        constexpr Mat(
            const Vec<4, V1>& v1,
            const Vec<4, V2>& v2,
            const Vec<4, V3>& v3,
            const Vec<4, V4>& v4) :

            value{
                col_type{v1},
                col_type{v2},
                col_type{v3},
                col_type{v4}
            }
        {}

        constexpr Mat(const Mat<3, 3, T>& m) :
            value{
                col_type{m[0], T{}},
                col_type{m[1], T{}},
                col_type{m[2], T{}},
                col_type{T{}, T{}, T{}, T(1)}
            }
        {}

        template<typename U>
        Mat& operator=(const Mat<4, 4, U>& m)
        {
            this->value[0] = m[0];
            this->value[1] = m[1];
            this->value[1] = m[1];
            this->value[2] = m[2];
            return *this;
        }

        constexpr size_type size() const { return 4; }
        constexpr pointer data() { return value; }
        constexpr const_pointer data() const { return value; }
        constexpr iterator begin() { return data(); }
        constexpr const_iterator begin() const { return data(); }
        constexpr iterator end() { return data() + size(); }
        constexpr const_iterator end() const { return data() + size(); }
        constexpr reference operator[](size_type pos) { return begin()[pos]; }
        constexpr const_reference operator[](size_type pos) const { return begin()[pos]; }

        friend bool operator==(const Mat&, const Mat&) = default;

        template<typename U>
        Mat& operator+=(U s)
        {
            this->value[0] += s;
            this->value[1] += s;
            this->value[2] += s;
            this->value[3] += s;
            return *this;
        }

        template<typename U>
        Mat& operator+=(const Mat<4, 4, U>& m)
        {
            this->value[0] += m[0];
            this->value[1] += m[1];
            this->value[2] += m[2];
            this->value[3] += m[3];
            return *this;
        }

        template<typename U>
        Mat& operator-=(U s)
        {
            this->value[0] -= s;
            this->value[1] -= s;
            this->value[2] -= s;
            this->value[3] -= s;
            return *this;
        }

        template<typename U>
        Mat& operator-=(const Mat<4, 4, U>& m)
        {
            this->value[0] -= m[0];
            this->value[1] -= m[1];
            this->value[2] -= m[2];
            this->value[3] -= m[3];
            return *this;
        }

        template<typename U>
        Mat& operator*=(U s)
        {
            this->value[0] *= s;
            this->value[1] *= s;
            this->value[2] *= s;
            this->value[3] *= s;
            return *this;
        }

        template<typename U>
        Mat& operator*=(const Mat<4, 4, U>& m)
        {
            return (*this = *this * m);
        }

        template<typename U>
        Mat& operator/=(U s)
        {
            this->value[0] /= s;
            this->value[1] /= s;
            this->value[2] /= s;
            this->value[3] /= s;
            return *this;
        }

        template<typename U>
        Mat& operator/=(const Mat<4, 4, U>& m)
        {
            return *this *= inverse(m);
        }

        Mat& operator++()
        {
            ++this->value[0];
            ++this->value[1];
            ++this->value[2];
            ++this->value[3];
            return *this;
        }

        Mat& operator--()
        {
            --this->value[0];
            --this->value[1];
            --this->value[2];
            --this->value[3];
            return *this;
        }

        Mat operator++(int)
        {
            Mat copy{*this};
            ++*this;
            return copy;
        }

        Mat operator--(int)
        {
            Mat copy{*this};
            ++*this;
            return copy;
        }

    private:
        col_type value[4];
    };

    template<typename T>
    Mat<4, 4, T> operator+(const Mat<4, 4, T>& m)
    {
        return m;
    }

    template<typename T>
    Mat<4, 4, T> operator-(const Mat<4, 4, T>& m)
    {
        return Mat<4, 4, T>(-m[0], -m[1], -m[2], -m[3]);
    }

    template<typename T>
    Mat<4, 4, T> operator+(const Mat<4, 4, T>& m, T scalar)
    {
        return Mat<4, 4, T>(m[0] + scalar, m[1] + scalar, m[2] + scalar, m[3] + scalar);
    }

    template<typename T>
    Mat<4, 4, T> operator+(T scalar, const Mat<4, 4, T>& m)
    {
        return Mat<4, 4, T>(scalar + m[0], scalar + m[1], scalar + m[2], scalar + m[3]);
    }

    template<typename T>
    Mat<4, 4, T> operator+(const Mat<4, 4, T>& m1, const Mat<4, 4, T>& m2)
    {
        return Mat<4, 4, T>(m1[0] + m2[0], m1[1] + m2[1], m1[2] + m2[2], m1[3] + m2[3]);
    }

    template<typename T>
    Mat<4, 4, T> operator-(const Mat<4, 4, T>& m, T scalar)
    {
        return Mat<4, 4, T>(m[0] - scalar, m[1] - scalar, m[2] - scalar, m[3] - scalar);
    }

    template<typename T>
    Mat<4, 4, T> operator-(T scalar, const Mat<4, 4, T>& m)
    {
        return Mat<4, 4, T>(scalar - m[0], scalar - m[1], scalar - m[2], scalar - m[3]);
    }

    template<typename T>
    Mat<4, 4, T> operator-(const Mat<4, 4, T>& m1, const Mat<4, 4, T>& m2)
    {
        return Mat<4, 4, T>(m1[0] - m2[0], m1[1] - m2[1], m1[2] - m2[2], m1[3] - m2[3]);
    }

    template<typename T>
    Mat<4, 4, T> operator*(const Mat<4, 4, T>& m, T scalar)
    {
        return Mat<4, 4, T>(m[0] * scalar, m[1] * scalar, m[2] * scalar, m[3] * scalar);
    }

    template<typename T>
    Mat<4, 4, T> operator*(T scalar, const Mat<4, 4, T>& m)
    {
        return Mat<4, 4, T>(scalar * m[0], scalar * m[1], scalar * m[2], scalar * m[3]);
    }

    template<typename T>
    typename Mat<4, 4, T>::col_type operator*(const Mat<4, 4, T>& m, const typename Mat<4, 4, T>::row_type& v)
    {
        using col_type = typename Mat<4, 4, T>::col_type;

        const col_type mov0(v[0]);
        const col_type mov1(v[1]);
        const col_type mul0 = m[0] * mov0;
        const col_type mul1 = m[1] * mov1;
        const col_type add0 = mul0 + mul1;
        const col_type mov2(v[2]);
        const col_type mov3(v[3]);
        const col_type mul2 = m[2] * mov2;
        const col_type mul3 = m[3] * mov3;
        const col_type add1 = mul2 + mul3;
        const col_type add2 = add0 + add1;
        return add2;
    }

    template<typename T>
    typename Mat<4, 4, T>::row_type operator*(const typename Mat<4, 4, T>::col_type& v, const Mat<4, 4, T>& m)
    {
        return typename Mat<4, 4, T>::row_type(
            m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2] + m[0][3] * v[3],
            m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2] + m[1][3] * v[3],
            m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2] + m[2][3] * v[3],
            m[3][0] * v[0] + m[3][1] * v[1] + m[3][2] * v[2] + m[3][3] * v[3]
        );
    }

    template<typename T>
    Mat<4, 4, T> operator*(const Mat<4, 4, T>& a, const Mat<4, 4, T>& b)
    {
        using col_type = typename Mat<4, 4, T>::col_type;

        const col_type& a0 = a[0];
        const col_type& a1 = a[1];
        const col_type& a2 = a[2];
        const col_type& a3 = a[3];

        const col_type& b0 = b[0];
        const col_type& b1 = b[1];
        const col_type& b2 = b[2];
        const col_type& b3 = b[3];

        Mat<4, 4, T> rv;
        rv[0] = a0 * b0[0] + a1 * b0[1] + a2 * b0[2] + a3 * b0[3];
        rv[1] = a0 * b1[0] + a1 * b1[1] + a2 * b1[2] + a3 * b1[3];
        rv[2] = a0 * b2[0] + a1 * b2[1] + a2 * b2[2] + a3 * b2[3];
        rv[3] = a0 * b3[0] + a1 * b3[1] + a2 * b3[2] + a3 * b3[3];
        return rv;
    }

    template<typename T>
    Mat<4, 4, T> operator/(const Mat<4, 4, T>& m, T scalar)
    {
        return Mat<4, 4, T>(m[0] / scalar, m[1] / scalar, m[2] / scalar, m[3] / scalar);
    }

    template<typename T>
    Mat<4, 4, T> operator/(T scalar, const Mat<4, 4, T>& m)
    {
        return Mat<4, 4, T>(scalar / m[0], scalar / m[1], scalar / m[2], scalar / m[3]);
    }

    template<typename T>
    typename Mat<4, 4, T>::col_type operator/(const Mat<4, 4, T>& m, const typename Mat<4, 4, T>::row_type& v)
    {
        return inverse(m) * v;
    }

    template<typename T>
    typename Mat<4, 4, T>::row_type operator/(const typename Mat<4, 4, T>::col_type& v, const Mat<4, 4, T>& m)
    {
        return v * inverse(m);
    }

    template<typename T>
    Mat<4, 4, T> operator/(const Mat<4, 4, T>& m1, const Mat<4, 4, T>& m2)
    {
        Mat<4, 4, T> m1_copy{m1};
        return m1_copy /= m2;
    }

    using Mat4 = Mat<4, 4, float>;
    using Mat4f = Mat<4, 4, float>;
    using Mat4d = Mat<4, 4, double>;
    using Mat4i = Mat<4, 4, int>;
    using Mat4z = Mat<4, 4, ptrdiff_t>;
    using Mat4zu = Mat<4, 4, size_t>;
    using Mat4u32 = Mat<4, 4, uint32_t>;

    template<typename T>
    constexpr T identity();

    template<>
    constexpr Mat4 identity<Mat4>()
    {
        return Mat4{1.0f};
    }
}
