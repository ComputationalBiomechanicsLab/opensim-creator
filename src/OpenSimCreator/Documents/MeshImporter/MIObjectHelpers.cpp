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

bool osc::mi::CanAttachMeshTo(MIObject const& e)
{
    return std::visit(Overload
    {
        [](Ground const&)  { return true; },
        [](Mesh const&)    { return false; },
        [](Body const&)    { return true; },
        [](Joint const&)   { return true; },
        [](StationEl const&) { return false; },
    }, e.toVariant());
}

bool osc::mi::CanAttachStationTo(MIObject const& e)
{
    return std::visit(Overload
    {
        [](Ground const&)  { return true; },
        [](Mesh const&)    { return true; },
        [](Body const&)    { return true; },
        [](Joint const&)   { return false; },
        [](StationEl const&) { return false; },
    }, e.toVariant());
}

std::array<MIClass, std::variant_size_v<SceneElVariant>> const& osc::mi::GetSceneElClasses()
{
    static auto const s_Classes = std::to_array(
    {
        Ground::Class(),
        Mesh::Class(),
        Body::Class(),
        Joint::Class(),
        StationEl::Class(),
    });
    return s_Classes;
}

Vec3 osc::mi::AverageCenter(Mesh const& el)
{
    Vec3 const centerpointInModelSpace = AverageCenterpoint(el.getMeshData());
    return el.getXForm() * centerpointInModelSpace;
}

Vec3 osc::mi::MassCenter(Mesh const& el)
{
    Vec3 const massCenterInModelSpace = MassCenter(el.getMeshData());
    return el.getXForm() * massCenterInModelSpace;
}
