#include "SimTKHelpers.h"

#include <SimTKcommon/SmallMatrix.h>
#include <SimTKcommon/internal/MassProperties.h>
#include <SimTKcommon/internal/Rotation.h>
#include <SimTKcommon/internal/Transform.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>

using namespace osc;

SimTK::Vec3 osc::ToSimTKVec3(const Vec3& v)
{
    return {
        static_cast<double>(v.x),
        static_cast<double>(v.y),
        static_cast<double>(v.z),
    };
}

SimTK::Vec3 osc::ToSimTKVec3(const Eulers& v)
{
    return {
        static_cast<double>(v.x.count()),
        static_cast<double>(v.y.count()),
        static_cast<double>(v.z.count()),
    };
}

SimTK::Mat33 osc::ToSimTKMat3(const Mat3& m)
{
    return SimTK::Mat33 {
        static_cast<double>(m[0][0]), static_cast<double>(m[1][0]), static_cast<double>(m[2][0]),
        static_cast<double>(m[0][1]), static_cast<double>(m[1][1]), static_cast<double>(m[2][1]),
        static_cast<double>(m[0][2]), static_cast<double>(m[1][2]), static_cast<double>(m[2][2]),
    };
}

SimTK::Inertia osc::ToSimTKInertia(const Vec3& v)
{
    return {
        static_cast<double>(v[0]),
        static_cast<double>(v[1]),
        static_cast<double>(v[2]),
    };
}

SimTK::Transform osc::ToSimTKTransform(const Transform& t)
{
    return SimTK::Transform{ToSimTKRotation(t.rotation), ToSimTKVec3(t.position)};
}

SimTK::Transform osc::ToSimTKTransform(const Eulers& eulers, const Vec3& translation)
{
    return SimTK::Transform{ToSimTKRotation(eulers), ToSimTKVec3(translation)};
}

SimTK::Rotation osc::ToSimTKRotation(const Quat& q)
{
    return SimTK::Rotation{ToSimTKMat3(mat3_cast(q))};
}

SimTK::Rotation osc::ToSimTKRotation(const Eulers& eulers)
{
    return ToSimTKRotation(to_worldspace_rotation_quat(eulers));
}

SimTK::Vec3 osc::ToSimTKRGBVec3(const Color& color)
{
    return {color.r, color.g, color.b};
}

Vec3 osc::ToVec3(const SimTK::Vec3& v)
{
    return Vec3{
        static_cast<float>(v[0]),
        static_cast<float>(v[1]),
        static_cast<float>(v[2]),
    };
}

Vec4 osc::to_vec4(const SimTK::Vec3& v, float w)
{
    return Vec4{
        static_cast<float>(v[0]),
        static_cast<float>(v[1]),
        static_cast<float>(v[2]),
        static_cast<float>(w),
    };
}

Mat4 osc::ToMat4x4(const SimTK::Transform& t)
{
    Mat4 m{};

    // x0 y0 z0 w0
    const SimTK::Rotation& r = t.R();
    const SimTK::Vec3& p = t.p();

    {
        const auto& row0 = r[0];
        m[0][0] = static_cast<float>(row0[0]);
        m[1][0] = static_cast<float>(row0[1]);
        m[2][0] = static_cast<float>(row0[2]);
        m[3][0] = static_cast<float>(p[0]);
    }

    {
        const auto& row1 = r[1];
        m[0][1] = static_cast<float>(row1[0]);
        m[1][1] = static_cast<float>(row1[1]);
        m[2][1] = static_cast<float>(row1[2]);
        m[3][1] = static_cast<float>(p[1]);
    }

    {
        const auto& row2 = r[2];
        m[0][2] = static_cast<float>(row2[0]);
        m[1][2] = static_cast<float>(row2[1]);
        m[2][2] = static_cast<float>(row2[2]);
        m[3][2] = static_cast<float>(p[2]);
    }

    // row3
    m[0][3] = 0.0f;
    m[1][3] = 0.0f;
    m[2][3] = 0.0f;
    m[3][3] = 1.0f;

    return m;
}

Mat3 osc::ToMat3(const SimTK::Mat33& m)
{
    Mat3 rv{};
    for (int row = 0; row < 3; ++row) {
        const auto& r = m[row];
        rv[0][row] = static_cast<float>(r[0]);
        rv[1][row] = static_cast<float>(r[1]);
        rv[2][row] = static_cast<float>(r[2]);
    }
    return rv;
}

Mat4 osc::mat4_cast(const SimTK::Rotation& r)
{
    const SimTK::Transform t{r};
    return ToMat4x4(t);
}

Quat osc::ToQuat(const SimTK::Rotation& r)
{
    const SimTK::Quaternion q = r.convertRotationToQuaternion();

    return Quat{
        static_cast<float>(q[0]),
        static_cast<float>(q[1]),
        static_cast<float>(q[2]),
        static_cast<float>(q[3]),
    };
}

std::array<float, 6> osc::ToArray(const SimTK::Vec6& v)
{
    return {
        static_cast<float>(v[0]),
        static_cast<float>(v[1]),
        static_cast<float>(v[2]),
        static_cast<float>(v[3]),
        static_cast<float>(v[4]),
        static_cast<float>(v[5]),
    };
}

Transform osc::decompose_to_transform(const SimTK::Transform& t)
{
    return Transform{.rotation = ToQuat(t.R()), .position = ToVec3(t.p())};
}
