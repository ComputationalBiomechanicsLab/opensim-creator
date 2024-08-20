#pragma once

#include <OpenSimCreator/Graphics/OverlayDecorationGenerator.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.h>

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneHelpers.h>

#include <functional>

namespace osc
{
    // append decorations that are common to all panels to the given output vector
    void AppendCommonDecorations(
        MeshWarpingTabSharedState& sharedState,
        const Mesh& tpsSourceOrDestinationMesh,
        bool wireframeMode,
        const std::function<void(SceneDecoration&&)>& out,
        Color meshColor = Color::white())
    {
        // draw the mesh
        out(SceneDecoration{
            .mesh = tpsSourceOrDestinationMesh,
            .shading = meshColor,
            .flags = wireframeMode ? SceneDecorationFlags{SceneDecorationFlag::Default, SceneDecorationFlag::DrawWireframeOverlay} : SceneDecorationFlag::Default,
        });

        // add overlay decorations
        GenerateOverlayDecorations(
            sharedState.updSceneCache(),
            sharedState.getOverlayDecorationOptions(),
            BVH{},  // TODO: should have a scene BVH by this point
            1.0f,  // scale factor
            out
        );
    }

    // returns the amount by which non-participating landmarks should be scaled w.r.t. pariticpating ones
    constexpr float GetNonParticipatingLandmarkScaleFactor()
    {
        return 0.75f;
    }
}
