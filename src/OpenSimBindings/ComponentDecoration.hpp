#pragma once

#include "src/3D/Mesh.hpp"
#include "src/3D/Model.hpp"  // AABB, Transform

#include <glm/vec4.hpp>

namespace OpenSim
{
    class Component;
}

namespace osc
{
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
