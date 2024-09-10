#pragma once

#include <SimTKcommon/SmallMatrix.h>
#include <SimTKcommon/internal/MassProperties.h>
#include <SimTKcommon/internal/Rotation.h>
#include <SimTKcommon/internal/Transform.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/EulerAngles.h>
#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>

#include <array>

namespace osc { struct Transform; }

namespace osc
{
    // converters: from osc types to SimTK,
    SimTK::Vec3 ToSimTKVec3(const Vec3&);
    inline SimTK::Vec3 ToSimTKVec3(const Vec4& v) { return ToSimTKVec3(Vec3{v}); }
    SimTK::Vec3 ToSimTKVec3(const EulerAngles&);
    SimTK::Mat33 ToSimTKMat3(const Mat3&);
    SimTK::Inertia ToSimTKInertia(const Vec3&);
    SimTK::Transform ToSimTKTransform(const Transform&);
    SimTK::Transform ToSimTKTransform(const EulerAngles&, const Vec3&);
    SimTK::Rotation ToSimTKRotation(const Quat&);
    SimTK::Rotation ToSimTKRotation(const EulerAngles&);
    SimTK::Vec3 ToSimTKRGBVec3(const Color&);

    // converters: from SimTK types to osc
    Vec3 ToVec3(const SimTK::Vec3&);
    Vec4 to_vec4(const SimTK::Vec3&, float w = 1.0f);
    Mat4 ToMat4x4(const SimTK::Transform&);
    Mat3 ToMat3(const SimTK::Mat33&);
    Mat4 mat4_cast(const SimTK::Rotation&);
    Quat ToQuat(const SimTK::Rotation&);
    EulerAngles ToEulerAngles(const SimTK::Rotation&);
    std::array<float, 6> ToArray(const SimTK::Vec6&);
    Transform decompose_to_transform(const SimTK::Transform&);
}
