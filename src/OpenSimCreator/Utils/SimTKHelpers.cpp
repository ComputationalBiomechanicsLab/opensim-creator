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

// public API

SimTK::Vec3 osc::ToSimTKVec3(Vec3 const& v)
{
    return
    {
        static_cast<double>(v.x),
        static_cast<double>(v.y),
        static_cast<double>(v.z),
    };
}

SimTK::Vec3 osc::ToSimTKVec3(Eulers const& v)
{
    return
    {
        static_cast<double>(v.x.count()),
        static_cast<double>(v.y.count()),
        static_cast<double>(v.z.count()),
    };
}

SimTK::Mat33 osc::ToSimTKMat3(Mat3 const& m)
{
    return SimTK::Mat33
    {
        static_cast<double>(m[0][0]), static_cast<double>(m[1][0]), static_cast<double>(m[2][0]),
        static_cast<double>(m[0][1]), static_cast<double>(m[1][1]), static_cast<double>(m[2][1]),
        static_cast<double>(m[0][2]), static_cast<double>(m[1][2]), static_cast<double>(m[2][2]),
    };
}

SimTK::Inertia osc::ToSimTKInertia(Vec3 const& v)
{
    return
    {
        static_cast<double>(v[0]),
        static_cast<double>(v[1]),
        static_cast<double>(v[2]),
    };
}

SimTK::Transform osc::ToSimTKTransform(Transform const& t)
{
    return SimTK::Transform{ToSimTKRotation(t.rotation), ToSimTKVec3(t.position)};
}

SimTK::Rotation osc::ToSimTKRotation(Quat const& q)
{
    return SimTK::Rotation{ToSimTKMat3(ToMat3(q))};
}

SimTK::Vec3 osc::ToSimTKRGBVec3(Color const& color)
{
    return {color.r, color.g, color.b};
}

Vec3 osc::ToVec3(SimTK::Vec3 const& v)
{
    return Vec3
    {
        static_cast<float>(v[0]),
        static_cast<float>(v[1]),
        static_cast<float>(v[2]),
    };
}

Vec4 osc::ToVec4(SimTK::Vec3 const& v, float w)
{
    return Vec4
    {
        static_cast<float>(v[0]),
        static_cast<float>(v[1]),
        static_cast<float>(v[2]),
        static_cast<float>(w),
    };
}

Mat4 osc::ToMat4x4(SimTK::Transform const& t)
{
    Mat4 m{};

    // x0 y0 z0 w0
    SimTK::Rotation const& r = t.R();
    SimTK::Vec3 const& p = t.p();

    {
        auto const& row0 = r[0];
        m[0][0] = static_cast<float>(row0[0]);
        m[1][0] = static_cast<float>(row0[1]);
        m[2][0] = static_cast<float>(row0[2]);
        m[3][0] = static_cast<float>(p[0]);
    }

    {
        auto const& row1 = r[1];
        m[0][1] = static_cast<float>(row1[0]);
        m[1][1] = static_cast<float>(row1[1]);
        m[2][1] = static_cast<float>(row1[2]);
        m[3][1] = static_cast<float>(p[1]);
    }

    {
        auto const& row2 = r[2];
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

Mat4 osc::ToMat4(SimTK::Rotation const& r)
{
    SimTK::Transform const t{r};
    return ToMat4x4(t);
}

Quat osc::ToQuat(SimTK::Rotation const& r)
{
    SimTK::Quaternion const q = r.convertRotationToQuaternion();

    return Quat
    {
        static_cast<float>(q[0]),
        static_cast<float>(q[1]),
        static_cast<float>(q[2]),
        static_cast<float>(q[3]),
    };
}

Transform osc::ToTransform(SimTK::Transform const& t)
{
    return Transform{.rotation = ToQuat(t.R()), .position = ToVec3(t.p())};
}
