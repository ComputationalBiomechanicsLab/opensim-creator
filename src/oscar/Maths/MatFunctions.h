#pragma once

#include <oscar/Maths/Angle.h>
#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/Constants.h>
#include <oscar/Maths/Mat.h>
#include <oscar/Maths/Qua.h>
#include <oscar/Maths/UnitVec3.h>
#include <oscar/Maths/Vec.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/HashHelpers.h>

#include <concepts>
#include <cstddef>
#include <functional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace osc
{
    template<std::floating_point T>
    Mat<4, 4, T> look_at(
        const Vec<3, T>& eye,
        const Vec<3, T>& center,
        const Vec<3, T>& up)
    {
        const Vec<3, T> f(normalize(center - eye));
        const Vec<3, T> s(normalize(cross(f, up)));
        const Vec<3, T> u(cross(s, f));

        Mat<4, 4, T> rv(1);
        rv[0][0] =  s.x;
        rv[1][0] =  s.y;
        rv[2][0] =  s.z;
        rv[0][1] =  u.x;
        rv[1][1] =  u.y;
        rv[2][1] =  u.z;
        rv[0][2] = -f.x;
        rv[1][2] = -f.y;
        rv[2][2] = -f.z;
        rv[3][0] = -dot(s, eye);
        rv[3][1] = -dot(u, eye);
        rv[3][2] =  dot(f, eye);
        return rv;
    }

    template<std::floating_point T, AngularUnitTraits Units>
    Mat<4, 4, T> perspective(Angle<T, Units> vertical_fov, T aspect, T z_near, T z_far)
    {
        if (fabs(aspect - epsilon_v<T>) <= T{}) {
            // edge-case: some UIs ask for a perspective matrix on first frame before
            // aspect ratio is known or the aspect ratio is NaN because of a division
            // by zero
            return Mat<4, 4, T>{T(1)};
        }

        const T tan_half_fovy = tan(vertical_fov / static_cast<T>(2));

        Mat<4, 4, T> rv(static_cast<T>(0));
        rv[0][0] = static_cast<T>(1) / (aspect * tan_half_fovy);
        rv[1][1] = static_cast<T>(1) / (tan_half_fovy);
        rv[2][2] = - (z_far + z_near) / (z_far - z_near);
        rv[2][3] = - static_cast<T>(1);
        rv[3][2] = - (static_cast<T>(2) * z_far * z_near) / (z_far - z_near);
        return rv;
    }

    template<std::floating_point T>
    Mat<4, 4, T> ortho(T left, T right, T bottom, T top, T z_near, T z_far)
    {
        Mat<4, 4, T> rv(1);
        rv[0][0] = static_cast<T>(2) / (right - left);
        rv[1][1] = static_cast<T>(2) / (top - bottom);
        rv[2][2] = - static_cast<T>(2) / (z_far - z_near);
        rv[3][0] = - (right + left) / (right - left);
        rv[3][1] = - (top + bottom) / (top - bottom);
        rv[3][2] = - (z_far + z_near) / (z_far - z_near);
        return rv;
    }

    template<typename T>
    Mat<4, 4, T> scale(const Mat<4, 4, T>& m, const Vec<3, T>& v)
    {
        Mat<4, 4, T> rv;
        rv[0] = m[0] * v[0];
        rv[1] = m[1] * v[1];
        rv[2] = m[2] * v[2];
        rv[3] = m[3];
        return rv;
    }

    template<std::floating_point T, AngularUnitTraits Units>
    Mat<4, 4, T> rotate(const Mat<4, 4, T>& m, Angle<T, Units> angle, UnitVec<3, T> axis)
    {
        const T c = cos(angle);
        const T s = sin(angle);

        const Vec<3, T> temp((T(1) - c) * axis);

        Mat<4, 4, T> rotate;
        rotate[0][0] = c + temp[0] * axis[0];
        rotate[0][1] = temp[0] * axis[1] + s * axis[2];
        rotate[0][2] = temp[0] * axis[2] - s * axis[1];

        rotate[1][0] = temp[1] * axis[0] - s * axis[2];
        rotate[1][1] = c + temp[1] * axis[1];
        rotate[1][2] = temp[1] * axis[2] + s * axis[0];

        rotate[2][0] = temp[2] * axis[0] + s * axis[1];
        rotate[2][1] = temp[2] * axis[1] - s * axis[0];
        rotate[2][2] = c + temp[2] * axis[2];

        Mat<4, 4, T> rv;
        rv[0] = m[0] * rotate[0][0] + m[1] * rotate[0][1] + m[2] * rotate[0][2];
        rv[1] = m[0] * rotate[1][0] + m[1] * rotate[1][1] + m[2] * rotate[1][2];
        rv[2] = m[0] * rotate[2][0] + m[1] * rotate[2][1] + m[2] * rotate[2][2];
        rv[3] = m[3];
        return rv;
    }

    template<std::floating_point T, AngularUnitTraits Units>
    Mat<4, 4, T> rotate(const Mat<4, 4, T>& m, Angle<T, Units> angle, const Vec<3, T>& axis)
    {
        return rotate(m, angle, UnitVec<3, T>{axis});
    }

    template<typename T>
    Mat<4, 4, T> translate(const Mat<4, 4, T>& m, const Vec<3, T>& v)
    {
        Mat<4, 4, T> rv(m);
        rv[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
        return rv;
    }

    template<std::floating_point T>
    T determinant(const Mat<3, 3, T>& m)
    {
        return
            + m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2])
            - m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2])
            + m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]
        );
    }

    template<std::floating_point T>
    T determinant(const Mat<4, 4, T>& m)
    {
        const T subfactor_00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
        const T subfactor_01 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
        const T subfactor_02 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
        const T subfactor_03 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
        const T subfactor_04 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
        const T subfactor_05 = m[2][0] * m[3][1] - m[3][0] * m[2][1];

        const Vec<4, T> determinant_coef(
            + (m[1][1] * subfactor_00 - m[1][2] * subfactor_01 + m[1][3] * subfactor_02),
            - (m[1][0] * subfactor_00 - m[1][2] * subfactor_03 + m[1][3] * subfactor_04),
            + (m[1][0] * subfactor_01 - m[1][1] * subfactor_03 + m[1][3] * subfactor_05),
            - (m[1][0] * subfactor_02 - m[1][1] * subfactor_04 + m[1][2] * subfactor_05)
        );

        return
            m[0][0] * determinant_coef[0] + m[0][1] * determinant_coef[1] +
            m[0][2] * determinant_coef[2] + m[0][3] * determinant_coef[3];
    }

    template<std::floating_point T>
    T inverse(const Mat<3, 3, T>& m)
    {
        const T one_over_determinant = static_cast<T>(1) / determinant(m);

        Mat<3, 3, T> rv;
        rv[0][0] = + (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * one_over_determinant;
        rv[1][0] = - (m[1][0] * m[2][2] - m[2][0] * m[1][2]) * one_over_determinant;
        rv[2][0] = + (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * one_over_determinant;
        rv[0][1] = - (m[0][1] * m[2][2] - m[2][1] * m[0][2]) * one_over_determinant;
        rv[1][1] = + (m[0][0] * m[2][2] - m[2][0] * m[0][2]) * one_over_determinant;
        rv[2][1] = - (m[0][0] * m[2][1] - m[2][0] * m[0][1]) * one_over_determinant;
        rv[0][2] = + (m[0][1] * m[1][2] - m[1][1] * m[0][2]) * one_over_determinant;
        rv[1][2] = - (m[0][0] * m[1][2] - m[1][0] * m[0][2]) * one_over_determinant;
        rv[2][2] = + (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * one_over_determinant;
        return rv;
    }

    template<std::floating_point T>
    Mat<4, 4, T> inverse(const Mat<4, 4, T>& m)
    {
        const T coef_00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
        const T coef_02 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
        const T coef_03 = m[1][2] * m[2][3] - m[2][2] * m[1][3];

        const T coef_04 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
        const T coef_06 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
        const T coef_07 = m[1][1] * m[2][3] - m[2][1] * m[1][3];

        const T coef_08 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
        const T coef_10 = m[1][1] * m[3][2] - m[3][1] * m[1][2];
        const T coef_11 = m[1][1] * m[2][2] - m[2][1] * m[1][2];

        const T coef_12 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
        const T coef_14 = m[1][0] * m[3][3] - m[3][0] * m[1][3];
        const T coef_15 = m[1][0] * m[2][3] - m[2][0] * m[1][3];

        const T coef_16 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
        const T coef_18 = m[1][0] * m[3][2] - m[3][0] * m[1][2];
        const T coef_19 = m[1][0] * m[2][2] - m[2][0] * m[1][2];

        const T coef_20 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
        const T coef_22 = m[1][0] * m[3][1] - m[3][0] * m[1][1];
        const T coef_23 = m[1][0] * m[2][1] - m[2][0] * m[1][1];

        const Vec<4, T> fac_0(coef_00, coef_00, coef_02, coef_03);
        const Vec<4, T> fac_1(coef_04, coef_04, coef_06, coef_07);
        const Vec<4, T> fac_2(coef_08, coef_08, coef_10, coef_11);
        const Vec<4, T> fac_3(coef_12, coef_12, coef_14, coef_15);
        const Vec<4, T> fac_4(coef_16, coef_16, coef_18, coef_19);
        const Vec<4, T> fac_5(coef_20, coef_20, coef_22, coef_23);

        const Vec<4, T> vec_0(m[1][0], m[0][0], m[0][0], m[0][0]);
        const Vec<4, T> vec_1(m[1][1], m[0][1], m[0][1], m[0][1]);
        const Vec<4, T> vec_2(m[1][2], m[0][2], m[0][2], m[0][2]);
        const Vec<4, T> vec_3(m[1][3], m[0][3], m[0][3], m[0][3]);

        const Vec<4, T> inv_0(vec_1 * fac_0 - vec_2 * fac_1 + vec_3 * fac_2);
        const Vec<4, T> inv_1(vec_0 * fac_0 - vec_2 * fac_3 + vec_3 * fac_4);
        const Vec<4, T> inv_2(vec_0 * fac_1 - vec_1 * fac_3 + vec_3 * fac_5);
        const Vec<4, T> inv_3(vec_0 * fac_2 - vec_1 * fac_4 + vec_2 * fac_5);

        const Vec<4, T> sign_a(+1, -1, +1, -1);
        const Vec<4, T> sign_b(-1, +1, -1, +1);
        const Mat<4, 4, T> inverted(inv_0 * sign_a, inv_1 * sign_b, inv_2 * sign_a, inv_3 * sign_b);

        const Vec<4, T> row_0(inverted[0][0], inverted[1][0], inverted[2][0], inverted[3][0]);

        const Vec<4, T> dot_0(m[0] * row_0);
        const T dot_1 = (dot_0.x + dot_0.y) + (dot_0.z + dot_0.w);

        const T one_over_determinant = static_cast<T>(1) / dot_1;

        return inverted * one_over_determinant;
    }

    template<typename T>
    constexpr Mat<3, 3, T> transpose(const Mat<3, 3, T>& m)
    {
        Mat<3, 3, T> rv;
        rv[0][0] = m[0][0];
        rv[0][1] = m[1][0];
        rv[0][2] = m[2][0];

        rv[1][0] = m[0][1];
        rv[1][1] = m[1][1];
        rv[1][2] = m[2][1];

        rv[2][0] = m[0][2];
        rv[2][1] = m[1][2];
        rv[2][2] = m[2][2];
        return rv;
    }

    template<typename T>
    Mat<4, 4, T> transpose(const Mat<4, 4, T>& m)
    {
        Mat<4, 4, T> rv;
        rv[0][0] = m[0][0];
        rv[0][1] = m[1][0];
        rv[0][2] = m[2][0];
        rv[0][3] = m[3][0];

        rv[1][0] = m[0][1];
        rv[1][1] = m[1][1];
        rv[1][2] = m[2][1];
        rv[1][3] = m[3][1];

        rv[2][0] = m[0][2];
        rv[2][1] = m[1][2];
        rv[2][2] = m[2][2];
        rv[2][3] = m[3][2];

        rv[3][0] = m[0][3];
        rv[3][1] = m[1][3];
        rv[3][2] = m[2][3];
        rv[3][3] = m[3][3];
        return rv;
    }

    // returns euler angles for performing an intrinsic, step-by-step, rotation about X, Y, and then Z
    template<std::floating_point T>
    Vec<3, RadiansT<T>> extract_eulers_xyz(const Mat<4, 4, T>& m)
    {
        const RadiansT<T> t1 = atan2(m[2][1], m[2][2]);
        const T c2 = sqrt(m[0][0]*m[0][0] + m[1][0]*m[1][0]);
        const RadiansT<T> t2 = atan2(-m[2][0], c2);
        const T s1 = sin(t1);
        const T c1 = cos(t1);
        const RadiansT<T> t3 = atan2(s1*m[0][2] - c1*m[0][1], c1*m[1][1] - s1*m[1][2  ]);

        return Vec<3, RadiansT<T>>{-t1, -t2, -t3};
    }

    namespace detail
    {
        /// Make a linear combination of two vectors and return the result.
        template<typename T>
        Vec<3, T> combine(const Vec<3, T>& a, const Vec<3, T>& b, T ascl, T bscl)
        {
            return (a * ascl) + (b * bscl);
        }

        template<typename T>
        Vec<3, T> scale(const Vec<3, T>& v, T desiredLength)
        {
            return v * desiredLength / length(v);
        }
    }

    template<typename T>
    bool decompose(
        const Mat<4, 4, T>& model_mat4,
        Vec<3, T>& r_scale,
        Qua<T>& r_orientation,
        Vec<3, T>& r_translation,
        Vec<3, T>& r_skew,
        Vec<4, T>& r_perspective)
    {
        // Matrix decompose
        // http://www.opensource.apple.com/source/WebCore/WebCore-514/platform/graphics/transforms/TransformationMatrix.cpp
        // Decomposes the mode matrix to translations,rotation scale components

        Mat<4, 4, T> local_matrix(model_mat4);

        // normalize the matrix
        if (equal_within_epsilon(local_matrix[3][3], static_cast<T>(0))) {
            return false;
        }

        for (size_t i = 0; i < 4; ++i) {
            for (size_t j = 0; j < 4; ++j) {
                local_matrix[i][j] /= local_matrix[3][3];
            }
        }

        // `perspective_matrix` is used to solve for perspective, but it also provides
        // an easy way to test for singularity of the upper 3x3 component
        Mat<4, 4, T> perspective_matrix(local_matrix);
        for (size_t i = 0; i < 3; i++) {
            perspective_matrix[i][3] = static_cast<T>(0);
        }
        perspective_matrix[3][3] = static_cast<T>(1);

        if (equal_within_epsilon(determinant(perspective_matrix), static_cast<T>(0))) {
            return false;
        }

        // first, isolate perspective, which is the messiest
        if (
            not equal_within_epsilon(local_matrix[0][3], static_cast<T>(0)) or
            not equal_within_epsilon(local_matrix[1][3], static_cast<T>(0)) or
            not equal_within_epsilon(local_matrix[2][3], static_cast<T>(0)))
        {
            // `right_hand_side` is the right hand side of the equation
            const Vec<4, T> right_hand_side = {
                local_matrix[0][3],
                local_matrix[1][3],
                local_matrix[2][3],
                local_matrix[3][3],
            };

            // solve the equation by inverting `perspective_matrix` and multiplying
            // `right_hand_side` by the inverse. This is the easiest way, not
            // necessarily the best.
            const Mat<4, 4, T> inverse_perspective_matrix = inverse(perspective_matrix);
            const Mat<4, 4, T> transposed_inverse_perspective_matrix = transpose(inverse_perspective_matrix);//   transposeMatrix4(inversePerspectiveMatrix, transposedInversePerspectiveMatrix);

            r_perspective = transposed_inverse_perspective_matrix * right_hand_side;

            // clear the perspective partition
            local_matrix[0][3] = local_matrix[1][3] = local_matrix[2][3] = static_cast<T>(0);
            local_matrix[3][3] = static_cast<T>(1);
        }
        else {
            // no perspective
            r_perspective = Vec<4, T>(0, 0, 0, 1);
        }

        // second, take care of translation (easy).
        r_translation = Vec<3, T>(local_matrix[3]);
        local_matrix[3] = Vec<4, T>(0, 0, 0, local_matrix[3].w);

        // third/fourth, calculate the scale and shear
        Vec<3, T> Row[3];
        Vec<3, T> Pdum3;
        for (size_t i = 0; i < 3; ++i) {
            for(size_t j = 0; j < 3; ++j) {
                Row[i][j] = local_matrix[i][j];
            }
        }

        // compute X scale factor and normalize first row
        r_scale.x = length(Row[0]);

        Row[0] = detail::scale(Row[0], static_cast<T>(1));

        // compute XY shear factor and make 2nd row orthogonal to 1st
        r_skew.z = dot(Row[0], Row[1]);
        Row[1] = detail::combine(Row[1], Row[0], static_cast<T>(1), -r_skew.z);

        // compute Y scale and normalize 2nd row
        r_scale.y = length(Row[1]);
        Row[1] = detail::scale(Row[1], static_cast<T>(1));
        r_skew.z /= r_scale.y;

        // compute XZ and YZ shears, orthogonalize 3rd row
        r_skew.y = dot(Row[0], Row[2]);
        Row[2] = detail::combine(Row[2], Row[0], static_cast<T>(1), -r_skew.y);
        r_skew.x = dot(Row[1], Row[2]);
        Row[2] = detail::combine(Row[2], Row[1], static_cast<T>(1), -r_skew.x);

        // get Z scale and normalize 3rd row
        r_scale.z = length(Row[2]);
        Row[2] = detail::scale(Row[2], static_cast<T>(1));
        r_skew.y /= r_scale.z;
        r_skew.x /= r_scale.z;

        // at this point, the matrix (in rows[]) is orthonormal
        //
        // check for a coordinate system flip.  If the determinant
        // is -1, then negate the matrix and the scaling factors.
        Pdum3 = cross(Row[1], Row[2]);
        if (dot(Row[0], Pdum3) < 0) {
            for (size_t i = 0; i < 3; i++) {
                r_scale[i] *= static_cast<T>(-1);
                Row[i] *= static_cast<T>(-1);
            }
        }

        // fifth (finally), get the rotations out, as described in the gem

        // FIXME - Add the ability to return either quaternions (which are
        // easier to recompose with) or Euler angles (rx, ry, rz), which
        // are easier for authors to deal with. The latter will only be useful
        // when we fix https://bugs.webkit.org/show_bug.cgi?id=23799, so I
        // will leave the Euler angle code here for now.

        // ret.rotateY = asin(-Row[0][2]);
        // if (cos(ret.rotateY) != 0) {
        //     ret.rotateX = atan2(Row[1][2], Row[2][2]);
        //     ret.rotateZ = atan2(Row[0][1], Row[0][0]);
        // } else {
        //     ret.rotateX = atan2(-Row[2][0], Row[1][1]);
        //     ret.rotateZ = 0;
        // }

        int i, j, k = 0;
        T root, trace = Row[0].x + Row[1].y + Row[2].z;
        if (trace > static_cast<T>(0)) {
            root = sqrt(trace + static_cast<T>(1.0));
            r_orientation.w = static_cast<T>(0.5) * root;
            root = static_cast<T>(0.5) / root;
            r_orientation.x = root * (Row[1].z - Row[2].y);
            r_orientation.y = root * (Row[2].x - Row[0].z);
            r_orientation.z = root * (Row[0].y - Row[1].x);
        } // end_panel if > 0
        else {
            static int next[3] = {1, 2, 0};
            i = 0;
            if(Row[1].y > Row[0].x) i = 1;
            if(Row[2].z > Row[i][i]) i = 2;
            j = next[i];
            k = next[j];

            int off = 1;

            root = sqrt(Row[i][i] - Row[j][j] - Row[k][k] + static_cast<T>(1.0));

            r_orientation[i + off] = static_cast<T>(0.5) * root;
            root = static_cast<T>(0.5) / root;
            r_orientation[j + off] = root * (Row[i][j] + Row[j][i]);
            r_orientation[k + off] = root * (Row[i][k] + Row[k][i]);
            r_orientation.w = root * (Row[j][k] - Row[k][j]);
        } // end_panel if <= 0

        return true;
    }

    template<std::floating_point T>
    constexpr Mat<3, 3, T> adjugate(const Mat<3, 3, T>& m)
    {
        // google: "Adjugate Matrix": it's related to the cofactor matrix and is
        // related to the inverse of a matrix through:
        //
        //     inverse(M) = Adjugate(M) / determinant(M);

        Mat<3, 3, T> rv;
        rv[0][0] = + (m[1][1] * m[2][2] - m[2][1] * m[1][2]);
        rv[1][0] = - (m[1][0] * m[2][2] - m[2][0] * m[1][2]);
        rv[2][0] = + (m[1][0] * m[2][1] - m[2][0] * m[1][1]);
        rv[0][1] = - (m[0][1] * m[2][2] - m[2][1] * m[0][2]);
        rv[1][1] = + (m[0][0] * m[2][2] - m[2][0] * m[0][2]);
        rv[2][1] = - (m[0][0] * m[2][1] - m[2][0] * m[0][1]);
        rv[0][2] = + (m[0][1] * m[1][2] - m[1][1] * m[0][2]);
        rv[1][2] = - (m[0][0] * m[1][2] - m[1][0] * m[0][2]);
        rv[2][2] = + (m[0][0] * m[1][1] - m[1][0] * m[0][1]);
        return rv;
    }

    template<std::floating_point T>
    Mat<3, 3, T> normal_matrix(const Mat<4, 4, T>& m)
    {
        // "On the Transformation of Surface Normals" by Andrew Glassner (1987)
        //
        // "One option is to replace the inverse with the adjoint of M. The
        //  adjoint is attractive because it always exists, even when M is
        //  singular. The inverse and the adjoint are related by:
        //
        //      inverse(M) = adjoint(M) / determinant(M);
        //
        //  so, when the inverse exists, they only differ by a constant factor.
        //  Therefore, using adjoint(M) instead of inverse(M) only affects the
        //  magnitude of the resulting normal vector. Normal vectors have to
        //  be normalized after mutiplication with a normal matrix anyway, so
        //  nothing is lost"

        const Mat<3, 3, T> top_left{m};
        return adjugate(transpose(top_left));
    }

    template<std::floating_point T>
    Mat<4, 4, T> normal_matrix4(const Mat<4, 4, T>& m)
    {
        return Mat<4, 4, T>{normal_matrix(m)};
    }

    template<size_t C, size_t R, typename T>
    constexpr const T* value_ptr(const Mat<C, R, T>& m)
    {
        return m.data()->data();
    }

    template<size_t C, size_t R, typename T>
    constexpr T* value_ptr(Mat<C, R, T>& m)
    {
        return m.data()->data();
    }
}

template<size_t C, size_t R, typename T>
struct std::hash<osc::Mat<C, R, T>> final {
    size_t operator()(const osc::Mat<C, R, T>& m) const
    {
        return osc::hash_range(m);
    }
};
