#pragma once

#include <oscar/Utils/Flags.h>

namespace osc
{
    // a flag associated with a `SceneDecoration`
    enum class SceneDecorationFlag : unsigned {
        None                 = 0,
        NoCastsShadows       = 1<<0,  // the decoration should not cast shadows
        NoDrawInScene        = 1<<1,  // the decoration should not be drawn in the scene (still can cast shadows, show as wireframe, etc.)
        RimHighlight0        = 1<<2,  // the decoration should be highlighted with a colored rim (group #0)
        RimHighlight1        = 1<<3,  // the decoration should be highlighted with a colored rim (group #1)
        DrawWireframeOverlay = 1<<4,  // the decoration's wireframe should additionally be drawn in the scene

        // helpful combinations
        Default = None,
        AnnotationElement = NoCastsShadows,
        OnlyWireframe = NoDrawInScene | DrawWireframeOverlay,
        Hidden = NoCastsShadows | NoDrawInScene,
        AllRimHighlightGroups = RimHighlight0 | RimHighlight1,
    };

    // a set of flags associated with a `SceneDecoration`
    using SceneDecorationFlags = Flags<SceneDecorationFlag>;
}
