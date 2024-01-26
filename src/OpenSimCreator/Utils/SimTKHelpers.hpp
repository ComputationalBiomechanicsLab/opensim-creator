#pragma once

#include <oscar/Maths/Eulers.hpp>
#include <oscar/Maths/Mat3.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/Mat4x3.hpp>
#include <oscar/Maths/Quat.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Maths/Vec4.hpp>
#include <SimTKcommon/SmallMatrix.h>
#include <SimTKcommon/internal/MassProperties.h>
#include <SimTKcommon/internal/Transform.h>
#include <SimTKcommon/internal/Rotation.h>

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
    SimTK::Transform ToSimTKTransform(Mat4x3 const&);
    SimTK::Transform ToSimTKTransform(Transform const&);
    SimTK::Rotation ToSimTKRotation(Quat const&);
    SimTK::Vec3 ToSimTKRGBVec3(Color const&);

    // converters: from SimTK types to osc
    Vec3 ToVec3(SimTK::Vec3 const&);
    Vec4 ToVec4(SimTK::Vec3 const&, float w = 1.0f);
    Mat4x3 ToMat4x3(SimTK::Transform const&);
    Mat4 ToMat4x4(SimTK::Transform const&);
    Mat4 ToMat4(SimTK::Rotation const&);
    Quat ToQuat(SimTK::Rotation const&);
    Transform ToTransform(SimTK::Transform const&);
}
