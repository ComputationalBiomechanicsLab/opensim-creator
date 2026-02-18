#pragma once

#include <libopensimcreator/ui/mesh_warper/mesh_warping_tab_shared_state.h>

#include <libopynsim/graphics/overlay_decoration_generator.h>
#include <liboscar/graphics/color.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/graphics/scene/scene_helpers.h>

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
