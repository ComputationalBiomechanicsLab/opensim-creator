#pragma once

#include "src/Graphics/Mesh.hpp"
#include "src/Maths/Transform.hpp"

#include <glm/vec4.hpp>

#include <memory>

namespace osc
{
    class BasicSceneElement final {
    public:
        Transform transform;
        glm::vec4 color;
        std::shared_ptr<Mesh> mesh;
    };
}
