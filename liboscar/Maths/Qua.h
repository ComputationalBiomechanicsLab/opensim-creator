#pragma once

#include <liboscar/Maths/CommonFunctions.h>
#include <liboscar/Maths/EulerAngles.h>
#include <liboscar/Maths/GeometricFunctions.h>
#include <liboscar/Maths/TrigonometricFunctions.h>
#include <liboscar/Maths/Matrix.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Utils/HashHelpers.h>

#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

namespace osc
{
    // a 3D (4-element) quaternion: usually used to represent rotations
    template<typename T>
    struct Qua {
        using value_type = T;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using iterator = T*;
        using const_iterator = const T*;
        using type = Qua<T>;

        static constexpr Qua<T> wxyz(T w, T x, T y, T z) { return Qua<T>(w, x, y, z); }

        // default-constructs a `Qua` with an identity transform
        constexpr Qua() = default;

        // constructs a `Qua` with `w = s` and the xyz of `v`
        constexpr Qua(T s, const Vector<T, 3>& v) :
            w{s}, x{v.x}, y{v.y}, z{v.z}
        {}

        // constructs a `Qua` from its wxyz components
        constexpr Qua(T w_, T x_, T y_, T z_) :
            w{w_}, x{x_}, y{y_}, z{z_}
        {}

        // constructs a `Qua` by `static_cast<T>`ing each element in `q`
        template<typename U>
        explicit constexpr Qua(const Qua<U>& q) :
            w{static_cast<T>(q.w)},
            x{static_cast<T>(q.x)},
            y{static_cast<T>(q.y)},
            z{static_cast<T>(q.z)}
        {}

        /// constructs a `Qua` from two normalized axis
        ///
        /// @param u A first normalized axis
        /// @param v A second normalized axis
        /// @see gtc_Quaternion
        /// @see http://lolengine.net/blog/2013/09/18/beautiful-maths-Quaternion-from-Vectors
        Qua(const Vector<T, 3>& u, const Vector<T, 3>& v)
        {
            T norm_u_norm_v = sqrt(dot(u, u) * dot(v, v));
            T real_part = norm_u_norm_v + dot(u, v);
            Vector<T, 3> t;

            if(real_part < static_cast<T>(1.e-6f) * norm_u_norm_v) {
                // If u and v are exactly opposite, rotate 180 degrees
                // around an arbitrary orthogonal axis. Axis normalisation
                // can happen later, when we normalise the quaternion.
                real_part = static_cast<T>(0);
                t = abs(u.x) > abs(u.z) ? Vector<T, 3>(-u.y, u.x, static_cast<T>(0)) : Vector<T, 3>(static_cast<T>(0), -u.z, u.y);
            }
            else {
                // Otherwise, build quaternion the standard way.
                t = cross(u, v);
            }

            *this = normalize(Qua<T>::wxyz(real_part, t.x, t.y, t.z));
        }

        // constructs a `Qua` from Euler angles that are assumed to represent an
        // intrinsic, step-by-step, rotation about X, Y, and then Z
        explicit Qua(const EulerAngles& euler_angles)
        {
            Vector<T, 3> c = cos(euler_angles * T(0.5));
            Vector<T, 3> s = sin(euler_angles * T(0.5));

            this->w = c.x * c.y * c.z + s.x * s.y * s.z;
            this->x = s.x * c.y * c.z - c.x * s.y * s.z;
            this->y = c.x * s.y * c.z + s.x * c.y * s.z;
            this->z = c.x * c.y * s.z - s.x * s.y * c.z;
        }

        // constructs a `Qua` by decomposing an orthogonal matrix
        explicit Qua(const Matrix<T, 3, 3>& m)
        {
            *this = quaternion_cast(m);
        }

        // constructs a `Qua` by decomposing an orthogonal matrix
        explicit Qua(const Matrix<T, 4, 4>& m)
        {
            *this = quaternion_cast(m);
        }

        constexpr size_type size() const { return 4; }
        constexpr pointer data() { return &w; }
        constexpr const_pointer data() const { return &w; }
        constexpr iterator begin() { return data(); }
        constexpr const_iterator begin() const { return data(); }
        constexpr iterator end() { return data() + size(); }
        constexpr const_iterator end() const { return data() + size(); }
        constexpr reference operator[](size_type pos) { return begin()[pos]; }
        constexpr const_reference operator[](size_type pos) const { return begin()[pos]; }

        friend constexpr bool operator==(const Qua&, const Qua&) = default;

        template<typename U>
        constexpr Qua<T>& operator=(const Qua<U>& q)
        {
            this->w = static_cast<T>(q.w);
            this->x = static_cast<T>(q.x);
            this->y = static_cast<T>(q.y);
            this->z = static_cast<T>(q.z);
            return *this;
        }

        template<typename U>
        constexpr Qua<T>& operator+=(const Qua<U>& q)
        {
            this->w += static_cast<T>(q.w);
            this->x += static_cast<T>(q.x);
            this->y += static_cast<T>(q.y);
            this->z += static_cast<T>(q.z);
            return *this;
        }

        template<typename U>
        constexpr Qua<T>& operator-=(const Qua<U>& q)
        {
            this->w -= static_cast<T>(q.w);
            this->x -= static_cast<T>(q.x);
            this->y -= static_cast<T>(q.y);
            this->z -= static_cast<T>(q.z);
            return *this;
        }

        template<typename U>
        constexpr Qua<T>& operator*=(const Qua<U>& r)
        {
            const Qua<T> p(*this);
            const Qua<T> q(r);
            this->w = p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z;
            this->x = p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y;
            this->y = p.w * q.y + p.y * q.w + p.z * q.x - p.x * q.z;
            this->z = p.w * q.z + p.z * q.w + p.x * q.y - p.y * q.x;
            return *this;
        }

        template<typename U>
        constexpr Qua<T>& operator*=(U s)
        {
            this->w *= static_cast<T>(s);
            this->x *= static_cast<T>(s);
            this->y *= static_cast<T>(s);
            this->z *= static_cast<T>(s);
            return *this;
        }

        template<typename U>
        constexpr Qua<T>& operator/=(U s)
        {
            this->w /= static_cast<T>(s);
            this->x /= static_cast<T>(s);
            this->y /= static_cast<T>(s);
            this->z /= static_cast<T>(s);
            return *this;
        }

        T w = T(1);
        T x = T{};
        T y = T{};
        T z = T{};
    };

    template<typename T>
    constexpr Qua<T> operator+(const Qua<T>& q)
    {
        return q;
    }

    template<typename T>
    constexpr Qua<T> operator-(const Qua<T>& q)
    {
        return Qua<T>::wxyz(-q.w, -q.x, -q.y, -q.z);
    }

    template<typename T>
    constexpr Qua<T> operator+(const Qua<T>& q, const Qua<T>& p)
    {
        return Qua<T>(q) += p;
    }

    template<typename T>
    constexpr Qua<T> operator-(const Qua<T>& q, const Qua<T>& p)
    {
        return Qua<T>(q) -= p;
    }

    template<typename T>
    constexpr Qua<T> operator*(const Qua<T>& q, const Qua<T>& p)
    {
        return Qua<T>(q) *= p;
    }

    template<typename T>
    constexpr Vector<T, 3> operator*(const Qua<T>& q, const Vector<T, 3>& v)
    {
        const Vector<T, 3> qvector(q.x, q.y, q.z);
        const Vector<T, 3> uv(cross(qvector, v));
        const Vector<T, 3> uuv(cross(qvector, uv));

        return v + ((uv * q.w) + uuv) * static_cast<T>(2);
    }

    template<typename T>
    constexpr Vector<T, 3> operator*(const Vector<T, 3>& v, const Qua<T>& q)
    {
        return inverse(q) * v;
    }

    template<typename T>
    constexpr Vector<T, 4> operator*(const Qua<T>& q, const Vector<T, 4>& v)
    {
        return Vector<T, 4>(q * Vector<T, 3>(v), v.w);
    }

    template<typename T>
    constexpr Vector<T, 4> operator*(const Vector<T, 4>& v, const Qua<T>& q)
    {
        return inverse(q) * v;
    }

    template<typename T>
    constexpr Qua<T> operator*(const Qua<T>& q, const T& s)
    {
        return Qua<T>::wxyz(q.w * s, q.x * s, q.y * s, q.z * s);
    }

    template<typename T>
    constexpr Qua<T> operator*(const T& s, const Qua<T>& q)
    {
        return q * s;
    }

    template<typename T>
    constexpr Qua<T> operator/(const Qua<T>& q, const T& s)
    {
        return Qua<T>::wxyz(q.w / s, q.x / s, q.y / s, q.z / s);
    }

    template<typename T>
    std::ostream& operator<<(std::ostream& o, const Qua<T>& v)
    {
        return o << "Quaternion(" << v.w << ", " << v.x << ", " << v.y << ", " << v.z << ')';
    }

    template<typename T>
    std::string to_string(const Qua<T>& v)
    {
        std::stringstream ss;
        ss << v;
        return std::move(ss).str();
    }

    // when handled as a tuple-like object, a `Quaternion` decomposes into its elements

    template<size_t I, typename T>
    constexpr const T& get(const Qua<T>& v) { return v[I]; }

    template<size_t I, typename T>
    constexpr T& get(Qua<T>& v) { return v[I]; }

    template<size_t I, typename T>
    constexpr T&& get(Qua<T>&& v) { return std::move(v[I]); }

    template<size_t I, typename T>
    constexpr const T&& get(const Qua<T>&& v) { return std::move(v[I]); }
}

template<typename T>
struct std::tuple_size<osc::Qua<T>> {
    static inline constexpr size_t value = 4;
};

template<size_t I, typename T>
struct std::tuple_element<I, osc::Qua<T>> {
    using type = T;
};
