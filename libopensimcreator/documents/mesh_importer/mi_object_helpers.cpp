#include "mi_object_helpers.h"

#include <libopensimcreator/documents/mesh_importer/mi_body.h>
#include <libopensimcreator/documents/mesh_importer/mi_ground.h>
#include <libopensimcreator/documents/mesh_importer/mi_joint.h>
#include <libopensimcreator/documents/mesh_importer/mi_class.h>
#include <libopensimcreator/documents/mesh_importer/mi_mesh.h>
#include <libopensimcreator/documents/mesh_importer/mi_object.h>
#include <libopensimcreator/documents/mesh_importer/mi_station.h>

#include <liboscar/graphics/mesh_functions.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/utilities/std_variant_helpers.h>

#include <array>
#include <variant>

using osc::MiClass;
using osc::MiVariantReference;
using osc::Vector3;

bool osc::CanAttachMeshTo(const MiObject& e)
{
    return std::visit(Overload
    {
        [](const MiGround&)  { return true; },
        [](const MiMesh&)    { return false; },
        [](const MiBody&)    { return true; },
        [](const MiJoint&)   { return true; },
        [](const MiStation&) { return false; },
    }, e.toVariant());
}

bool osc::CanAttachStationTo(const MiObject& e)
{
    return std::visit(Overload
    {
        [](const MiGround&)  { return true; },
        [](const MiMesh&)    { return true; },
        [](const MiBody&)    { return true; },
        [](const MiJoint&)   { return false; },
        [](const MiStation&) { return false; },
    }, e.toVariant());
}

const std::array<MiClass, std::variant_size_v<MiVariantReference>>& osc::GetSceneElClasses()
{
    static const auto s_Classes = std::to_array(
    {
        MiGround::Class(),
        MiMesh::Class(),
        MiBody::Class(),
        MiJoint::Class(),
        MiStation::Class(),
    });
    return s_Classes;
}

Vector3 osc::AverageCenter(const MiMesh& el)
{
    const Vector3 centerpointInModelSpace = average_centroid_of(el.getMeshData());
    return el.getXForm() * centerpointInModelSpace;
}

Vector3 osc::mass_center_of(const MiMesh& el)
{
    const Vector3 massCenterInModelSpace = mass_center_of(el.getMeshData());
    return el.getXForm() * massCenterInModelSpace;
}
