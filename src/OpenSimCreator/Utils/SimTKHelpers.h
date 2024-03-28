#pragma once

#include <SimTKcommon/SmallMatrix.h>
#include <SimTKcommon/internal/MassProperties.h>
#include <SimTKcommon/internal/Rotation.h>
#include <SimTKcommon/internal/Transform.h>
#include <oscar/Maths/Eulers.h>
#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>

namespace osc { struct Color; }
namespace osc { struct Transform; }

namespace osc
{
    // converters: from osc types to SimTK,
    SimTK::Vec3 ToSimTKVec3(Vec3 const&);
    inline SimTK::Vec3 ToSimTKVec3(Vec4 const& v) { return ToSimTKVec3(Vec3{v}); }
    SimTK::Vec3 ToSimTKVec3(Eulers const&);
    SimTK::Mat33 ToSimTKMat3(Mat3 const&);
    SimTK::Inertia ToSimTKInertia(Vec3 const&);
    SimTK::Transform ToSimTKTransform(Transform const&);
    SimTK::Rotation ToSimTKRotation(Quat const&);
    SimTK::Vec3 ToSimTKRGBVec3(Color const&);

    // converters: from SimTK types to osc
    Vec3 ToVec3(SimTK::Vec3 const&);
    Vec4 to_vec4(SimTK::Vec3 const&, float w = 1.0f);
    Mat4 ToMat4x4(SimTK::Transform const&);
    Mat3 ToMat3(SimTK::Mat33 const&);
    Mat4 mat4_cast(SimTK::Rotation const&);
    Quat ToQuat(SimTK::Rotation const&);
    Transform decompose_to_transform(SimTK::Transform const&);
}
