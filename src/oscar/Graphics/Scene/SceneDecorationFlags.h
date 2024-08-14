#pragma once

#include <oscar/Utils/Flags.h>

namespace osc
{
    // a flag associated with a `SceneDecoration`
    enum class SceneDecorationFlag {
        None                 = 0,
        RimHighlight0        = 1<<0,  // the decoration should be highlighted with a colored rim (group #0)
        RimHighlight1        = 1<<1,  // the decoration should be highlighted with a colored rim (group #1)
        CastsShadows         = 1<<2,  // casts shadows on other elements in the scene
        DrawWireframeOverlay = 1<<3,  // in addition to everything else, draw the geometry as a wireframe
        NoDrawNormally       = 1<<4,  // disable drawing "normally" (i.e. in the scene) but still draw wireframes, cast shadows, rims, etc.

        Default = CastsShadows,
    };

    // a set of flags associated with a `SceneDecoration`
    using SceneDecorationFlags = Flags<SceneDecorationFlag>;
}
