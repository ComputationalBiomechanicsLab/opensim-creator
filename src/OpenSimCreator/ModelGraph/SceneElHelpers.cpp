#include "SceneElHelpers.hpp"

#include <OpenSimCreator/ModelGraph/BodyEl.hpp>
#include <OpenSimCreator/ModelGraph/EdgeEl.hpp>
#include <OpenSimCreator/ModelGraph/GroundEl.hpp>
#include <OpenSimCreator/ModelGraph/JointEl.hpp>
#include <OpenSimCreator/ModelGraph/MeshEl.hpp>
#include <OpenSimCreator/ModelGraph/SceneEl.hpp>
#include <OpenSimCreator/ModelGraph/SceneElClass.hpp>
#include <OpenSimCreator/ModelGraph/StationEl.hpp>

#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/VariantHelpers.hpp>

#include <array>
#include <variant>

bool osc::CanAttachMeshTo(SceneEl const& e)
{
    return std::visit(Overload
    {
        [](GroundEl const&)  { return true; },
        [](MeshEl const&)    { return false; },
        [](BodyEl const&)    { return true; },
        [](JointEl const&)   { return true; },
        [](StationEl const&) { return false; },
        [](EdgeEl const&)    { return false; },
    }, e.toVariant());
}

bool osc::CanAttachStationTo(SceneEl const& e)
{
    return std::visit(Overload
    {
        [](GroundEl const&)  { return true; },
        [](MeshEl const&)    { return true; },
        [](BodyEl const&)    { return true; },
        [](JointEl const&)   { return false; },
        [](StationEl const&) { return false; },
        [](EdgeEl const&)    { return false; },
    }, e.toVariant());
}

bool osc::CanAttachEdgeTo(SceneEl const& el)
{
    return std::visit(Overload
    {
        [](GroundEl const&)  { return true; },
        [](MeshEl const&)    { return true; },
        [](BodyEl const&)    { return true; },
        [](JointEl const&)   { return true; },
        [](StationEl const&) { return true; },
        [](EdgeEl const&)    { return false; },
    }, el.toVariant());
}

std::array<osc::SceneElClass, std::variant_size_v<osc::SceneElVariant>> const& osc::GetSceneElClasses()
{
    static auto const s_Classes = std::to_array(
    {
        GroundEl::Class(),
        MeshEl::Class(),
        BodyEl::Class(),
        JointEl::Class(),
        StationEl::Class(),
        EdgeEl::Class(),
    });
    return s_Classes;
}

osc::Vec3 osc::AverageCenter(MeshEl const& el)
{
    Vec3 const centerpointInModelSpace = AverageCenterpoint(el.getMeshData());
    return el.getXForm() * centerpointInModelSpace;
}

osc::Vec3 osc::MassCenter(MeshEl const& el)
{
    Vec3 const massCenterInModelSpace = MassCenter(el.getMeshData());
    return el.getXForm() * massCenterInModelSpace;
}
