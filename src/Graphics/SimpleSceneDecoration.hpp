#pragma once

#include "src/Graphics/Color.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Maths/Transform.hpp"

namespace osc
{
    struct SimpleSceneDecoration final {

        SimpleSceneDecoration(
            Mesh const& mesh_,
            Transform const& transform_,
            Color const& color_) :

            mesh{mesh_},
            transform{transform_},
            color{color_}
        {
        }

        Mesh mesh;
        Transform transform;
        Color color;
    };
}