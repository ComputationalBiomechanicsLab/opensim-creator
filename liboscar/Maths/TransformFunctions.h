#pragma once

#include <liboscar/Maths/Angle.h>
#include <liboscar/Maths/CommonFunctions.h>
#include <liboscar/Maths/EulerAngles.h>
#include <liboscar/Maths/Functors.h>
#include <liboscar/Maths/GeometricFunctions.h>
#include <liboscar/Maths/Matrix3x3.h>
#include <liboscar/Maths/Matrix4x4.h>
#include <liboscar/Maths/MatrixFunctions.h>
#include <liboscar/Maths/Quaternion.h>
#include <liboscar/Maths/QuaternionFunctions.h>
#include <liboscar/Maths/Transform.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Maths/Vector4.h>

#include <stdexcept>

namespace osc
{
    // returns a 3x3 transform matrix equivalent to the provided transform (ignores position)
    constexpr Matrix3x3 matrix3x3_cast(const Transform& transform)
    {
        Matrix3x3 rv = matrix3x3_cast(transform.rotation);

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
    constexpr Matrix4x4 matrix4x4_cast(const Transform& transform)
    {
        Matrix4x4 rv = matrix4x4_cast(transform.rotation);

        rv[0][0] *= transform.scale.x;
        rv[0][1] *= transform.scale.x;
        rv[0][2] *= transform.scale.x;

        rv[1][0] *= transform.scale.y;
        rv[1][1] *= transform.scale.y;
        rv[1][2] *= transform.scale.y;

        rv[2][0] *= transform.scale.z;
        rv[2][1] *= transform.scale.z;
        rv[2][2] *= transform.scale.z;

        rv[3][0] = transform.translation.x;
        rv[3][1] = transform.translation.y;
        rv[3][2] = transform.translation.z;

        return rv;
    }

    // returns a 4x4 transform matrix equivalent to the inverse of the provided transform
    inline Matrix4x4 inverse_matrix4x4_cast(const Transform& transform)
    {
        const Matrix4x4 translator = translate(identity<Matrix4x4>(), -transform.translation);
        const Matrix4x4 rotator = matrix4x4_cast(conjugate(transform.rotation));
        const Matrix4x4 scaler = scale(identity<Matrix4x4>(), 1.0f/transform.scale);

        return scaler * rotator * translator;
    }

    // returns a 3x3 normal matrix for the provided transform
    constexpr Matrix3x3 normal_matrix(const Transform& transform)
    {
        return adjugate(transpose(matrix3x3_cast(transform)));
    }

    // returns a 4x4 normal matrix for the provided transform
    constexpr Matrix4x4 normal_matrix_4x4(const Transform& transform)
    {
        return adjugate(transpose(matrix3x3_cast(transform)));
    }

    // returns `true` if `out` was updated with the decomposition of `m`
    //
    // - not all 4x4 matrices can be expressed as an `Transform` (e.g. those containing skews)
    // - uses matrix decomposition to break up the provided matrix
    inline bool try_decompose_to_transform(const Matrix4x4& m, Transform& out)
    {
        Vector3 skew;
        Vector4 perspective;
        return decompose(m, out.scale, out.rotation, out.translation, skew, perspective);
    }

    // returns a transform that *tries to* perform the equivalent transform as the provided `Matrix4x4`
    //
    // - not all 4x4 matrices can be expressed as an `Transform` (e.g. those containing skews)
    // - uses matrix decomposition to break up the provided matrix
    // - throws if decomposition of the provided matrix is not possible
    inline Transform decompose_to_transform(const Matrix4x4& m)
    {
        Transform rv;
        if (not try_decompose_to_transform(m, rv)) {
            throw std::runtime_error{"failed to decompose a matrix into scale, rotation, etc."};
        }
        return rv;
    }

    // returns a unit-length vector that is the equivalent of the provided direction vector after applying the transform
    //
    // effectively, apply the Transform but ignore the `position` (translation) component
    inline Vector3 transform_direction(const Transform& transform, const Vector3& direction)
    {
        return normalize(transform.rotation * (transform.scale * direction));
    }

    // returns a unit-length vector that is the equivalent of the provided direction vector after applying the inverse of the transform
    //
    // effectively, apply the inverse transform but ignore the `position` (translation) component
    inline Vector3 inverse_transform_direction(const Transform& transform, const Vector3& direction)
    {
        return normalize((conjugate(transform.rotation) * direction) / transform.scale);
    }

    // returns a vector that is the equivalent of the provided vector after applying the transform
    constexpr Vector3 transform_point(const Transform& transform, Vector3 point)
    {
        return transform * point;
    }

    // returns a vector that is the equivalent of the provided vector after applying the inverse of the transform
    constexpr Vector3 inverse_transform_point(const Transform& transform, Vector3 point)
    {
        point -= transform.translation;
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
        return extract_eulers_xyz(matrix4x4_cast(transform.rotation));
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
        Vector3::size_type axis_index,
        const Vector3& new_direction)
    {
        Vector3 old_direction{};
        old_direction[axis_index] = 1.0f;
        old_direction = transform.rotation * old_direction;

        const Quaternion rotation_old_to_new = rotation(old_direction, new_direction);
        const Quaternion new_rotation = normalize(rotation_old_to_new * transform.rotation);

        return transform.with_rotation(new_rotation);
    }

    // returns the provided transform, but rotated such that the given axis, as expressed
    // in the original transform, will instead point towards the given point
    //
    // alternate explanation: "performs the shortest (angular) rotation of the given
    // transform such that the given axis points towards a point in the same space"
    inline Transform point_axis_towards(
        const Transform& transform,
        Vector3::size_type axis_index,
        const Vector3& position)
    {
        return point_axis_along(transform, axis_index, normalize(position - transform.translation));
    }

    // returns the provided transform, but intrinsically rotated along the given axis by
    // the given number of radians
    inline Transform rotate_axis(
        const Transform& transform,
        Vector3::size_type axis_index,
        Radians angle)
    {
        Vector3 ax{};
        ax[axis_index] = 1.0f;
        ax = transform.rotation * ax;

        const Quaternion q = angle_axis(angle, ax);

        return transform.with_rotation(normalize(q * transform.rotation));
    }

    // returns `true` if any element in `transform`'s `scale`, `rotation`, or
    // `position` is NaN.
    inline bool any_element_is_nan(const Transform& transform)
    {
        return any_of(isnan(transform.scale)) or any_of(isnan(transform.rotation)) or any_of(isnan(transform.translation));
    }
}
