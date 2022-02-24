#pragma once

#include "src/3D/Mesh.hpp"
#include "src/3D/Model.hpp"

#include <glm/mat4x3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec4.hpp>

namespace OpenSim
{
    class Component;
}

namespace osc
{
    struct ComponentDecoration {
        std::shared_ptr<Mesh> mesh;
        glm::mat4x3 modelMtx;
        glm::mat3 normalMtx;
        glm::vec4 color;
        AABB worldspaceAABB;
        OpenSim::Component const* component;

        ComponentDecoration(std::shared_ptr<Mesh>,
                            Transform const&,
                            glm::vec4 const&,
                            OpenSim::Component const*);
    };
}
