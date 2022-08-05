#pragma once

#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/SceneDecorationNewFlags.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/Transform.hpp"

#include <glm/vec4.hpp>

#include <memory>
#include <string>

namespace osc
{
    // represents a renderable decoration for a component in a model
    class SceneDecorationNew final {
    public:
        std::shared_ptr<Mesh> mesh;
        Transform transform;
        glm::vec4 color;
        std::string id;
        SceneDecorationNewFlags flags;

        SceneDecorationNew(std::shared_ptr<Mesh> mesh_,
                           Transform const& transform_,
                           glm::vec4 const& color_,
                           std::string id_,
                           SceneDecorationNewFlags flags_) :
            mesh{std::move(mesh_)},
            transform{transform_},
            color{color_},
            id{std::move(id_)},
            flags{std::move(flags_)}
        {
        }
    };

    AABB GetWorldspaceAABB(SceneDecorationNew const&);
}
