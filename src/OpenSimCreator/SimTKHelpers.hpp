#pragma once

#include <oscar/Maths/Transform.hpp>

#include <glm/gtx/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <SimTKcommon/SmallMatrix.h>
#include <SimTKcommon/internal/MassProperties.h>
#include <SimTKcommon/internal/Transform.h>
#include <SimTKcommon/internal/Rotation.h>

namespace osc
{
    // converters: from osc types to SimTK
    SimTK::Vec3 ToSimTKVec3(float v[3]);
    SimTK::Vec3 ToSimTKVec3(glm::vec3 const&);
    SimTK::Mat33 ToSimTKMat3(glm::mat3 const&);
    SimTK::Inertia ToSimTKInertia(float v[3]);
    SimTK::Inertia ToSimTKInertia(glm::vec3 const&);
    SimTK::Transform ToSimTKTransform(glm::mat4x3 const&);
    SimTK::Transform ToSimTKTransform(Transform const&);
    SimTK::Rotation ToSimTKRotation(glm::quat const&);

    // converters: from SimTK types to osc
    glm::vec3 ToVec3(SimTK::Vec3 const&);
    glm::vec4 ToVec4(SimTK::Vec3 const&, float w = 1.0f);
    glm::mat4x3 ToMat4x3(SimTK::Transform const&);
    glm::mat4x4 ToMat4x4(SimTK::Transform const&);
    glm::mat4x4 ToMat4(SimTK::Rotation const&);
    glm::quat ToQuat(SimTK::Rotation const&);
    Transform ToTransform(SimTK::Transform const&);
}
