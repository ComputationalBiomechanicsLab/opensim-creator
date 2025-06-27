#pragma once

#include <liboscar/Graphics/Color.h>
#include <liboscar/Graphics/Material.h>
#include <liboscar/Graphics/MaterialPropertyBlock.h>

#include <variant>

namespace osc
{
    // the choice of shading that a `SceneRenderer` should use when drawing a `SceneDecoration`
    using SceneDecorationShading = std::variant<

        // draws the `SceneDecoration` with this `Color` using an implementation-defined `Material`
        Color,

        // draws the `SceneDecoration` with this `Material`
        //
        // note: the vertex shader for the `Material` should apply the decoration's transform
        //       to the decoration's mesh "normally" (`SceneRenderer` relies on knowing the
        //       world space mesh bounds etc. for various calculations)
        Material,

        // draws the `SceneDecoration` with this `(Material, MaterialPropertyBlock)` pair
        //
        // note: the vertex shader for the `Material` should apply the decoration's transform
        //       to the decoration's mesh "normally" (`SceneRenderer` relies on knowing the
        //       world space mesh bounds etc. for various calculations)
        std::pair<Material, MaterialPropertyBlock>
    >;
}
