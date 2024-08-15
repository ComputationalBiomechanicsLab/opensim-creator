#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/MaterialPropertyBlock.h>

#include <variant>

namespace osc
{
    // the choice of shading that should be used when drawing a `SceneDecroation` in the scene
    using SceneDecorationShading = std::variant<

        // draw the decoration with this `Color` using an implementation-defined `Material`
        Color,

        // draw the decoration with this `Material`
        //
        // note: the vertex shader for the `Material` should apply the decoration's transform
        //       to the decoration's mesh "normally" (`SceneRenderer` relies on knowing the
        //       worldspace mesh bounds etc. for various calculations)
        Material,

        // draw the decoration with this `(Material, MaterialPropertyBlock)` pair
        //
        // note: the vertex shader for the `Material` should apply the decoration's transform
        //       to the decoration's mesh "normally" (`SceneRenderer` relies on knowing the
        //       worldspace mesh bounds etc. for various calculations)
        std::pair<Material, MaterialPropertyBlock>
    >;
}
