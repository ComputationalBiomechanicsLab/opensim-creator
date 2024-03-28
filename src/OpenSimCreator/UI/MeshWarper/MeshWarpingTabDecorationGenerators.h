#pragma once

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
        MeshWarpingTabSharedState const& sharedState,
        Mesh const& tpsSourceOrDestinationMesh,
        bool wireframeMode,
        std::function<void(SceneDecoration&&)> const& out,
        Color meshColor = Color::white())
    {
        // draw the mesh
        out({
            .mesh = tpsSourceOrDestinationMesh,
            .color = meshColor,
        });

        // if requested, also draw wireframe overlays for the mesh
        if (wireframeMode)
        {
            out({
                .mesh = tpsSourceOrDestinationMesh,
                .maybe_material = sharedState.wireframe_material,
            });
        }

        // add grid decorations
        draw_xz_grid(*sharedState.meshCache, out);
        draw_xz_floor_lines(*sharedState.meshCache, out, 100.0f);
    }

    // returns the amount by which non-participating landmarks should be scaled w.r.t. pariticpating ones
    constexpr float GetNonParticipatingLandmarkScaleFactor()
    {
        return 0.75f;
    }
}
