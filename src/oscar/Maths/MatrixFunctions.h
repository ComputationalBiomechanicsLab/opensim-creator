#pragma once

#include <oscar/Maths/Angle.h>
#include <oscar/Maths/CommonFunctions.h>
#include <oscar/Maths/Constants.h>
#include <oscar/Maths/Mat.h>
#include <oscar/Maths/Qua.h>
#include <oscar/Maths/UnitVec3.h>
#include <oscar/Maths/Vec.h>
#include <oscar/Utils/Assertions.h>

#include <concepts>

namespace osc
{
    template<std::floating_point T>
    Mat<4, 4, T> look_at(
        Vec<3, T> const& eye,
        Vec<3, T> const& center,
        Vec<3, T> const& up)
    {
        Vec<3, T> const f(normalize(center - eye));
        Vec<3, T> const s(normalize(cross(f, up)));
        Vec<3, T> const u(cross(s, f));

        Mat<4, 4, T> Result(1);
        Result[0][0] =  s.x;
        Result[1][0] =  s.y;
        Result[2][0] =  s.z;
        Result[0][1] =  u.x;
        Result[1][1] =  u.y;
        Result[2][1] =  u.z;
        Result[0][2] = -f.x;
        Result[1][2] = -f.y;
        Result[2][2] = -f.z;
        Result[3][0] = -dot(s, eye);
        Result[3][1] = -dot(u, eye);
        Result[3][2] =  dot(f, eye);
        return Result;
    }

    template<std::floating_point T, AngularUnitTraits Units>
    Mat<4, 4, T> perspective(Angle<T, Units> fovy, T aspect, T zNear, T zFar)
    {
        if (fabs(aspect - epsilon<T>) <= T{}) {
            // edge-case: some UIs ask for a perspective matrix on first frame before
            // aspect ratio is known or the aspect ratio is NaN because of a division
            // by zero
            return Mat4{T(1)};
        }

        T const tanHalfFovy = tan(fovy / static_cast<T>(2));

        Mat<4, 4, T> Result(static_cast<T>(0));
        Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
        Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
        Result[2][2] = - (zFar + zNear) / (zFar - zNear);
        Result[2][3] = - static_cast<T>(1);
        Result[3][2] = - (static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
        return Result;
    }

    template<std::floating_point T>
    Mat<4, 4, T> ortho(T left, T right, T bottom, T top, T zNear, T zFar)
    {
        Mat<4, 4, T> Result(1);
        Result[0][0] = static_cast<T>(2) / (right - left);
        Result[1][1] = static_cast<T>(2) / (top - bottom);
        Result[2][2] = - static_cast<T>(2) / (zFar - zNear);
        Result[3][0] = - (right + left) / (right - left);
        Result[3][1] = - (top + bottom) / (top - bottom);
        Result[3][2] = - (zFar + zNear) / (zFar - zNear);
        return Result;
    }

    template<typename T>
    Mat<4, 4, T> scale(Mat<4, 4, T> const& m, Vec<3, T> const& v)
    {
        Mat<4, 4, T> Result;
        Result[0] = m[0] * v[0];
        Result[1] = m[1] * v[1];
        Result[2] = m[2] * v[2];
        Result[3] = m[3];
        return Result;
    }

    template<std::floating_point T, AngularUnitTraits Units>
    Mat<4, 4, T> rotate(Mat<4, 4, T> const& m, Angle<T, Units> angle, UnitVec<3, T> axis)
    {
        T const c = cos(angle);
        T const s = sin(angle);

        Vec<3, T> temp((T(1) - c) * axis);

        Mat<4, 4, T> Rotate;
        Rotate[0][0] = c + temp[0] * axis[0];
        Rotate[0][1] = temp[0] * axis[1] + s * axis[2];
        Rotate[0][2] = temp[0] * axis[2] - s * axis[1];

        Rotate[1][0] = temp[1] * axis[0] - s * axis[2];
        Rotate[1][1] = c + temp[1] * axis[1];
        Rotate[1][2] = temp[1] * axis[2] + s * axis[0];

        Rotate[2][0] = temp[2] * axis[0] + s * axis[1];
        Rotate[2][1] = temp[2] * axis[1] - s * axis[0];
        Rotate[2][2] = c + temp[2] * axis[2];

        Mat<4, 4, T> Result;
        Result[0] = m[0] * Rotate[0][0] + m[1] * Rotate[0][1] + m[2] * Rotate[0][2];
        Result[1] = m[0] * Rotate[1][0] + m[1] * Rotate[1][1] + m[2] * Rotate[1][2];
        Result[2] = m[0] * Rotate[2][0] + m[1] * Rotate[2][1] + m[2] * Rotate[2][2];
        Result[3] = m[3];
        return Result;
    }

    template<std::floating_point T, AngularUnitTraits Units>
    Mat<4, 4, T> rotate(Mat<4, 4, T> const& m, Angle<T, Units> angle, Vec<3, T> const& axis)
    {
        return rotate(m, angle, UnitVec<3, T>{axis});
    }

    template<typename T>
    Mat<4, 4, T> translate(Mat<4, 4, T> const& m, Vec<3, T> const& v)
    {
        Mat<4, 4, T> Result(m);
        Result[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
        return Result;
    }

    template<std::floating_point T>
    T determinant(Mat<3, 3, T> const& m)
    {
        return
            + m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2])
            - m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2])
            + m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]
        );
    }

    template<std::floating_point T>
    T determinant(Mat<4, 4, T> const& m)
    {
        T SubFactor00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
        T SubFactor01 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
        T SubFactor02 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
        T SubFactor03 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
        T SubFactor04 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
        T SubFactor05 = m[2][0] * m[3][1] - m[3][0] * m[2][1];

        Vec<4, T> DetCof(
            + (m[1][1] * SubFactor00 - m[1][2] * SubFactor01 + m[1][3] * SubFactor02),
            - (m[1][0] * SubFactor00 - m[1][2] * SubFactor03 + m[1][3] * SubFactor04),
            + (m[1][0] * SubFactor01 - m[1][1] * SubFactor03 + m[1][3] * SubFactor05),
            - (m[1][0] * SubFactor02 - m[1][1] * SubFactor04 + m[1][2] * SubFactor05));

        return
            m[0][0] * DetCof[0] + m[0][1] * DetCof[1] +
            m[0][2] * DetCof[2] + m[0][3] * DetCof[3];
    }

    template<std::floating_point T>
    T inverse(Mat<3, 3, T> const& m)
    {
        T OneOverDeterminant = static_cast<T>(1) / determinant(m);

        Mat<3, 3, T> Inverse;
        Inverse[0][0] = + (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * OneOverDeterminant;
        Inverse[1][0] = - (m[1][0] * m[2][2] - m[2][0] * m[1][2]) * OneOverDeterminant;
        Inverse[2][0] = + (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * OneOverDeterminant;
        Inverse[0][1] = - (m[0][1] * m[2][2] - m[2][1] * m[0][2]) * OneOverDeterminant;
        Inverse[1][1] = + (m[0][0] * m[2][2] - m[2][0] * m[0][2]) * OneOverDeterminant;
        Inverse[2][1] = - (m[0][0] * m[2][1] - m[2][0] * m[0][1]) * OneOverDeterminant;
        Inverse[0][2] = + (m[0][1] * m[1][2] - m[1][1] * m[0][2]) * OneOverDeterminant;
        Inverse[1][2] = - (m[0][0] * m[1][2] - m[1][0] * m[0][2]) * OneOverDeterminant;
        Inverse[2][2] = + (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * OneOverDeterminant;

        return Inverse;
    }

    template<std::floating_point T>
    Mat<4, 4, T> inverse(Mat<4, 4, T> const& m)
    {
        T Coef00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
        T Coef02 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
        T Coef03 = m[1][2] * m[2][3] - m[2][2] * m[1][3];

        T Coef04 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
        T Coef06 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
        T Coef07 = m[1][1] * m[2][3] - m[2][1] * m[1][3];

        T Coef08 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
        T Coef10 = m[1][1] * m[3][2] - m[3][1] * m[1][2];
        T Coef11 = m[1][1] * m[2][2] - m[2][1] * m[1][2];

        T Coef12 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
        T Coef14 = m[1][0] * m[3][3] - m[3][0] * m[1][3];
        T Coef15 = m[1][0] * m[2][3] - m[2][0] * m[1][3];

        T Coef16 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
        T Coef18 = m[1][0] * m[3][2] - m[3][0] * m[1][2];
        T Coef19 = m[1][0] * m[2][2] - m[2][0] * m[1][2];

        T Coef20 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
        T Coef22 = m[1][0] * m[3][1] - m[3][0] * m[1][1];
        T Coef23 = m[1][0] * m[2][1] - m[2][0] * m[1][1];

        Vec<4, T> Fac0(Coef00, Coef00, Coef02, Coef03);
        Vec<4, T> Fac1(Coef04, Coef04, Coef06, Coef07);
        Vec<4, T> Fac2(Coef08, Coef08, Coef10, Coef11);
        Vec<4, T> Fac3(Coef12, Coef12, Coef14, Coef15);
        Vec<4, T> Fac4(Coef16, Coef16, Coef18, Coef19);
        Vec<4, T> Fac5(Coef20, Coef20, Coef22, Coef23);

        Vec<4, T> Vec0(m[1][0], m[0][0], m[0][0], m[0][0]);
        Vec<4, T> Vec1(m[1][1], m[0][1], m[0][1], m[0][1]);
        Vec<4, T> Vec2(m[1][2], m[0][2], m[0][2], m[0][2]);
        Vec<4, T> Vec3(m[1][3], m[0][3], m[0][3], m[0][3]);

        Vec<4, T> Inv0(Vec1 * Fac0 - Vec2 * Fac1 + Vec3 * Fac2);
        Vec<4, T> Inv1(Vec0 * Fac0 - Vec2 * Fac3 + Vec3 * Fac4);
        Vec<4, T> Inv2(Vec0 * Fac1 - Vec1 * Fac3 + Vec3 * Fac5);
        Vec<4, T> Inv3(Vec0 * Fac2 - Vec1 * Fac4 + Vec2 * Fac5);

        Vec<4, T> SignA(+1, -1, +1, -1);
        Vec<4, T> SignB(-1, +1, -1, +1);
        Mat<4, 4, T> Inverse(Inv0 * SignA, Inv1 * SignB, Inv2 * SignA, Inv3 * SignB);

        Vec<4, T> Row0(Inverse[0][0], Inverse[1][0], Inverse[2][0], Inverse[3][0]);

        Vec<4, T> Dot0(m[0] * Row0);
        T Dot1 = (Dot0.x + Dot0.y) + (Dot0.z + Dot0.w);

        T OneOverDeterminant = static_cast<T>(1) / Dot1;

        return Inverse * OneOverDeterminant;
    }

    template<typename T>
    Mat<3, 3, T> transpose(Mat<3, 3, T> const& m)
    {
        Mat<3, 3, T> Result;
        Result[0][0] = m[0][0];
        Result[0][1] = m[1][0];
        Result[0][2] = m[2][0];

        Result[1][0] = m[0][1];
        Result[1][1] = m[1][1];
        Result[1][2] = m[2][1];

        Result[2][0] = m[0][2];
        Result[2][1] = m[1][2];
        Result[2][2] = m[2][2];
        return Result;
    }

    template<typename T>
    Mat<4, 4, T> transpose(Mat<4, 4, T> const& m)
    {
        Mat<4, 4, T> Result;
        Result[0][0] = m[0][0];
        Result[0][1] = m[1][0];
        Result[0][2] = m[2][0];
        Result[0][3] = m[3][0];

        Result[1][0] = m[0][1];
        Result[1][1] = m[1][1];
        Result[1][2] = m[2][1];
        Result[1][3] = m[3][1];

        Result[2][0] = m[0][2];
        Result[2][1] = m[1][2];
        Result[2][2] = m[2][2];
        Result[2][3] = m[3][2];

        Result[3][0] = m[0][3];
        Result[3][1] = m[1][3];
        Result[3][2] = m[2][3];
        Result[3][3] = m[3][3];
        return Result;
    }

    template<typename T>
    Vec<3, RadiansT<T>> extract_eulers_xyz(Mat<4, 4, T> const& M)
    {
        RadiansT<T> T1 = atan2(M[2][1], M[2][2]);
        T C2 = sqrt(M[0][0]*M[0][0] + M[1][0]*M[1][0]);
        RadiansT<T> T2 = atan2(-M[2][0], C2);
        T S1 = sin(T1);
        T C1 = cos(T1);
        RadiansT<T> T3 = atan2(S1*M[0][2] - C1*M[0][1], C1*M[1][1] - S1*M[1][2  ]);

        return Vec<3, RadiansT<T>>{-T1, -T2, -T3};
    }

    namespace detail
    {
        /// Make a linear combination of two vectors and return the result.
        template<typename T>
        Vec<3, T> combine(Vec<3, T> const& a, Vec<3, T> const& b, T ascl, T bscl)
        {
            return (a * ascl) + (b * bscl);
        }

        template<typename T>
        Vec<3, T> scale(Vec<3, T> const& v, T desiredLength)
        {
            return v * desiredLength / length(v);
        }
    }

    template<typename T>
    bool decompose(
        Mat<4, 4, T> const& ModelMatrix,
        Vec<3, T>& Scale,
        Qua<T>& Orientation,
        Vec<3, T>& Translation,
        Vec<3, T>& Skew,
        Vec<4, T>& Perspective)
    {
        // Matrix decompose
        // http://www.opensource.apple.com/source/WebCore/WebCore-514/platform/graphics/transforms/TransformationMatrix.cpp
        // Decomposes the mode matrix to translations,rotation scale components

        Mat<4, 4, T> LocalMatrix(ModelMatrix);

        // Normalize the matrix.
        if (equal_within_epsilon(LocalMatrix[3][3], static_cast<T>(0))) {
            return false;
        }

        for (size_t i = 0; i < 4; ++i) {
            for (size_t j = 0; j < 4; ++j) {
                LocalMatrix[i][j] /= LocalMatrix[3][3];
            }
        }

        // perspectiveMatrix is used to solve for perspective, but it also provides
        // an easy way to test for singularity of the upper 3x3 component.
        Mat<4, 4, T> PerspectiveMatrix(LocalMatrix);

        for (size_t i = 0; i < 3; i++) {
            PerspectiveMatrix[i][3] = static_cast<T>(0);
        }
        PerspectiveMatrix[3][3] = static_cast<T>(1);

        /// TODO: Fixme!
        if (equal_within_epsilon(determinant(PerspectiveMatrix), static_cast<T>(0))) {
            return false;
        }

        // First, isolate perspective.  This is the messiest.
        if(
            !equal_within_epsilon(LocalMatrix[0][3], static_cast<T>(0)) ||
            !equal_within_epsilon(LocalMatrix[1][3], static_cast<T>(0)) ||
            !equal_within_epsilon(LocalMatrix[2][3], static_cast<T>(0)))
        {
            // rightHandSide is the right hand side of the equation.
            Vec<4, T> RightHandSide;
            RightHandSide[0] = LocalMatrix[0][3];
            RightHandSide[1] = LocalMatrix[1][3];
            RightHandSide[2] = LocalMatrix[2][3];
            RightHandSide[3] = LocalMatrix[3][3];

            // Solve the equation by inverting PerspectiveMatrix and multiplying
            // rightHandSide by the inverse.  (This is the easiest way, not
            // necessarily the best.)
            Mat<4, 4, T> InversePerspectiveMatrix = inverse(PerspectiveMatrix);//   inverse(PerspectiveMatrix, inversePerspectiveMatrix);
            Mat<4, 4, T> TransposedInversePerspectiveMatrix = transpose(InversePerspectiveMatrix);//   transposeMatrix4(inversePerspectiveMatrix, transposedInversePerspectiveMatrix);

            Perspective = TransposedInversePerspectiveMatrix * RightHandSide;
            //  v4MulPointByMatrix(rightHandSide, transposedInversePerspectiveMatrix, perspectivePoint);

            // Clear the perspective partition
            LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
            LocalMatrix[3][3] = static_cast<T>(1);
        }
        else {
            // No perspective.
            Perspective = Vec<4, T>(0, 0, 0, 1);
        }

        // Next take care of translation (easy).
        Translation = Vec<3, T>(LocalMatrix[3]);
        LocalMatrix[3] = Vec<4, T>(0, 0, 0, LocalMatrix[3].w);

        Vec<3, T> Row[3], Pdum3;

        // Now get scale and shear.
        for (size_t i = 0; i < 3; ++i) {
            for(size_t j = 0; j < 3; ++j) {
                Row[i][j] = LocalMatrix[i][j];
            }
        }

        // Compute X scale factor and normalize first row.
        Scale.x = length(Row[0]);// v3Length(Row[0]);

        Row[0] = detail::scale(Row[0], static_cast<T>(1));

        // Compute XY shear factor and make 2nd row orthogonal to 1st.
        Skew.z = dot(Row[0], Row[1]);
        Row[1] = detail::combine(Row[1], Row[0], static_cast<T>(1), -Skew.z);

        // Now, compute Y scale and normalize 2nd row.
        Scale.y = length(Row[1]);
        Row[1] = detail::scale(Row[1], static_cast<T>(1));
        Skew.z /= Scale.y;

        // Compute XZ and YZ shears, orthogonalize 3rd row.
        Skew.y = dot(Row[0], Row[2]);
        Row[2] = detail::combine(Row[2], Row[0], static_cast<T>(1), -Skew.y);
        Skew.x = dot(Row[1], Row[2]);
        Row[2] = detail::combine(Row[2], Row[1], static_cast<T>(1), -Skew.x);

        // Next, get Z scale and normalize 3rd row.
        Scale.z = length(Row[2]);
        Row[2] = detail::scale(Row[2], static_cast<T>(1));
        Skew.y /= Scale.z;
        Skew.x /= Scale.z;

        // At this point, the matrix (in rows[]) is orthonormal.
        // Check for a coordinate system flip.  If the determinant
        // is -1, then negate the matrix and the scaling factors.
        Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
        if (dot(Row[0], Pdum3) < 0) {
            for (size_t i = 0; i < 3; i++) {
                Scale[i] *= static_cast<T>(-1);
                Row[i] *= static_cast<T>(-1);
            }
        }

        // Now, get the rotations out, as described in the gem.

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
            Orientation.w = static_cast<T>(0.5) * root;
            root = static_cast<T>(0.5) / root;
            Orientation.x = root * (Row[1].z - Row[2].y);
            Orientation.y = root * (Row[2].x - Row[0].z);
            Orientation.z = root * (Row[0].y - Row[1].x);
        } // End if > 0
        else {
            static int Next[3] = {1, 2, 0};
            i = 0;
            if(Row[1].y > Row[0].x) i = 1;
            if(Row[2].z > Row[i][i]) i = 2;
            j = Next[i];
            k = Next[j];

            int off = 1;

            root = sqrt(Row[i][i] - Row[j][j] - Row[k][k] + static_cast<T>(1.0));

            Orientation[i + off] = static_cast<T>(0.5) * root;
            root = static_cast<T>(0.5) / root;
            Orientation[j + off] = root * (Row[i][j] + Row[j][i]);
            Orientation[k + off] = root * (Row[i][k] + Row[k][i]);
            Orientation.w = root * (Row[j][k] - Row[k][j]);
        } // End if <= 0

        return true;
    }
}
