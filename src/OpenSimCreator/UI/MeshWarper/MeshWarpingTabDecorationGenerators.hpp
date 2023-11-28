#pragma once

#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.hpp>

#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Scene/SceneDecoration.hpp>
#include <oscar/Scene/SceneHelpers.hpp>

#include <functional>
#include <utility>

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
        {
            SceneDecoration dec{tpsSourceOrDestinationMesh};
            dec.color = meshColor;
            out(std::move(dec));
        }

        // if requested, also draw wireframe overlays for the mesh
        if (wireframeMode)
        {
            SceneDecoration dec{tpsSourceOrDestinationMesh};
            dec.maybeMaterial = sharedState.wireframeMaterial;
            out(std::move(dec));
        }

        // add grid decorations
        DrawXZGrid(*sharedState.meshCache, out);
        DrawXZFloorLines(*sharedState.meshCache, out, 100.0f);
    }

    // returns the amount by which non-participating landmarks should be scaled w.r.t. pariticpating ones
    constexpr float GetNonParticipatingLandmarkScaleFactor()
    {
        return 0.75f;
    }
}
