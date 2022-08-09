#pragma once

#include "src/Graphics/Renderer.hpp"
#include "src/Graphics/SceneDecorationFlags.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/Transform.hpp"

#include <glm/vec4.hpp>

#include <memory>
#include <string>

namespace osc
{
    // represents a renderable decoration for a component in a model
    class SceneDecoration final {
    public:
        std::shared_ptr<experimental::Mesh const> mesh;
        Transform transform;
        glm::vec4 color;
        std::string id;
        SceneDecorationFlags flags = SceneDecorationFlags_None;

        SceneDecoration(std::shared_ptr<experimental::Mesh const> mesh_,
                        Transform const& transform_,
                        glm::vec4 const& color_) :
            mesh{std::move(mesh_)},
            transform{transform_},
            color{color_}
        {
        }

        SceneDecoration(std::shared_ptr<experimental::Mesh const> mesh_,
                        Transform const& transform_,
                        glm::vec4 const& color_,
                        std::string id_,
                        SceneDecorationFlags flags_) :
            mesh{std::move(mesh_)},
            transform{transform_},
            color{color_},
            id{std::move(id_)},
            flags{std::move(flags_)}
        {
        }
    };

    AABB GetWorldspaceAABB(SceneDecoration const&);
}
