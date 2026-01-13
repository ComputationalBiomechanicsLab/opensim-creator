#include "MIObjectHelpers.h"

#include <libopensimcreator/Documents/MeshImporter/Body.h>
#include <libopensimcreator/Documents/MeshImporter/Ground.h>
#include <libopensimcreator/Documents/MeshImporter/Joint.h>
#include <libopensimcreator/Documents/MeshImporter/MIClass.h>
#include <libopensimcreator/Documents/MeshImporter/MIObject.h>
#include <libopensimcreator/Documents/MeshImporter/Mesh.h>
#include <libopensimcreator/Documents/MeshImporter/Station.h>

#include <liboscar/graphics/MeshFunctions.h>
#include <liboscar/maths/Vector3.h>
#include <liboscar/utils/StdVariantHelpers.h>

#include <array>
#include <variant>

using osc::mi::MIClass;
using osc::mi::SceneElVariant;
using osc::Vector3;

bool osc::mi::CanAttachMeshTo(const MIObject& e)
{
    return std::visit(Overload
    {
        [](const Ground&)  { return true; },
        [](const Mesh&)    { return false; },
        [](const Body&)    { return true; },
        [](const Joint&)   { return true; },
        [](const StationEl&) { return false; },
    }, e.toVariant());
}

bool osc::mi::CanAttachStationTo(const MIObject& e)
{
    return std::visit(Overload
    {
        [](const Ground&)  { return true; },
        [](const Mesh&)    { return true; },
        [](const Body&)    { return true; },
        [](const Joint&)   { return false; },
        [](const StationEl&) { return false; },
    }, e.toVariant());
}

const std::array<MIClass, std::variant_size_v<SceneElVariant>>& osc::mi::GetSceneElClasses()
{
    static const auto s_Classes = std::to_array(
    {
        Ground::Class(),
        Mesh::Class(),
        Body::Class(),
        Joint::Class(),
        StationEl::Class(),
    });
    return s_Classes;
}

Vector3 osc::mi::AverageCenter(const Mesh& el)
{
    const Vector3 centerpointInModelSpace = average_centroid_of(el.getMeshData());
    return el.getXForm() * centerpointInModelSpace;
}

Vector3 osc::mi::mass_center_of(const Mesh& el)
{
    const Vector3 massCenterInModelSpace = mass_center_of(el.getMeshData());
    return el.getXForm() * massCenterInModelSpace;
}
