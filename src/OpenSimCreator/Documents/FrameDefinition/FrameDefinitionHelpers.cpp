#include "FrameDefinitionHelpers.h"

#include <OpenSimCreator/Documents/CustomComponents/Edge.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Graphics/ModelRendererParams.h>
#include <OpenSimCreator/Utils/SimTKHelpers.h>

#include <Simbody.h>
#include <OpenSim/Simulation/Model/Appearance.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Vec3.h>

#include <array>
#include <atomic>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

using osc::fd::EdgePoints;
using namespace osc;

// returns the ground-based location re-expressed w.r.t. the given frame
SimTK::Vec3 osc::fd::CalcLocationInFrame(
    OpenSim::Frame const& frame,
    SimTK::State const& state,
    Vec3 const& locationInGround)
{
    SimTK::Vec3 const translationInGround = ToSimTKVec3(locationInGround);
    return frame.getTransformInGround(state).invert() * translationInGround;
}

void osc::fd::SetGeomAppearance(
    SimTK::DecorativeGeometry& geometry,
    OpenSim::Appearance const& appearance)
{
    geometry.setColor(appearance.get_color());
    geometry.setOpacity(appearance.get_opacity());
    if (appearance.get_visible())
    {
        geometry.setRepresentation(appearance.get_representation());
    }
    else
    {
        geometry.setRepresentation(SimTK::DecorativeGeometry::Hide);
    }
}

void osc::fd::SetColorAndOpacity(
    OpenSim::Appearance& appearance,
    Color const& color)
{
    appearance.set_color(ToSimTKRGBVec3(color));
    appearance.set_opacity(color.a);
}

SimTK::DecorativeSphere osc::fd::CreateDecorativeSphere(
    double radius,
    SimTK::Vec3 const& position,
    OpenSim::Appearance const& appearance)
{
    SimTK::DecorativeSphere sphere{radius};
    sphere.setTransform(SimTK::Transform{position});
    SetGeomAppearance(sphere, appearance);
    return sphere;
}

SimTK::DecorativeArrow osc::fd::CreateDecorativeArrow(
    SimTK::Vec3 const& startPosition,
    SimTK::Vec3 const& endPosition,
    OpenSim::Appearance const& appearance)
{
    SimTK::DecorativeArrow arrow{startPosition, endPosition, 1.75 * c_SphereDefaultRadius};
    arrow.setLineThickness(0.5 * c_SphereDefaultRadius);
    SetGeomAppearance(arrow, appearance);
    return arrow;
}

SimTK::DecorativeFrame osc::fd::CreateDecorativeFrame(
    SimTK::Transform const& transformInGround)
{
    // adapted from OpenSim::FrameGeometry (Geometry.cpp)
    SimTK::DecorativeFrame frame(1.0);
    frame.setTransform(transformInGround);
    frame.setScale(0.2);
    frame.setLineThickness(0.004);
    return frame;
}

SimTK::DecorativeMesh osc::fd::CreateParallelogramMesh(
    SimTK::Vec3 const& origin,
    SimTK::Vec3 const& firstEdge,
    SimTK::Vec3 const& secondEdge,
    OpenSim::Appearance const& appearance)
{
    SimTK::PolygonalMesh polygonalMesh;
    {
        auto const vertices = std::to_array(
        {
            origin,
            origin + firstEdge,
            origin + firstEdge + secondEdge,
            origin + secondEdge,
        });

        SimTK::Array_<int> face;
        face.reserve(static_cast<decltype(face)::size_type>(vertices.size()));
        for (SimTK::Vec3 const& vert : vertices)
        {
            face.push_back(polygonalMesh.addVertex(vert));
        }
        polygonalMesh.addFace(face);
    }

    SimTK::DecorativeMesh rv{polygonalMesh};
    SetGeomAppearance(rv, appearance);
    return rv;
}

std::shared_ptr<UndoableModelStatePair> osc::fd::MakeSharedUndoableFrameDefinitionModel()
{
    auto model = std::make_unique<OpenSim::Model>();
    model->updDisplayHints().set_show_frames(true);
    return std::make_shared<UndoableModelStatePair>(std::move(model));
}

// gets the next unique suffix numer for geometry
int32_t osc::fd::GetNextGlobalGeometrySuffix()
{
    static std::atomic<int32_t> s_GeometryCounter = 0;
    return s_GeometryCounter++;
}

std::string osc::fd::GenerateSceneElementName(std::string_view prefix)
{
    std::stringstream ss;
    ss << prefix;
    ss << GetNextGlobalGeometrySuffix();
    return std::move(ss).str();
}

std::string osc::fd::GenerateAddedSomethingCommitMessage(std::string_view somethingName)
{
    std::string_view const prefix = "added ";
    std::string rv;
    rv.reserve(prefix.size() + somethingName.size());
    rv += prefix;
    rv += somethingName;
    return rv;
}

void osc::fd::SetupDefault3DViewportRenderingParams(ModelRendererParams& renderParams)
{
    renderParams.renderingOptions.setDrawFloor(false);
    renderParams.overlayOptions.setDrawXZGrid(true);
    renderParams.backgroundColor = {48.0f/255.0f, 48.0f/255.0f, 48.0f/255.0f, 1.0f};
}

bool osc::fd::IsPoint(OpenSim::Component const& component)
{
    return dynamic_cast<OpenSim::Point const*>(&component) != nullptr;
}

bool osc::fd::IsMesh(OpenSim::Component const& component)
{
    return dynamic_cast<OpenSim::Mesh const*>(&component) != nullptr;
}

bool osc::fd::IsPhysicalFrame(OpenSim::Component const& component)
{
    return dynamic_cast<OpenSim::PhysicalFrame const*>(&component) != nullptr;
}

bool osc::fd::IsEdge(OpenSim::Component const& component)
{
    return dynamic_cast<Edge const*>(&component) != nullptr;
}

SimTK::UnitVec3 osc::fd::CalcDirection(EdgePoints const& a)
{
    return SimTK::UnitVec3{a.end - a.start};
}

EdgePoints osc::fd::CrossProduct(EdgePoints const& a, EdgePoints const& b)
{
    // TODO: if cross product isn't possible (e.g. angle between vectors is zero)
    // then this needs to fail or fallback
    SimTK::Vec3 const firstEdge = a.end - a.start;
    SimTK::Vec3 const secondEdge = b.end - b.start;
    SimTK::Vec3 const resultEdge = SimTK::cross(firstEdge, secondEdge).normalize();
    double const resultEdgeLength = min(firstEdge.norm(), secondEdge.norm());

    return {a.start, a.start + (resultEdgeLength*resultEdge)};
}
