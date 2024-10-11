#include "SimTKConverters.h"

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

SimTK::Vec3 osc::Converter<Vec3, SimTK::Vec3>::operator()(const Vec3& v) const
{
    return {
        static_cast<double>(v.x),
        static_cast<double>(v.y),
        static_cast<double>(v.z),
    };
}

SimTK::Vec3 osc::Converter<EulerAngles, SimTK::Vec3>::operator()(const EulerAngles& v) const
{
    return {
        static_cast<double>(v.x.count()),
        static_cast<double>(v.y.count()),
        static_cast<double>(v.z.count()),
    };
}

SimTK::Mat33 osc::Converter<Mat3, SimTK::Mat33>::operator()(const Mat3& m) const
{
    return SimTK::Mat33 {
        static_cast<double>(m[0][0]), static_cast<double>(m[1][0]), static_cast<double>(m[2][0]),
        static_cast<double>(m[0][1]), static_cast<double>(m[1][1]), static_cast<double>(m[2][1]),
        static_cast<double>(m[0][2]), static_cast<double>(m[1][2]), static_cast<double>(m[2][2]),
    };
}

SimTK::Inertia osc::Converter<Vec3, SimTK::Inertia>::operator()(const Vec3& v) const
{
    return {
        static_cast<double>(v[0]),
        static_cast<double>(v[1]),
        static_cast<double>(v[2]),
    };
}

SimTK::Transform osc::Converter<Transform, SimTK::Transform>::operator()(const Transform& t) const
{
    return SimTK::Transform{to<SimTK::Rotation>(t.rotation), to<SimTK::Vec3>(t.position)};
}

SimTK::Rotation osc::Converter<Quat, SimTK::Rotation>::operator()(const Quat& q) const
{
    return SimTK::Rotation{to<SimTK::Mat33>(mat3_cast(q))};
}

SimTK::Rotation osc::Converter<EulerAngles, SimTK::Rotation>::operator()(const EulerAngles& eulers) const
{
    return to<SimTK::Rotation>(to_worldspace_rotation_quat(eulers));
}

SimTK::Vec3 osc::Converter<Color, SimTK::Vec3>::operator()(const Color& color) const
{
    return {color.r, color.g, color.b};
}

Vec3 osc::Converter<SimTK::Vec3, Vec3>::operator()(const SimTK::Vec3& v) const
{
    return Vec3{
        static_cast<float>(v[0]),
        static_cast<float>(v[1]),
        static_cast<float>(v[2]),
    };
}

Vec3 osc::Converter<SimTK::UnitVec3, Vec3>::operator()(const SimTK::UnitVec3& v) const
{
    return to<Vec3>(SimTK::Vec3{v});
}

Mat4 osc::Converter<SimTK::Transform, Mat4>::operator()(const SimTK::Transform& t) const
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

Mat3 osc::Converter<SimTK::Mat33, Mat3>::operator()(const SimTK::Mat33& m) const
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

Mat4 osc::Converter<SimTK::Rotation, Mat4>::operator()(const SimTK::Rotation& r) const
{
    const SimTK::Transform t{r};
    return to<Mat4>(t);
}

Quat osc::Converter<SimTK::Rotation, Quat>::operator()(const SimTK::Rotation& r) const
{
    const SimTK::Quaternion q = r.convertRotationToQuaternion();

    return Quat{
        static_cast<float>(q[0]),
        static_cast<float>(q[1]),
        static_cast<float>(q[2]),
        static_cast<float>(q[3]),
    };
}

EulerAngles osc::Converter<SimTK::Rotation, EulerAngles>::operator()(const SimTK::Rotation& r) const
{
    return EulerAngles{to<Vec3>(r.convertRotationToBodyFixedXYZ())};
}

std::array<float, 6> osc::Converter<SimTK::Vec6, std::array<float, 6>>::operator()(const SimTK::Vec6& v) const
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

Transform osc::Converter<SimTK::Transform, Transform>::operator()(const SimTK::Transform& t) const
{
    return Transform{.rotation = to<Quat>(t.R()), .position = to<Vec3>(t.p())};
}
