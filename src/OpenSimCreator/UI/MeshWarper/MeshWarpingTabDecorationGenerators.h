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
        out({
            .mesh = tpsSourceOrDestinationMesh,
            .color = meshColor,
        });

        // if requested, also draw wireframe overlays for the mesh
        if (wireframeMode) {
            out({
                .mesh = tpsSourceOrDestinationMesh,
                .material = sharedState.wireframe_material(),
            });
        }

        // add overlay decorations
        GenerateOverlayDecorations(
            sharedState.updSceneCache(),
            sharedState.getOverlayDecorationOptions(),
            BVH{},  // TODO: should have a scene BVH by this point
            out
        );
    }

    // returns the amount by which non-participating landmarks should be scaled w.r.t. pariticpating ones
    constexpr float GetNonParticipatingLandmarkScaleFactor()
    {
        return 0.75f;
    }
}
