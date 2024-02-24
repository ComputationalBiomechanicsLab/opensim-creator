#pragma once

namespace osc { template<typename> struct Qua; }

#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Constants.h>
#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/TrigonometricFunctions.h>

#include <concepts>
#include <limits>
#include <type_traits>
#include <utility>

namespace osc
{
    template<typename T>
    constexpr Qua<T> conjugate(Qua<T> const& q)
        requires std::is_arithmetic_v<T>
    {
        return Qua<T>::wxyz(q.w, -q.x, -q.y, -q.z);
    }

    template<typename T>
    constexpr T dot(Qua<T> const& a, Qua<T> const& b)
        requires std::is_arithmetic_v<T>
    {
        return (a.w*b.w + a.x*b.x) + (a.y*b.y + a.z*b.z);
    }

    template<typename T>
    constexpr Qua<T> inverse(Qua<T> const& q)
        requires std::is_arithmetic_v<T>
    {
        return conjugate(q) / dot(q, q);
    }

    template<typename T>
    T length(Qua<T> const& q)
    {
        return sqrt(dot(q, q));
    }

    // returns a normalized version of the provided argument
    template<std::floating_point T>
    Qua<T> normalize(Qua<T> const& q)
    {
        T len = length(q);

        if(len <= static_cast<T>(0)) {  // uh oh
            return Qua<T>::wxyz(
                static_cast<T>(1),
                static_cast<T>(0),
                static_cast<T>(0),
                static_cast<T>(0)
            );
        }

        T oneOverLen = static_cast<T>(1) / len;
        return Qua<T>::wxyz(q.w * oneOverLen, q.x * oneOverLen, q.y * oneOverLen, q.z * oneOverLen);
    }

    template<typename T>
    Qua<T> quat_cast(Mat<3, 3, T> const& m)
    {
        T fourXSquaredMinus1 = m[0][0] - m[1][1] - m[2][2];
        T fourYSquaredMinus1 = m[1][1] - m[0][0] - m[2][2];
        T fourZSquaredMinus1 = m[2][2] - m[0][0] - m[1][1];
        T fourWSquaredMinus1 = m[0][0] + m[1][1] + m[2][2];

        int biggestIndex = 0;
        T fourBiggestSquaredMinus1 = fourWSquaredMinus1;
        if(fourXSquaredMinus1 > fourBiggestSquaredMinus1)
        {
            fourBiggestSquaredMinus1 = fourXSquaredMinus1;
            biggestIndex = 1;
        }
        if(fourYSquaredMinus1 > fourBiggestSquaredMinus1)
        {
            fourBiggestSquaredMinus1 = fourYSquaredMinus1;
            biggestIndex = 2;
        }
        if(fourZSquaredMinus1 > fourBiggestSquaredMinus1)
        {
            fourBiggestSquaredMinus1 = fourZSquaredMinus1;
            biggestIndex = 3;
        }

        T biggestVal = sqrt(fourBiggestSquaredMinus1 + static_cast<T>(1)) * static_cast<T>(0.5);
        T mult = static_cast<T>(0.25) / biggestVal;

        switch(biggestIndex) {
        default:
        case 0:
            return Qua<T>::wxyz(biggestVal, (m[1][2] - m[2][1]) * mult, (m[2][0] - m[0][2]) * mult, (m[0][1] - m[1][0]) * mult);
        case 1:
            return Qua<T>::wxyz((m[1][2] - m[2][1]) * mult, biggestVal, (m[0][1] + m[1][0]) * mult, (m[2][0] + m[0][2]) * mult);
        case 2:
            return Qua<T>::wxyz((m[2][0] - m[0][2]) * mult, (m[0][1] + m[1][0]) * mult, biggestVal, (m[1][2] + m[2][1]) * mult);
        case 3:
            return Qua<T>::wxyz((m[0][1] - m[1][0]) * mult, (m[2][0] + m[0][2]) * mult, (m[1][2] + m[2][1]) * mult, biggestVal);
        }
    }

    template<typename T>
    Qua<T> quat_cast(Mat<4, 4, T> const& m)
    {
        return quat_cast(Mat<3, 3, T>(m));
    }

    template<typename T>
    Mat<3, 3, T> mat3_cast(Qua<T> const& q)
    {
        Mat<3, 3, T> Result(T(1));
        T qxx(q.x * q.x);
        T qyy(q.y * q.y);
        T qzz(q.z * q.z);
        T qxz(q.x * q.z);
        T qxy(q.x * q.y);
        T qyz(q.y * q.z);
        T qwx(q.w * q.x);
        T qwy(q.w * q.y);
        T qwz(q.w * q.z);

        Result[0][0] = T(1) - T(2) * (qyy +  qzz);
        Result[0][1] = T(2) * (qxy + qwz);
        Result[0][2] = T(2) * (qxz - qwy);

        Result[1][0] = T(2) * (qxy - qwz);
        Result[1][1] = T(1) - T(2) * (qxx +  qzz);
        Result[1][2] = T(2) * (qyz + qwx);

        Result[2][0] = T(2) * (qxz + qwy);
        Result[2][1] = T(2) * (qyz - qwx);
        Result[2][2] = T(1) - T(2) * (qxx +  qyy);
        return Result;
    }

    template<typename T>
    Mat<4, 4, T> mat4_cast(Qua<T> const& q)
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
        std::convertible_to<Vec<3, T> const&> Veclike>
    Qua<T> angle_axis(Angle<T, Units> angle, Veclike&& axis)
    {
        T const s = sin(angle * static_cast<T>(0.5));
        return Qua<T>(cos(angle * static_cast<T>(0.5)), static_cast<Vec<3, T> const&>(axis) * s);
    }

    // computes the rotation from `src` to `dest`
    template<typename T>
    Qua<T> rotation(Vec<3, T> const& orig, Vec<3, T> const& dest)
    {
        T cosTheta = dot(orig, dest);
        Vec<3, T> rotationAxis;

        if(cosTheta >= static_cast<T>(1) - epsilon_v<T>) {
            // orig and dest point in the same direction
            return quat_identity<T>();
        }

        if(cosTheta < static_cast<T>(-1) + epsilon_v<T>) {
            // special case when vectors in opposite directions :
            // there is no "ideal" rotation axis
            // So guess one; any will do as long as it's perpendicular to start
            // This implementation favors a rotation around the Up axis (Y),
            // since it's often what you want to do.
            rotationAxis = cross(Vec<3, T>(0, 0, 1), orig);
            if(length2(rotationAxis) < epsilon_v<T>) { // bad luck, they were parallel, try again!
                rotationAxis = cross(Vec<3, T>(1, 0, 0), orig);
            }

            rotationAxis = normalize(rotationAxis);
            return angle_axis(Degrees{180}, rotationAxis);
        }

        // Implementation from Stan Melax's Game Programming Gems 1 article
        rotationAxis = cross(orig, dest);

        T s = sqrt((T(1) + cosTheta) * static_cast<T>(2));
        T invs = static_cast<T>(1) / s;

        return Qua<T>::wxyz(
            s * static_cast<T>(0.5f),
            rotationAxis.x * invs,
            rotationAxis.y * invs,
            rotationAxis.z * invs
        );
    }

    template<typename T>
    RadiansT<T> pitch(Qua<T> const& q)
    {
        //return T(atan(T(2) * (q.y * q.z + q.w * q.x), q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z));
        T const y = static_cast<T>(2) * (q.y * q.z + q.w * q.x);
        T const x = q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z;

        if (all_of(equal_within_epsilon(Vec<2, T>(x, y), Vec<2, T>(0)))) {
            //avoid atan2(0,0) - handle singularity - Matiis
            return static_cast<T>(2) * atan2(q.x, q.w);
        }

        return atan2(y, x);
    }

    template<typename T>
    RadiansT<T> yaw(Qua<T> const& q)
    {
        return asin(clamp(static_cast<T>(-2) * (q.x * q.z - q.w * q.y), static_cast<T>(-1), static_cast<T>(1)));
    }

    template<typename T>
    RadiansT<T> roll(Qua<T> const& q)
    {
        T const y = static_cast<T>(2) * (q.x * q.y + q.w * q.z);
        T const x = q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z;

        if (all_of(equal_within_epsilon(Vec<2, T>(x, y), Vec<2, T>(0)))) {
            //avoid atan2(0,0) - handle singularity - Matiis
            return RadiansT<T>{0};
        }

        return atan2(y, x);
    }

    template<typename T>
    Vec<3, RadiansT<T>> euler_angles(Qua<T> const& x)
    {
        return Vec<3, RadiansT<T>>(pitch(x), yaw(x), roll(x));
    }
}
