#pragma once

#include <oscar/Maths/Angle.h>
#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/Constants.h>
#include <oscar/Maths/CoordinateDirection.h>
#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Qua.h>
#include <oscar/Maths/TrigonometricFunctions.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>

#include <concepts>
#include <limits>
#include <type_traits>
#include <utility>

namespace osc
{
    template<typename T>
    requires std::is_arithmetic_v<T>
    constexpr Qua<T> conjugate(const Qua<T>& q)
    {
        return Qua<T>::wxyz(q.w, -q.x, -q.y, -q.z);
    }

    template<typename T>
    requires std::is_arithmetic_v<T>
    constexpr T dot(const Qua<T>& a, const Qua<T>& b)
    {
        return (a.w*b.w + a.x*b.x) + (a.y*b.y + a.z*b.z);
    }

    template<typename T>
    requires std::is_arithmetic_v<T>
    constexpr Qua<T> inverse(const Qua<T>& q)
    {
        return conjugate(q) / dot(q, q);
    }

    template<typename T>
    T length(const Qua<T>& q)
    {
        return sqrt(dot(q, q));
    }

    // returns a normalized version of the provided argument
    template<std::floating_point T>
    Qua<T> normalize(const Qua<T>& q)
    {
        const T len = length(q);

        if (len <= static_cast<T>(0)) {  // uh oh
            return Qua<T>::wxyz(
                static_cast<T>(1),
                static_cast<T>(0),
                static_cast<T>(0),
                static_cast<T>(0)
            );
        }

        const T one_over_len = static_cast<T>(1) / len;
        return Qua<T>::wxyz(q.w * one_over_len, q.x * one_over_len, q.y * one_over_len, q.z * one_over_len);
    }

    template<typename T>
    Qua<T> quat_cast(const Mat<3, 3, T>& m)
    {
        const T four_x_squared_minus_1 = m[0][0] - m[1][1] - m[2][2];
        const T four_y_squared_minus_1 = m[1][1] - m[0][0] - m[2][2];
        const T four_z_squared_minus_1 = m[2][2] - m[0][0] - m[1][1];
        const T four_w_squared_minus_1 = m[0][0] + m[1][1] + m[2][2];

        int biggest_index = 0;
        T four_biggest_squared_minus_1 = four_w_squared_minus_1;
        if (four_x_squared_minus_1 > four_biggest_squared_minus_1) {
            four_biggest_squared_minus_1 = four_x_squared_minus_1;
            biggest_index = 1;
        }
        if (four_y_squared_minus_1 > four_biggest_squared_minus_1) {
            four_biggest_squared_minus_1 = four_y_squared_minus_1;
            biggest_index = 2;
        }
        if (four_z_squared_minus_1 > four_biggest_squared_minus_1) {
            four_biggest_squared_minus_1 = four_z_squared_minus_1;
            biggest_index = 3;
        }

        const T biggest_val = sqrt(four_biggest_squared_minus_1 + static_cast<T>(1)) * static_cast<T>(0.5);
        const T mult = static_cast<T>(0.25) / biggest_val;

        switch(biggest_index) {
        default:
        case 0:
            return Qua<T>::wxyz(biggest_val, (m[1][2] - m[2][1]) * mult, (m[2][0] - m[0][2]) * mult, (m[0][1] - m[1][0]) * mult);
        case 1:
            return Qua<T>::wxyz((m[1][2] - m[2][1]) * mult, biggest_val, (m[0][1] + m[1][0]) * mult, (m[2][0] + m[0][2]) * mult);
        case 2:
            return Qua<T>::wxyz((m[2][0] - m[0][2]) * mult, (m[0][1] + m[1][0]) * mult, biggest_val, (m[1][2] + m[2][1]) * mult);
        case 3:
            return Qua<T>::wxyz((m[0][1] - m[1][0]) * mult, (m[2][0] + m[0][2]) * mult, (m[1][2] + m[2][1]) * mult, biggest_val);
        }
    }

    template<typename T>
    Qua<T> quat_cast(const Mat<4, 4, T>& m)
    {
        return quat_cast(Mat<3, 3, T>(m));
    }

    template<typename T>
    constexpr Mat<3, 3, T> mat3_cast(const Qua<T>& q)
    {
        const T qxx(q.x * q.x);
        const T qyy(q.y * q.y);
        const T qzz(q.z * q.z);
        const T qxz(q.x * q.z);
        const T qxy(q.x * q.y);
        const T qyz(q.y * q.z);
        const T qwx(q.w * q.x);
        const T qwy(q.w * q.y);
        const T qwz(q.w * q.z);

        Mat<3, 3, T> rv(T(1));

        rv[0][0] = T(1) - T(2) * (qyy +  qzz);
        rv[0][1] = T(2) * (qxy + qwz);
        rv[0][2] = T(2) * (qxz - qwy);

        rv[1][0] = T(2) * (qxy - qwz);
        rv[1][1] = T(1) - T(2) * (qxx +  qzz);
        rv[1][2] = T(2) * (qyz + qwx);

        rv[2][0] = T(2) * (qxz + qwy);
        rv[2][1] = T(2) * (qyz - qwx);
        rv[2][2] = T(1) - T(2) * (qxx +  qyy);

        return rv;
    }

    template<typename T>
    constexpr Mat<4, 4, T> mat4_cast(const Qua<T>& q)
    {
        return Mat<4, 4, T>(mat3_cast(q));
    }

    template<typename T>
    constexpr Qua<T> quat_identity()
    {
        return Qua<T>::wxyz(
            static_cast<T>(1),
            static_cast<T>(0),
            static_cast<T>(0),
            static_cast<T>(0)
        );
    }

    template<
        std::floating_point T,
        AngularUnitTraits Units,
        std::convertible_to<const Vec<3, T>&> Veclike
    >
    Qua<T> angle_axis(Angle<T, Units> angle, Veclike&& axis)
    {
        const T s = sin(angle * static_cast<T>(0.5));
        return Qua<T>(cos(angle * static_cast<T>(0.5)), static_cast<const Vec<3, T>&>(axis) * s);
    }

    template<
        std::floating_point T,
        AngularUnitTraits Units
    >
    Qua<T> angle_axis(Angle<T, Units> angle, CoordinateDirection direction)
    {
        return angle_axis(angle, direction.vec<T>());
    }

    // computes the rotation from `origin` to `destination`
    template<typename T>
    Qua<T> rotation(const Vec<3, T>& origin, const Vec<3, T>& destination)
    {
        const T cos_theta = dot(origin, destination);
        if (cos_theta >= static_cast<T>(1) - epsilon_v<T>) {
            // origin and destination point in the same direction
            return quat_identity<T>();
        }

        Vec<3, T> rotation_axis;
        if (cos_theta < static_cast<T>(-1) + epsilon_v<T>) {
            // special case when vectors in opposite directions :
            // there is no "ideal" rotation axis
            // So guess one; any will do as long as it's perpendicular to start
            // This implementation favors a rotation around the Up axis (Y),
            // since it's often what you want to do.
            rotation_axis = cross(Vec<3, T>(0, 0, 1), origin);
            if (length2(rotation_axis) < epsilon_v<T>) { // bad luck, they were parallel, try again!
                rotation_axis = cross(Vec<3, T>(1, 0, 0), origin);
            }

            rotation_axis = normalize(rotation_axis);
            return angle_axis(Degrees{180}, rotation_axis);
        }

        // Implementation from Stan Melax's Game Programming Gems 1 article
        rotation_axis = cross(origin, destination);

        const T s = sqrt((T(1) + cos_theta) * static_cast<T>(2));
        const T invs = static_cast<T>(1) / s;

        return Qua<T>::wxyz(
            s * static_cast<T>(0.5f),
            rotation_axis.x * invs,
            rotation_axis.y * invs,
            rotation_axis.z * invs
        );
    }

    template<typename T>
    RadiansT<T> pitch(const Qua<T>& q)
    {
        const T y = static_cast<T>(2) * (q.y * q.z + q.w * q.x);
        const T x = q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z;

        if (all_of(equal_within_epsilon(Vec<2, T>(x, y), Vec<2, T>(0)))) {
            //avoid atan2(0,0) - handle singularity - Matiis
            return static_cast<T>(2) * atan2(q.x, q.w);
        }

        return atan2(y, x);
    }

    template<typename T>
    RadiansT<T> yaw(const Qua<T>& q)
    {
        return asin(clamp(static_cast<T>(-2) * (q.x * q.z - q.w * q.y), static_cast<T>(-1), static_cast<T>(1)));
    }

    template<typename T>
    RadiansT<T> roll(const Qua<T>& q)
    {
        const T y = static_cast<T>(2) * (q.x * q.y + q.w * q.z);
        const T x = q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z;

        if (all_of(equal_within_epsilon(Vec<2, T>(x, y), Vec<2, T>(0)))) {
            //avoid atan2(0,0) - handle singularity - Matiis
            return RadiansT<T>{0};
        }

        return atan2(y, x);
    }

    template<typename T>
    Vec<3, RadiansT<T>> to_euler_angles(const Qua<T>& x)
    {
        return Vec<3, RadiansT<T>>(pitch(x), yaw(x), roll(x));
    }
}
