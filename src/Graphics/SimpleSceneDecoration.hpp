#pragma once

#include "src/Graphics/Mesh.hpp"
#include "src/Maths/Transform.hpp"

#include <glm/vec4.hpp>

namespace osc
{
    struct SimpleSceneDecoration final {

        SimpleSceneDecoration(
            Mesh const& mesh_,
            Transform const& transform_,
            glm::vec4 const& color_) :

            mesh{mesh_},
            transform{transform_},
            color{color_}
        {
        }

        Mesh mesh;
        Transform transform;
        glm::vec4 color;
    };
}