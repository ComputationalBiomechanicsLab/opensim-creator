#pragma once

#include <liboscar/utilities/flags.h>

namespace osc
{
    // a flag associated with a `SceneDecoration`
    enum class SceneDecorationFlag : unsigned {
        None = 0,

        // the decoration should not cast shadows
        NoCastsShadows = 1 << 0,
        // the decoration should not be drawn in the scene (it still can cast shadows, show its
        // wireframe, etc.)
        NoDrawInScene = 1 << 1,
        // the decoration should be highlighted with a colored rim (group #0)
        RimHighlight0 = 1 << 2,
        // the decoration should be highlighted with a colored rim (group #1)
        RimHighlight1 = 1 << 3,
        // the decoration's wireframe should additionally be drawn in the scene
        DrawWireframeOverlay = 1 << 4,

        // The decoration should not contribute to the scene's functional volume.
        //
        // This is useful when a graphics backend emits `SceneDecoration`s that need
        // to be visible to the user, but shouldn't be used to (e.g.) figure out how
        // big the scene is in order to auto-focus a camera or similar (opensim-creator#1071).
        NoSceneVolumeContribution = 1 << 5,

        // the decoration can be backface-culled (e.g. because it's using a "trusted" mesh)
        CanBackfaceCull = 1 << 6,

        // helpful combinations
        Default = None,
        WireframeOverlayedDefault = Default | DrawWireframeOverlay,
        AnnotationElement = NoCastsShadows | NoSceneVolumeContribution,
        OnlyWireframe = NoDrawInScene | DrawWireframeOverlay,
        Hidden = NoCastsShadows | NoDrawInScene,
        AllRimHighlightGroups = RimHighlight0 | RimHighlight1,
    };

    // a set of flags associated with a `SceneDecoration`
    using SceneDecorationFlags = Flags<SceneDecorationFlag>;
}
