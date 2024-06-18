#include "MIObjectHelpers.h"

#include <OpenSimCreator/Documents/MeshImporter/Body.h>
#include <OpenSimCreator/Documents/MeshImporter/Ground.h>
#include <OpenSimCreator/Documents/MeshImporter/Joint.h>
#include <OpenSimCreator/Documents/MeshImporter/MIClass.h>
#include <OpenSimCreator/Documents/MeshImporter/MIObject.h>
#include <OpenSimCreator/Documents/MeshImporter/Mesh.h>
#include <OpenSimCreator/Documents/MeshImporter/Station.h>

#include <oscar/Graphics/MeshFunctions.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/StdVariantHelpers.h>

#include <array>
#include <variant>

using osc::mi::MIClass;
using osc::mi::SceneElVariant;
using osc::Vec3;

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

std::array<MIClass, std::variant_size_v<SceneElVariant>> const& osc::mi::GetSceneElClasses()
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

Vec3 osc::mi::AverageCenter(const Mesh& el)
{
    const Vec3 centerpointInModelSpace = average_centroid_of(el.getMeshData());
    return el.getXForm() * centerpointInModelSpace;
}

Vec3 osc::mi::mass_center_of(const Mesh& el)
{
    const Vec3 massCenterInModelSpace = mass_center_of(el.getMeshData());
    return el.getXForm() * massCenterInModelSpace;
}
