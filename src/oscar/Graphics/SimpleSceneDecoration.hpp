#pragma once

#include "oscar/Graphics/Color.hpp"
#include "oscar/Graphics/Mesh.hpp"
#include "oscar/Maths/Transform.hpp"

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