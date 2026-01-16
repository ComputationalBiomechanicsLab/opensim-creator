#pragma once

#include <liboscar/graphics/color.h>
#include <liboscar/graphics/material.h>
#include <liboscar/graphics/material_property_block.h>

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
