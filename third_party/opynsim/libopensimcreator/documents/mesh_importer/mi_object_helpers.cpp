#include "mi_object_helpers.h"

#include <libopensimcreator/documents/mesh_importer/mi_body.h>
#include <libopensimcreator/documents/mesh_importer/mi_ground.h>
#include <libopensimcreator/documents/mesh_importer/mi_joint.h>
#include <libopensimcreator/documents/mesh_importer/mi_class.h>
#include <libopensimcreator/documents/mesh_importer/mi_mesh.h>
#include <libopensimcreator/documents/mesh_importer/mi_object.h>
#include <libopensimcreator/documents/mesh_importer/mi_station.h>

#include <liboscar/graphics/mesh_functions.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/std_variant_helpers.h>

#include <array>
#include <variant>

using osc::MiClass;
using osc::MiVariantReference;
using osc::Vector3;

bool osc::can_attach_mesh_to(const MiObject& e)
{
    // clang-format off
    return std::visit(Overload
    {
        [](const MiGround&)  { return true; },
        [](const MiMesh&)    { return false; },
        [](const MiBody&)    { return true; },
        [](const MiJoint&)   { return true; },
        [](const MiStation&) { return false; },
    }, e.to_variant());
    // clang-format on
}

bool osc::can_attach_station_to(const MiObject& e)
{
    // clang-format off
    return std::visit(Overload
    {
        [](const MiGround&)  { return true; },
        [](const MiMesh&)    { return true; },
        [](const MiBody&)    { return true; },
        [](const MiJoint&)   { return false; },
        [](const MiStation&) { return false; },
    }, e.to_variant());
    // clang-format on
}

const std::array<MiClass, std::variant_size_v<MiVariantReference>>& osc::get_scene_el_classes()
{
    static const auto s_classes = std::to_array({
        MiGround::get_class(),
        MiMesh::get_class(),
        MiBody::get_class(),
        MiJoint::get_class(),
        MiStation::get_class(),
    });
    return s_classes;
}

Vector3 osc::average_center(const MiMesh& el)
{
    const Vector3 centerpoint_in_model_space = average_centroid_of(el.get_mesh_data());
    return el.get_x_form() * centerpoint_in_model_space;
}

Vector3 osc::mass_center_of(const MiMesh& el)
{
    const Vector3 mass_center_in_model_space = mass_center_of(el.get_mesh_data());
    return el.get_x_form() * mass_center_in_model_space;
}
