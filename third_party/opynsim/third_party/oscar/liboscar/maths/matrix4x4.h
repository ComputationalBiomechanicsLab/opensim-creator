#pragma once

#include <liboscar/maths/matrix.h>
#include <liboscar/maths/vector4.h>

#include <cstddef>
#include <cstdint>

namespace osc
{
    // a 4x4 column-major matrix
    template<typename T>
    struct Matrix<T, 4, 4> {
        using column_type = Vector<T, 4>;
        using row_type = Vector<T, 4>;
        using value_type = column_type;
        using element_type = T;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = column_type&;
        using const_reference = const column_type&;
        using pointer = column_type*;
        using const_pointer = const column_type*;
        using iterator = column_type*;
        using const_iterator = const column_type*;

        constexpr Matrix() = default;

        explicit constexpr Matrix(T s) :
            value{
                column_type{s, T{}, T{}, T{}},
                column_type{T{}, s, T{}, T{}},
                column_type{T{}, T{}, s, T{}},
                column_type{T{}, T{}, T{}, s}
            }
        {}

        constexpr Matrix(
            const T& x0, const T& y0, const T& z0, const T& w0,
            const T& x1, const T& y1, const T& z1, const T& w1,
            const T& x2, const T& y2, const T& z2, const T& w2,
            const T& x3, const T& y3, const T& z3, const T& w3) :

            value{
                column_type{x0, y0, z0, w0},
                column_type{x1, y1, z1, w1},
                column_type{x2, y2, z2, w2},
                column_type{x3, y3, z3, w3}
            }
        {}

        constexpr Matrix(
            const column_type& v0,
            const column_type& v1,
            const column_type& v2,
            const column_type& v3) :

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
                column_type{x0, y0, z0, w0},
                column_type{x1, y1, z1, w1},
                column_type{x2, y2, z2, w2},
                column_type{x3, y3, z3, w3}
            }
        {}

        template<typename V1, typename V2, typename V3, typename V4>
        constexpr Matrix(
            const Vector<V1, 4>& v1,
            const Vector<V2, 4>& v2,
            const Vector<V3, 4>& v3,
            const Vector<V4, 4>& v4) :

            value{
                column_type{v1},
                column_type{v2},
                column_type{v3},
                column_type{v4}
            }
        {}

        constexpr Matrix(const Matrix<T, 3, 3>& m) :
            value{
                column_type{m[0], T{}},
                column_type{m[1], T{}},
                column_type{m[2], T{}},
                column_type{T{}, T{}, T{}, T(1)}
            }
        {}

        template<typename U>
        Matrix& operator=(const Matrix<U, 4, 4>& m)
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
        Matrix& operator+=(const Matrix<U, 4, 4>& m)
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
        Matrix& operator-=(const Matrix<U, 4, 4>& m)
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
        Matrix& operator*=(const Matrix<U, 4, 4>& m)
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
        Matrix& operator/=(const Matrix<U, 4, 4>& m)
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
        column_type value[4];
    };

    template<typename T>
    Matrix<T, 4, 4> operator+(const Matrix<T, 4, 4>& m)
    {
        return m;
    }

    template<typename T>
    Matrix<T, 4, 4> operator-(const Matrix<T, 4, 4>& m)
    {
        return Matrix<T, 4, 4>(-m[0], -m[1], -m[2], -m[3]);
    }

    template<typename T>
    Matrix<T, 4, 4> operator+(const Matrix<T, 4, 4>& m, T scalar)
    {
        return Matrix<T, 4, 4>(m[0] + scalar, m[1] + scalar, m[2] + scalar, m[3] + scalar);
    }

    template<typename T>
    Matrix<T, 4, 4> operator+(T scalar, const Matrix<T, 4, 4>& m)
    {
        return Matrix<T, 4, 4>(scalar + m[0], scalar + m[1], scalar + m[2], scalar + m[3]);
    }

    template<typename T>
    Matrix<T, 4, 4> operator+(const Matrix<T, 4, 4>& m1, const Matrix<T, 4, 4>& m2)
    {
        return Matrix<T, 4, 4>(m1[0] + m2[0], m1[1] + m2[1], m1[2] + m2[2], m1[3] + m2[3]);
    }

    template<typename T>
    Matrix<T, 4, 4> operator-(const Matrix<T, 4, 4>& m, T scalar)
    {
        return Matrix<T, 4, 4>(m[0] - scalar, m[1] - scalar, m[2] - scalar, m[3] - scalar);
    }

    template<typename T>
    Matrix<T, 4, 4> operator-(T scalar, const Matrix<T, 4, 4>& m)
    {
        return Matrix<T, 4, 4>(scalar - m[0], scalar - m[1], scalar - m[2], scalar - m[3]);
    }

    template<typename T>
    Matrix<T, 4, 4> operator-(const Matrix<T, 4, 4>& m1, const Matrix<T, 4, 4>& m2)
    {
        return Matrix<T, 4, 4>(m1[0] - m2[0], m1[1] - m2[1], m1[2] - m2[2], m1[3] - m2[3]);
    }

    template<typename T>
    Matrix<T, 4, 4> operator*(const Matrix<T, 4, 4>& m, T scalar)
    {
        return Matrix<T, 4, 4>(m[0] * scalar, m[1] * scalar, m[2] * scalar, m[3] * scalar);
    }

    template<typename T>
    Matrix<T, 4, 4> operator*(T scalar, const Matrix<T, 4, 4>& m)
    {
        return Matrix<T, 4, 4>(scalar * m[0], scalar * m[1], scalar * m[2], scalar * m[3]);
    }

    template<typename T>
    typename Matrix<T, 4, 4>::column_type operator*(const Matrix<T, 4, 4>& m, const typename Matrix<T, 4, 4>::row_type& v)
    {
        using column_type = typename Matrix<T, 4, 4>::column_type;

        const column_type mov0(v[0]);
        const column_type mov1(v[1]);
        const column_type mul0 = m[0] * mov0;
        const column_type mul1 = m[1] * mov1;
        const column_type add0 = mul0 + mul1;
        const column_type mov2(v[2]);
        const column_type mov3(v[3]);
        const column_type mul2 = m[2] * mov2;
        const column_type mul3 = m[3] * mov3;
        const column_type add1 = mul2 + mul3;
        const column_type add2 = add0 + add1;
        return add2;
    }

    template<typename T>
    typename Matrix<T, 4, 4>::row_type operator*(const typename Matrix<T, 4, 4>::column_type& v, const Matrix<T, 4, 4>& m)
    {
        return typename Matrix<T, 4, 4>::row_type(
            m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2] + m[0][3] * v[3],
            m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2] + m[1][3] * v[3],
            m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2] + m[2][3] * v[3],
            m[3][0] * v[0] + m[3][1] * v[1] + m[3][2] * v[2] + m[3][3] * v[3]
        );
    }

    template<typename T>
    Matrix<T, 4, 4> operator*(const Matrix<T, 4, 4>& a, const Matrix<T, 4, 4>& b)
    {
        using column_type = typename Matrix<T, 4, 4>::column_type;

        const column_type& a0 = a[0];
        const column_type& a1 = a[1];
        const column_type& a2 = a[2];
        const column_type& a3 = a[3];

        const column_type& b0 = b[0];
        const column_type& b1 = b[1];
        const column_type& b2 = b[2];
        const column_type& b3 = b[3];

        Matrix<T, 4, 4> rv;
        rv[0] = a0 * b0[0] + a1 * b0[1] + a2 * b0[2] + a3 * b0[3];
        rv[1] = a0 * b1[0] + a1 * b1[1] + a2 * b1[2] + a3 * b1[3];
        rv[2] = a0 * b2[0] + a1 * b2[1] + a2 * b2[2] + a3 * b2[3];
        rv[3] = a0 * b3[0] + a1 * b3[1] + a2 * b3[2] + a3 * b3[3];
        return rv;
    }

    template<typename T>
    Matrix<T, 4, 4> operator/(const Matrix<T, 4, 4>& m, T scalar)
    {
        return Matrix<T, 4, 4>(m[0] / scalar, m[1] / scalar, m[2] / scalar, m[3] / scalar);
    }

    template<typename T>
    Matrix<T, 4, 4> operator/(T scalar, const Matrix<T, 4, 4>& m)
    {
        return Matrix<T, 4, 4>(scalar / m[0], scalar / m[1], scalar / m[2], scalar / m[3]);
    }

    template<typename T>
    typename Matrix<T, 4, 4>::column_type operator/(const Matrix<T, 4, 4>& m, const typename Matrix<T, 4, 4>::row_type& v)
    {
        return inverse(m) * v;
    }

    template<typename T>
    typename Matrix<T, 4, 4>::row_type operator/(const typename Matrix<T, 4, 4>::column_type& v, const Matrix<T, 4, 4>& m)
    {
        return v * inverse(m);
    }

    template<typename T>
    Matrix<T, 4, 4> operator/(const Matrix<T, 4, 4>& m1, const Matrix<T, 4, 4>& m2)
    {
        Matrix<T, 4, 4> m1_copy{m1};
        return m1_copy /= m2;
    }

    using Matrix4x4 = Matrix<float, 4, 4>;
    using Matrix4x4f = Matrix<float, 4, 4>;
    using Matrix4x4d = Matrix<double, 4, 4>;
    using Matrix4x4i = Matrix<int, 4, 4>;
    using Matrix4x4z = Matrix<ptrdiff_t, 4, 4>;
    using Matrix4x4zu = Matrix<size_t, 4, 4>;
    using Matrix4x4u32 = Matrix<uint32_t, 4, 4>;

    template<typename T>
    constexpr T identity();

    template<>
    constexpr Matrix4x4 identity<Matrix4x4>()
    {
        return Matrix4x4{1.0f};
    }
}
