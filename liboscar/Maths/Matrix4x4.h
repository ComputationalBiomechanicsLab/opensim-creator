#pragma once

#include <liboscar/Maths/Matrix.h>
#include <liboscar/Maths/Vector4.h>

#include <cstddef>
#include <cstdint>

namespace osc
{
    // a 4x4 column-major matrix
    template<typename T>
    struct Matrix<4, 4, T> {
        using col_type = Vector<4, T>;
        using row_type = Vector<4, T>;
        using transpose_type = Matrix<4, 4, T>;
        using type = Matrix<4, 4, T>;
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

        constexpr Matrix() = default;

        explicit constexpr Matrix(T s) :
            value{
                col_type{s, T{}, T{}, T{}},
                col_type{T{}, s, T{}, T{}},
                col_type{T{}, T{}, s, T{}},
                col_type{T{}, T{}, T{}, s}
            }
        {}

        constexpr Matrix(
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

        constexpr Matrix(
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
        constexpr Matrix(
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
        constexpr Matrix(
            const Vector<4, V1>& v1,
            const Vector<4, V2>& v2,
            const Vector<4, V3>& v3,
            const Vector<4, V4>& v4) :

            value{
                col_type{v1},
                col_type{v2},
                col_type{v3},
                col_type{v4}
            }
        {}

        constexpr Matrix(const Matrix<3, 3, T>& m) :
            value{
                col_type{m[0], T{}},
                col_type{m[1], T{}},
                col_type{m[2], T{}},
                col_type{T{}, T{}, T{}, T(1)}
            }
        {}

        template<typename U>
        Matrix& operator=(const Matrix<4, 4, U>& m)
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

        friend bool operator==(const Matrix&, const Matrix&) = default;

        template<typename U>
        Matrix& operator+=(U s)
        {
            this->value[0] += s;
            this->value[1] += s;
            this->value[2] += s;
            this->value[3] += s;
            return *this;
        }

        template<typename U>
        Matrix& operator+=(const Matrix<4, 4, U>& m)
        {
            this->value[0] += m[0];
            this->value[1] += m[1];
            this->value[2] += m[2];
            this->value[3] += m[3];
            return *this;
        }

        template<typename U>
        Matrix& operator-=(U s)
        {
            this->value[0] -= s;
            this->value[1] -= s;
            this->value[2] -= s;
            this->value[3] -= s;
            return *this;
        }

        template<typename U>
        Matrix& operator-=(const Matrix<4, 4, U>& m)
        {
            this->value[0] -= m[0];
            this->value[1] -= m[1];
            this->value[2] -= m[2];
            this->value[3] -= m[3];
            return *this;
        }

        template<typename U>
        Matrix& operator*=(U s)
        {
            this->value[0] *= s;
            this->value[1] *= s;
            this->value[2] *= s;
            this->value[3] *= s;
            return *this;
        }

        template<typename U>
        Matrix& operator*=(const Matrix<4, 4, U>& m)
        {
            return (*this = *this * m);
        }

        template<typename U>
        Matrix& operator/=(U s)
        {
            this->value[0] /= s;
            this->value[1] /= s;
            this->value[2] /= s;
            this->value[3] /= s;
            return *this;
        }

        template<typename U>
        Matrix& operator/=(const Matrix<4, 4, U>& m)
        {
            return *this *= inverse(m);
        }

        Matrix& operator++()
        {
            ++this->value[0];
            ++this->value[1];
            ++this->value[2];
            ++this->value[3];
            return *this;
        }

        Matrix& operator--()
        {
            --this->value[0];
            --this->value[1];
            --this->value[2];
            --this->value[3];
            return *this;
        }

        Matrix operator++(int)
        {
            Matrix copy{*this};
            ++*this;
            return copy;
        }

        Matrix operator--(int)
        {
            Matrix copy{*this};
            ++*this;
            return copy;
        }

    private:
        col_type value[4];
    };

    template<typename T>
    Matrix<4, 4, T> operator+(const Matrix<4, 4, T>& m)
    {
        return m;
    }

    template<typename T>
    Matrix<4, 4, T> operator-(const Matrix<4, 4, T>& m)
    {
        return Matrix<4, 4, T>(-m[0], -m[1], -m[2], -m[3]);
    }

    template<typename T>
    Matrix<4, 4, T> operator+(const Matrix<4, 4, T>& m, T scalar)
    {
        return Matrix<4, 4, T>(m[0] + scalar, m[1] + scalar, m[2] + scalar, m[3] + scalar);
    }

    template<typename T>
    Matrix<4, 4, T> operator+(T scalar, const Matrix<4, 4, T>& m)
    {
        return Matrix<4, 4, T>(scalar + m[0], scalar + m[1], scalar + m[2], scalar + m[3]);
    }

    template<typename T>
    Matrix<4, 4, T> operator+(const Matrix<4, 4, T>& m1, const Matrix<4, 4, T>& m2)
    {
        return Matrix<4, 4, T>(m1[0] + m2[0], m1[1] + m2[1], m1[2] + m2[2], m1[3] + m2[3]);
    }

    template<typename T>
    Matrix<4, 4, T> operator-(const Matrix<4, 4, T>& m, T scalar)
    {
        return Matrix<4, 4, T>(m[0] - scalar, m[1] - scalar, m[2] - scalar, m[3] - scalar);
    }

    template<typename T>
    Matrix<4, 4, T> operator-(T scalar, const Matrix<4, 4, T>& m)
    {
        return Matrix<4, 4, T>(scalar - m[0], scalar - m[1], scalar - m[2], scalar - m[3]);
    }

    template<typename T>
    Matrix<4, 4, T> operator-(const Matrix<4, 4, T>& m1, const Matrix<4, 4, T>& m2)
    {
        return Matrix<4, 4, T>(m1[0] - m2[0], m1[1] - m2[1], m1[2] - m2[2], m1[3] - m2[3]);
    }

    template<typename T>
    Matrix<4, 4, T> operator*(const Matrix<4, 4, T>& m, T scalar)
    {
        return Matrix<4, 4, T>(m[0] * scalar, m[1] * scalar, m[2] * scalar, m[3] * scalar);
    }

    template<typename T>
    Matrix<4, 4, T> operator*(T scalar, const Matrix<4, 4, T>& m)
    {
        return Matrix<4, 4, T>(scalar * m[0], scalar * m[1], scalar * m[2], scalar * m[3]);
    }

    template<typename T>
    typename Matrix<4, 4, T>::col_type operator*(const Matrix<4, 4, T>& m, const typename Matrix<4, 4, T>::row_type& v)
    {
        using col_type = typename Matrix<4, 4, T>::col_type;

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
    typename Matrix<4, 4, T>::row_type operator*(const typename Matrix<4, 4, T>::col_type& v, const Matrix<4, 4, T>& m)
    {
        return typename Matrix<4, 4, T>::row_type(
            m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2] + m[0][3] * v[3],
            m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2] + m[1][3] * v[3],
            m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2] + m[2][3] * v[3],
            m[3][0] * v[0] + m[3][1] * v[1] + m[3][2] * v[2] + m[3][3] * v[3]
        );
    }

    template<typename T>
    Matrix<4, 4, T> operator*(const Matrix<4, 4, T>& a, const Matrix<4, 4, T>& b)
    {
        using col_type = typename Matrix<4, 4, T>::col_type;

        const col_type& a0 = a[0];
        const col_type& a1 = a[1];
        const col_type& a2 = a[2];
        const col_type& a3 = a[3];

        const col_type& b0 = b[0];
        const col_type& b1 = b[1];
        const col_type& b2 = b[2];
        const col_type& b3 = b[3];

        Matrix<4, 4, T> rv;
        rv[0] = a0 * b0[0] + a1 * b0[1] + a2 * b0[2] + a3 * b0[3];
        rv[1] = a0 * b1[0] + a1 * b1[1] + a2 * b1[2] + a3 * b1[3];
        rv[2] = a0 * b2[0] + a1 * b2[1] + a2 * b2[2] + a3 * b2[3];
        rv[3] = a0 * b3[0] + a1 * b3[1] + a2 * b3[2] + a3 * b3[3];
        return rv;
    }

    template<typename T>
    Matrix<4, 4, T> operator/(const Matrix<4, 4, T>& m, T scalar)
    {
        return Matrix<4, 4, T>(m[0] / scalar, m[1] / scalar, m[2] / scalar, m[3] / scalar);
    }

    template<typename T>
    Matrix<4, 4, T> operator/(T scalar, const Matrix<4, 4, T>& m)
    {
        return Matrix<4, 4, T>(scalar / m[0], scalar / m[1], scalar / m[2], scalar / m[3]);
    }

    template<typename T>
    typename Matrix<4, 4, T>::col_type operator/(const Matrix<4, 4, T>& m, const typename Matrix<4, 4, T>::row_type& v)
    {
        return inverse(m) * v;
    }

    template<typename T>
    typename Matrix<4, 4, T>::row_type operator/(const typename Matrix<4, 4, T>::col_type& v, const Matrix<4, 4, T>& m)
    {
        return v * inverse(m);
    }

    template<typename T>
    Matrix<4, 4, T> operator/(const Matrix<4, 4, T>& m1, const Matrix<4, 4, T>& m2)
    {
        Matrix<4, 4, T> m1_copy{m1};
        return m1_copy /= m2;
    }

    using Matrix4x4 = Matrix<4, 4, float>;
    using Matrix4x4f = Matrix<4, 4, float>;
    using Matrix4x4d = Matrix<4, 4, double>;
    using Matrix4x4i = Matrix<4, 4, int>;
    using Matrix4x4z = Matrix<4, 4, ptrdiff_t>;
    using Matrix4x4zu = Matrix<4, 4, size_t>;
    using Matrix4x4u32 = Matrix<4, 4, uint32_t>;

    template<typename T>
    constexpr T identity();

    template<>
    constexpr Matrix4x4 identity<Matrix4x4>()
    {
        return Matrix4x4{1.0f};
    }
}
