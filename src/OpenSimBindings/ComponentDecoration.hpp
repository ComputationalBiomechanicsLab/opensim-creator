#pragma once

#include "src/Graphics/Mesh.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/Transform.hpp"

#include <glm/vec4.hpp>

#include <memory>

namespace OpenSim
{
    class Component;
}

namespace osc
{
    // represents a renderable decoration for a component in a model
    struct ComponentDecoration {
        std::shared_ptr<Mesh> mesh;
        Transform transform;
        glm::vec4 color;
        AABB worldspaceAABB;
        OpenSim::Component const* component;

        ComponentDecoration(std::shared_ptr<Mesh>,
                            Transform const&,
                            glm::vec4 const& color,
                            OpenSim::Component const*);
    };
}
