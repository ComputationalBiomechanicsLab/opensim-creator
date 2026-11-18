#pragma once

#include <libopensimcreator/ui/mesh_warper/mw_ui_shared_state.h>

#include <libopynsim/graphics/overlay_decoration_generator.h>
#include <liboscar/graphics/color.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/graphics/scene/scene_decoration.h>
#include <liboscar/graphics/scene/scene_helpers.h>

#include <functional>

namespace osc
{
    // append decorations that are common to all panels to the given output vector
    inline void append_common_decorations(
        MwUiSharedState& shared_state,
        const Mesh& tps_source_or_destination_mesh,
        bool wireframe_mode,
        const std::function<void(SceneDecoration&&)>& out,
        Color mesh_color = Color::white())
    {
        // draw the mesh
        out(SceneDecoration{
            .mesh = tps_source_or_destination_mesh,
            .shading = mesh_color,
            .flags =
                wireframe_mode
                    ? SceneDecorationFlags{SceneDecorationFlag::Default, SceneDecorationFlag::DrawWireframeOverlay}
                    : SceneDecorationFlag::Default,
        });

        // add overlay decorations
        generate_overlay_decorations(
            shared_state.upd_scene_cache(),
            shared_state.get_overlay_decoration_options(),
            BVH{},  // TODO: should have a scene BVH by this point
            1.0f,   // scale factor
            out);
    }

    // returns the amount by which non-participating landmarks should be scaled w.r.t. pariticpating
    // ones
    constexpr float get_non_participating_landmark_scale_factor()
    {
        return 0.75f;
    }
}
