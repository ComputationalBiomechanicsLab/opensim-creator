#pragma once

#include <oscar/Maths/Angle.h>
#include <oscar/Maths/EulerAngles.h>
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
    constexpr Mat3 mat3_cast(const Transform& transform)
    {
        Mat3 rv = mat3_cast(transform.rotation);

        rv[0][0] *= transform.scale.x;
        rv[0][1] *= transform.scale.x;
        rv[0][2] *= transform.scale.x;

        rv[1][0] *= transform.scale.y;
        rv[1][1] *= transform.scale.y;
        rv[1][2] *= transform.scale.y;

        rv[2][0] *= transform.scale.z;
        rv[2][1] *= transform.scale.z;
        rv[2][2] *= transform.scale.z;

        return rv;
    }

    // returns a 4x4 transform matrix equivalent to the provided transform
    constexpr Mat4 mat4_cast(const Transform& transform)
    {
        Mat4 rv = mat4_cast(transform.rotation);

        rv[0][0] *= transform.scale.x;
        rv[0][1] *= transform.scale.x;
        rv[0][2] *= transform.scale.x;

        rv[1][0] *= transform.scale.y;
        rv[1][1] *= transform.scale.y;
        rv[1][2] *= transform.scale.y;

        rv[2][0] *= transform.scale.z;
        rv[2][1] *= transform.scale.z;
        rv[2][2] *= transform.scale.z;

        rv[3][0] = transform.position.x;
        rv[3][1] = transform.position.y;
        rv[3][2] = transform.position.z;

        return rv;
    }

    // returns a 4x4 transform matrix equivalent to the inverse of the provided transform
    inline Mat4 inverse_mat4_cast(const Transform& transform)
    {
        const Mat4 translator = translate(identity<Mat4>(), -transform.position);
        const Mat4 rotator = mat4_cast(conjugate(transform.rotation));
        const Mat4 scaler = scale(identity<Mat4>(), 1.0f/transform.scale);

        return scaler * rotator * translator;
    }

    // returns a 3x3 normal matrix for the provided transform
    constexpr Mat3 normal_matrix(const Transform& transform)
    {
        return adjugate(transpose(mat3_cast(transform)));
    }

    // returns a 4x4 normal matrix for the provided transform
    constexpr Mat4 normal_matrix_4x4(const Transform& transform)
    {
        return adjugate(transpose(mat3_cast(transform)));
    }

    // returns a transform that *tries to* perform the equivalent transform as the provided `Mat4`
    //
    // - not all 4x4 matrices can be expressed as an `Transform` (e.g. those containing skews)
    // - uses matrix decomposition to break up the provided matrix
    // - throws if decomposition of the provided matrix is not possible
    inline Transform decompose_to_transform(const Mat4& m)
    {
        Transform rv;
        Vec3 skew;
        Vec4 perspective;
        if (not decompose(m, rv.scale, rv.rotation, rv.position, skew, perspective)) {
            throw std::runtime_error{"failed to decompose a matrix into scale, rotation, etc."};
        }
        return rv;
    }

    // returns a unit-length vector that is the equivalent of the provided direction vector after applying the transform
    //
    // effectively, apply the Transform but ignore the `position` (translation) component
    inline Vec3 transform_direction(const Transform& transform, const Vec3& direction)
    {
        return normalize(transform.rotation * (transform.scale * direction));
    }

    // returns a unit-length vector that is the equivalent of the provided direction vector after applying the inverse of the transform
    //
    // effectively, apply the inverse transform but ignore the `position` (translation) component
    inline Vec3 inverse_transform_direction(const Transform& transform, const Vec3& direction)
    {
        return normalize((conjugate(transform.rotation) * direction) / transform.scale);
    }

    // returns a vector that is the equivalent of the provided vector after applying the transform
    constexpr Vec3 transform_point(const Transform& transform, Vec3 point)
    {
        return transform * point;
    }

    // returns a vector that is the equivalent of the provided vector after applying the inverse of the transform
    constexpr Vec3 inverse_transform_point(const Transform& transform, Vec3 point)
    {
        point -= transform.position;
        point = conjugate(transform.rotation) * point;
        point /= transform.scale;
        return point;
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
    inline EulerAngles extract_eulers_xyz(const Transform& transform)
    {
        return extract_eulers_xyz(mat4_cast(transform.rotation));
    }

    // returns XYZ (pitch, yaw, roll) Euler angles for an extrinsic rotation
    //
    // in extrinsic rotations, each rotation happens about a *fixed* coordinate system, which
    // is in contrast to intrinsic rotations, which happen in a coordinate system that's attached
    // to a moving body (the thing being rotated)
    //
    // see: https://en.wikipedia.org/wiki/Euler_angles#Conventions_by_extrinsic_rotations
    inline EulerAngles extract_extrinsic_eulers_xyz(const Transform& transform)
    {
        return to_euler_angles(transform.rotation);
    }

    // returns the provided transform, but rotated such that the given axis, as expressed
    // in the original transform, will instead point along the new direction
    inline Transform point_axis_along(
        const Transform& transform,
        Vec3::size_type axis_index,
        const Vec3& new_direction)
    {
        Vec3 old_direction{};
        old_direction[axis_index] = 1.0f;
        old_direction = transform.rotation * old_direction;

        const Quat rotation_old_to_new = rotation(old_direction, new_direction);
        const Quat new_rotation = normalize(rotation_old_to_new * transform.rotation);

        return transform.with_rotation(new_rotation);
    }

    // returns the provided transform, but rotated such that the given axis, as expressed
    // in the original transform, will instead point towards the given point
    //
    // alternate explanation: "performs the shortest (angular) rotation of the given
    // transform such that the given axis points towards a point in the same space"
    inline Transform point_axis_towards(
        const Transform& transform,
        Vec3::size_type axis_index,
        const Vec3& location)
    {
        return point_axis_along(transform, axis_index, normalize(location - transform.position));
    }

    // returns the provided transform, but intrinsically rotated along the given axis by
    // the given number of radians
    inline Transform rotate_axis(
        const Transform& transform,
        Vec3::size_type axis_index,
        Radians angle)
    {
        Vec3 ax{};
        ax[axis_index] = 1.0f;
        ax = transform.rotation * ax;

        const Quat q = angle_axis(angle, ax);

        return transform.with_rotation(normalize(q * transform.rotation));
    }
}
