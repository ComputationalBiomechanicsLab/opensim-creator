#pragma once

#include <oscar/Maths/Mat.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Utils/HashHelpers.h>

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>

namespace osc
{
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
        using const_reference = col_type const&;
        using pointer = col_type*;
        using const_pointer = col_type const*;
        using iterator = col_type*;
        using const_iterator = col_type const*;

        static constexpr size_type length() { return 4; }

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
            T const& x0, T const& y0, T const& z0, T const& w0,
            T const& x1, T const& y1, T const& z1, T const& w1,
            T const& x2, T const& y2, T const& z2, T const& w2,
            T const& x3, T const& y3, T const& z3, T const& w3) :

            value{
                col_type{x0, y0, z0, w0},
                col_type{x1, y1, z1, w1},
                col_type{x2, y2, z2, w2},
                col_type{x3, y3, z3, w3}
            }
        {}

        constexpr Mat(
            col_type const& v0,
            col_type const& v1,
            col_type const& v2,
            col_type const& v3) :

            value{v0, v1, v2, v3}
        {}

        template<
            typename X1, typename Y1, typename Z1, typename W1,
            typename X2, typename Y2, typename Z2, typename W2,
            typename X3, typename Y3, typename Z3, typename W3,
            typename X4, typename Y4, typename Z4, typename W4>
        constexpr Mat(
            X1 const& x1, Y1 const& y1, Z1 const& z1, W1 const& w1,
            X2 const& x2, Y2 const& y2, Z2 const& z2, W2 const& w2,
            X3 const& x3, Y3 const& y3, Z3 const& z3, W3 const& w3,
            X4 const& x4, Y4 const& y4, Z4 const& z4, W4 const& w4) :

            value{
                col_type{x1, y1, z1, w1},
                col_type{x2, y2, z2, w2},
                col_type{x3, y3, z3, w3},
                col_type{x4, y4, z4, w4}
            }
        {}

        template<typename V1, typename V2, typename V3, typename V4>
        constexpr Mat(
            Vec<4, V1> const& v1,
            Vec<4, V2> const& v2,
            Vec<4, V3> const& v3,
            Vec<4, V4> const& v4) :

            value{
                col_type{v1},
                col_type{v2},
                col_type{v3},
                col_type{v4}
            }
        {}

        constexpr Mat(Mat<3, 3, T> const& m) :
            value{
                col_type{m[0], T{}},
                col_type{m[1], T{}},
                col_type{m[2], T{}},
                col_type{T{}, T{}, T{}, T(1)}
            }
        {}

        template<typename U>
        Mat& operator=(Mat<4, 4, U> const& m)
        {
            this->value[0] = m[0];
            this->value[1] = m[1];
            this->value[1] = m[1];
            this->value[2] = m[2];
            return *this;
        }

        constexpr size_type size() const { return length(); }
        constexpr pointer data() { return value; }
        constexpr const_pointer data() const { return value; }
        constexpr iterator begin() { return data(); }
        constexpr const_iterator begin() const { return data(); }
        constexpr iterator end() { return data() + size(); }
        constexpr const_iterator end() const { return data() + size(); }
        constexpr reference operator[](size_type i) { return begin()[i]; }
        constexpr const_reference operator[](size_type i) const { return begin()[i]; }

        friend bool operator==(Mat const&, Mat const&) = default;

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
        Mat& operator+=(Mat<4, 4, U> const& m)
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
        Mat& operator-=(Mat<4, 4, U> const& m)
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
        Mat& operator*=(Mat<4, 4, U> const& m)
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
        Mat& operator/=(Mat<4, 4, U> const& m)
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
    Mat<4, 4, T> operator+(Mat<4, 4, T> const& m)
    {
        return m;
    }

    template<typename T>
    Mat<4, 4, T> operator-(Mat<4, 4, T> const& m)
    {
        return Mat<4, 4, T>(-m[0], -m[1], -m[2], -m[3]);
    }

    template<typename T>
    Mat<4, 4, T> operator+(Mat<4, 4, T> const& m, T scalar)
    {
        return Mat<4, 4, T>(m[0] + scalar, m[1] + scalar, m[2] + scalar, m[3] + scalar);
    }

    template<typename T>
    Mat<4, 4, T> operator+(T scalar, Mat<4, 4, T> const& m)
    {
        return Mat<4, 4, T>(scalar + m[0], scalar + m[1], scalar + m[2], scalar + m[3]);
    }

    template<typename T>
    Mat<4, 4, T> operator+(Mat<4, 4, T> const& m1, Mat<4, 4, T> const& m2)
    {
        return Mat<4, 4, T>(m1[0] + m2[0], m1[1] + m2[1], m1[2] + m2[2], m1[3] + m2[3]);
    }

    template<typename T>
    Mat<4, 4, T> operator-(Mat<4, 4, T> const& m, T scalar)
    {
        return Mat<4, 4, T>(m[0] - scalar, m[1] - scalar, m[2] - scalar, m[3] - scalar);
    }

    template<typename T>
    Mat<4, 4, T> operator-(T scalar, Mat<4, 4, T> const& m)
    {
        return Mat<4, 4, T>(scalar - m[0], scalar - m[1], scalar - m[2], scalar - m[3]);
    }

    template<typename T>
    Mat<4, 4, T> operator-(Mat<4, 4, T> const& m1,	Mat<4, 4, T> const& m2)
    {
        return Mat<4, 4, T>(m1[0] - m2[0], m1[1] - m2[1], m1[2] - m2[2], m1[3] - m2[3]);
    }

    template<typename T>
    Mat<4, 4, T> operator*(Mat<4, 4, T> const& m, T scalar)
    {
        return Mat<4, 4, T>(m[0] * scalar, m[1] * scalar, m[2] * scalar, m[3] * scalar);
    }

    template<typename T>
    Mat<4, 4, T> operator*(T scalar, Mat<4, 4, T> const& m)
    {
        return Mat<4, 4, T>(scalar * m[0], scalar * m[1], scalar * m[2], scalar * m[3]);
    }

    template<typename T>
    typename Mat<4, 4, T>::col_type operator*(Mat<4, 4, T> const& m, typename Mat<4, 4, T>::row_type const& v)
    {
        using col_type = typename Mat<4, 4, T>::col_type;

        col_type const Mov0(v[0]);
        col_type const Mov1(v[1]);
        col_type const Mul0 = m[0] * Mov0;
        col_type const Mul1 = m[1] * Mov1;
        col_type const Add0 = Mul0 + Mul1;
        col_type const Mov2(v[2]);
        col_type const Mov3(v[3]);
        col_type const Mul2 = m[2] * Mov2;
        col_type const Mul3 = m[3] * Mov3;
        col_type const Add1 = Mul2 + Mul3;
        col_type const Add2 = Add0 + Add1;
        return Add2;
    }

    template<typename T>
    typename Mat<4, 4, T>::row_type operator*(typename Mat<4, 4, T>::col_type const& v, Mat<4, 4, T> const& m)
    {
        return typename Mat<4, 4, T>::row_type(
            m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2] + m[0][3] * v[3],
            m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2] + m[1][3] * v[3],
            m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2] + m[2][3] * v[3],
            m[3][0] * v[0] + m[3][1] * v[1] + m[3][2] * v[2] + m[3][3] * v[3]
        );
    }

    template<typename T>
    Mat<4, 4, T> operator*(Mat<4, 4, T> const& m1, Mat<4, 4, T> const& m2)
    {
        using col_type = typename Mat<4, 4, T>::col_type;

        col_type const SrcA0 = m1[0];
        col_type const SrcA1 = m1[1];
        col_type const SrcA2 = m1[2];
        col_type const SrcA3 = m1[3];

        col_type const SrcB0 = m2[0];
        col_type const SrcB1 = m2[1];
        col_type const SrcB2 = m2[2];
        col_type const SrcB3 = m2[3];

        Mat<4, 4, T> Result;
        Result[0] = SrcA0 * SrcB0[0] + SrcA1 * SrcB0[1] + SrcA2 * SrcB0[2] + SrcA3 * SrcB0[3];
        Result[1] = SrcA0 * SrcB1[0] + SrcA1 * SrcB1[1] + SrcA2 * SrcB1[2] + SrcA3 * SrcB1[3];
        Result[2] = SrcA0 * SrcB2[0] + SrcA1 * SrcB2[1] + SrcA2 * SrcB2[2] + SrcA3 * SrcB2[3];
        Result[3] = SrcA0 * SrcB3[0] + SrcA1 * SrcB3[1] + SrcA2 * SrcB3[2] + SrcA3 * SrcB3[3];
        return Result;
    }

    template<typename T>
    Mat<4, 4, T> operator/(Mat<4, 4, T> const& m, T scalar)
    {
        return Mat<4, 4, T>(m[0] / scalar, m[1] / scalar, m[2] / scalar, m[3] / scalar);
    }

    template<typename T>
    Mat<4, 4, T> operator/(T scalar, Mat<4, 4, T> const& m)
    {
        return Mat<4, 4, T>(scalar / m[0], scalar / m[1], scalar / m[2], scalar / m[3]);
    }

    template<typename T>
    typename Mat<4, 4, T>::col_type operator/(Mat<4, 4, T> const& m, typename Mat<4, 4, T>::row_type const& v)
    {
        return inverse(m) * v;
    }

    template<typename T>
    typename Mat<4, 4, T>::row_type operator/(typename Mat<4, 4, T>::col_type const& v, Mat<4, 4, T> const& m)
    {
        return v * inverse(m);
    }

    template<typename T>
    Mat<4, 4, T> operator/(Mat<4, 4, T> const& m1,	Mat<4, 4, T> const& m2)
    {
        Mat<4, 4, T> copy{m1};
        return m1 /= m2;
    }

    template<typename T>
    constexpr T const* ValuePtr(Mat<4, 4, T> const& m)
    {
        return m.data()->data();
    }

    template<typename T>
    constexpr T* ValuePtr(Mat<4, 4, T>& m)
    {
        return m.data()->data();
    }

    using Mat4 = Mat<4, 4, float>;
    using Mat4f = Mat<4, 4, float>;
    using Mat4d = Mat<4, 4, double>;
    using Mat4i = Mat<4, 4, int>;
    using Mat4u32 = Mat<4, 4, uint32_t>;

    template<typename T>
    constexpr T Identity();

    template<>
    constexpr Mat4 Identity<Mat4>()
    {
        return Mat4{1.0f};
    }

    template<typename T>
    std::ostream& operator<<(std::ostream& o, Mat<4, 4, T> const& m)
    {
        // prints in row-major, because that's how most people debug matrices
        for (Mat4::size_type row = 0; row < 4; ++row) {
            std::string_view delim;
            for (Mat4::size_type col = 0; col < 4; ++col) {
                o << delim << m[col][row];
                delim = " ";
            }
            o << '\n';
        }
        return o;
    }

    template<typename T>
    std::string to_string(Mat<4, 4, T> const& m)
    {
        using std::to_string;

        // prints in row-major, because that's how most people debug matrices
        std::string rv;
        for (Mat4::size_type row = 0; row < 4; ++row) {
            std::string_view delim;
            for (Mat4::size_type col = 0; col < 4; ++col) {
                rv += delim;
                rv += to_string(m[col][row]);
                delim = " ";
            }
            rv += '\n';
        }
        return rv;
    }
}

template<>
struct std::hash<osc::Mat4> final {
    size_t operator()(osc::Mat4 const& v) const
    {
        return osc::HashRange(v);
    }
};
