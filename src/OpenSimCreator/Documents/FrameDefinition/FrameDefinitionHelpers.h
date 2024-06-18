#pragma once

#include <OpenSimCreator/Documents/CustomComponents/EdgePoints.h>

#include <SimTKcommon/internal/DecorativeGeometry.h>
#include <SimTKcommon/internal/Transform.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/CStringView.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace OpenSim { class Appearance; }
namespace OpenSim { class Component; }
namespace OpenSim { class Frame; }
namespace osc { struct ModelRendererParams; }
namespace osc { class UndoableModelStatePair; }
namespace SimTK { class State; }

namespace osc::fd
{
    constexpr CStringView c_TabStringID = "OpenSim/FrameDefinition";
    constexpr double c_SphereDefaultRadius = 0.01;
    constexpr Color c_SphereDefaultColor = {1.0f, 1.0f, 0.75f};
    constexpr Color c_MidpointDefaultColor = {0.75f, 1.0f, 1.0f};
    constexpr Color c_PointToPointEdgeDefaultColor = {0.75f, 1.0f, 1.0f};
    constexpr Color c_CrossProductEdgeDefaultColor = {0.75f, 1.0f, 1.0f};

    // returns the ground-based location re-expressed w.r.t. the given frame
    SimTK::Vec3 CalcLocationInFrame(
        const OpenSim::Frame&,
        const SimTK::State&,
        const Vec3& locationInGround
    );

    // sets the appearance of `geometry` (SimTK) from `appearance` (OpenSim)
    void SetGeomAppearance(
        SimTK::DecorativeGeometry&,
        const OpenSim::Appearance&
    );

    // sets the color and opacity of `appearance` from `color`
    void SetColorAndOpacity(
        OpenSim::Appearance&,
        const Color&
    );

    // returns a decorative sphere with `radius`, `position`, and `appearance`
    SimTK::DecorativeSphere CreateDecorativeSphere(
        double radius,
        const SimTK::Vec3& position,
        const OpenSim::Appearance&
    );

    // returns a decorative arrow between `startPosition` and `endPosition` with `appearance`
    SimTK::DecorativeArrow CreateDecorativeArrow(
        const SimTK::Vec3& startPosition,
        const SimTK::Vec3& endPosition,
        const OpenSim::Appearance&
    );

    // returns a decorative frame based on the provided transform
    SimTK::DecorativeFrame CreateDecorativeFrame(
        const SimTK::Transform& transformInGround
    );

    // returns a SimTK::DecorativeMesh reperesentation of the parallelogram formed between
    // two (potentially disconnected) edges, starting at `origin`
    SimTK::DecorativeMesh CreateParallelogramMesh(
        const SimTK::Vec3& origin,
        const SimTK::Vec3& firstEdge,
        const SimTK::Vec3& secondEdge,
        const OpenSim::Appearance& appearance
    );

    // custom helper that customizes the OpenSim model defaults to be more
    // suitable for the frame definition UI
    std::shared_ptr<UndoableModelStatePair> MakeSharedUndoableFrameDefinitionModel();

    // gets the next unique suffix numer for geometry
    int32_t GetNextGlobalGeometrySuffix();

    // returns a scene element name that should have a unique name
    std::string GenerateSceneElementName(std::string_view prefix);

    // returns an appropriate commit message for adding `somethingName` to a model
    std::string GenerateAddedSomethingCommitMessage(std::string_view somethingName);

    // mutates the given render params to match the style of the frame definition UI
    void SetupDefault3DViewportRenderingParams(ModelRendererParams&);

    // returns `true` if the given component is a point in the frame definition scene
    bool IsPoint(const OpenSim::Component&);

    // returns `true` if the given component is a mesh in the frame definition scene
    bool IsMesh(const OpenSim::Component&);

    // returns `true` if the given component is a frame in the frame definition scene
    bool IsPhysicalFrame(const OpenSim::Component&);

    // returns `true` if the given component is an edge
    bool IsEdge(const OpenSim::Component&);

    // returns the direction vector between the `start` and `end` points
    SimTK::UnitVec3 CalcDirection(const EdgePoints&);

    // returns points for an edge that:
    //
    // - originates at `a.start`
    // - points in the direction of `a x b`
    // - has a magnitude of min(|a|, |b|) - handy for rendering
    EdgePoints CrossProduct(const EdgePoints&, const EdgePoints&);
}
