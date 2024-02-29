#pragma once

#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Eulers.h>
#include <oscar/Maths/GeometricFunctions.h>
#include <oscar/Maths/MatFunctions.h>
#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/QuaternionFunctions.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>

#include <stdexcept>

namespace osc
{
    // returns a 3x3 transform matrix equivalent to the provided transform (ignores position)
    constexpr Mat3 mat3_cast(Transform const& t)
    {
        Mat3 rv = mat3_cast(t.rotation);

        rv[0][0] *= t.scale.x;
        rv[0][1] *= t.scale.x;
        rv[0][2] *= t.scale.x;

        rv[1][0] *= t.scale.y;
        rv[1][1] *= t.scale.y;
        rv[1][2] *= t.scale.y;

        rv[2][0] *= t.scale.z;
        rv[2][1] *= t.scale.z;
        rv[2][2] *= t.scale.z;

        return rv;
    }

    // returns a 4x4 transform matrix equivalent to the provided transform
    constexpr Mat4 mat4_cast(Transform const& t)
    {
        Mat4 rv = mat4_cast(t.rotation);

        rv[0][0] *= t.scale.x;
        rv[0][1] *= t.scale.x;
        rv[0][2] *= t.scale.x;

        rv[1][0] *= t.scale.y;
        rv[1][1] *= t.scale.y;
        rv[1][2] *= t.scale.y;

        rv[2][0] *= t.scale.z;
        rv[2][1] *= t.scale.z;
        rv[2][2] *= t.scale.z;

        rv[3][0] = t.position.x;
        rv[3][1] = t.position.y;
        rv[3][2] = t.position.z;

        return rv;
    }

    // returns a 4x4 transform matrix equivalent to the inverse of the provided transform
    inline Mat4 inverse_mat4_cast(Transform const& t)
    {
        Mat4 translater = translate(identity<Mat4>(), -t.position);
        Mat4 rotater = mat4_cast(conjugate(t.rotation));
        Mat4 scaler = scale(identity<Mat4>(), 1.0f/t.scale);

        return scaler * rotater * translater;
    }

    // returns a 3x3 normal matrix for the provided transform
    constexpr Mat3 normal_matrix(Transform const& t)
    {
        return adjugate(transpose(mat3_cast(t)));
    }

    // returns a 4x4 normal matrix for the provided transform
    constexpr Mat4 normal_matrix_4x4(Transform const& t)
    {
        return adjugate(transpose(mat3_cast(t)));
    }

    // returns a transform that *tries to* perform the equivalent transform as the provided mat4
    //
    // - not all 4x4 matrices can be expressed as an `Transform` (e.g. those containing skews)
    // - uses matrix decomposition to break up the provided matrix
    // - throws if decomposition of the provided matrix is not possible
    inline Transform decompose_to_transform(Mat4 const& m)
    {
        Transform rv;
        Vec3 skew;
        Vec4 perspective;
        if (!decompose(m, rv.scale, rv.rotation, rv.position, skew, perspective)) {
            throw std::runtime_error{"failed to decompose a matrix into scale, rotation, etc."};
        }
        return rv;
    }

    // returns a unit-length vector that is the equivalent of the provided direction vector after applying the transform
    //
    // effectively, apply the Transform but ignore the `position` (translation) component
    inline Vec3 transform_direction(Transform const& t, Vec3 const& localDir)
    {
        return normalize(t.rotation * (t.scale * localDir));
    }

    // returns a unit-length vector that is the equivalent of the provided direction vector after applying the inverse of the transform
    //
    // effectively, apply the inverse transform but ignore the `position` (translation) component
    inline Vec3 inverse_transform_direction(Transform const& t, Vec3 const& direction)
    {
        return normalize((conjugate(t.rotation) * direction) / t.scale);
    }

    // returns a vector that is the equivalent of the provided vector after applying the transform
    constexpr Vec3 transform_point(Transform const& t, Vec3 p)
    {
        return t * p;
    }

    // returns a vector that is the equivalent of the provided vector after applying the inverse of the transform
    constexpr Vec3 inverse_transform_point(Transform const& t, Vec3 p)
    {
        p -= t.position;
        p = conjugate(t.rotation) * p;
        p /= t.scale;
        return p;
    }

    // returns XYZ (pitch, yaw, roll) Euler angles for a one-by-one application of an
    // intrinsic rotations.
    //
    // Each rotation is applied one-at-a-time, to the transformed space, so we have:
    //
    // x-y-z (initial)
    // x'-y'-z' (after first rotation)
    // x''-y''-z'' (after second rotation)
    // x'''-y'''-z''' (after third rotation)
    //
    // Assuming we're doing an XYZ rotation, the first rotation rotates x, the second
    // rotation rotates around y', and the third rotation rotates around z''
    //
    // see: https://en.wikipedia.org/wiki/Euler_angles#Conventions_by_intrinsic_rotations
    inline Eulers extract_eulers_xyz(Transform const& t)
    {
        return extract_eulers_xyz(mat4_cast(t.rotation));
    }

    // returns XYZ (pitch, yaw, roll) Euler angles for an extrinsic rotation
    //
    // in extrinsic rotations, each rotation happens about a *fixed* coordinate system, which
    // is in contrast to intrinsic rotations, which happen in a coordinate system that's attached
    // to a moving body (the thing being rotated)
    //
    // see: https://en.wikipedia.org/wiki/Euler_angles#Conventions_by_extrinsic_rotations
    inline Eulers extract_extrinsic_eulers_xyz(Transform const& t)
    {
        return euler_angles(t.rotation);
    }

    // returns the provided transform, but rotated such that the given axis, as expressed
    // in the original transform, will instead point along the new direction
    inline Transform point_axis_along(Transform const& t, int axisIndex, Vec3 const& newDirection)
    {
        Vec3 beforeDir{};
        beforeDir[axisIndex] = 1.0f;
        beforeDir = t.rotation * beforeDir;

        Quat const rotBeforeToAfter = rotation(beforeDir, newDirection);
        Quat const newRotation = normalize(rotBeforeToAfter * t.rotation);

        return t.with_rotation(newRotation);
    }

    // returns the provided transform, but rotated such that the given axis, as expressed
    // in the original transform, will instead point towards the given point
    //
    // alternate explanation: "performs the shortest (angular) rotation of the given
    // transform such that the given axis points towards a point in the same space"
    inline Transform point_axis_towards(Transform const& t, int axisIndex, Vec3 const& location)
    {
        return point_axis_along(t, axisIndex, normalize(location - t.position));
    }

    // returns the provided transform, but intrinsically rotated along the given axis by
    // the given number of radians
    inline Transform rotate_axis(Transform const& t, int axisIndex, Radians angle)
    {
        Vec3 ax{};
        ax[axisIndex] = 1.0f;
        ax = t.rotation * ax;

        Quat const q = angle_axis(angle, ax);

        return t.with_rotation(normalize(q * t.rotation));
    }
}
