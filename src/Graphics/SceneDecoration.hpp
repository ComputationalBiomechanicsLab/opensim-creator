#pragma once

#include "src/Graphics/Color.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/MaterialPropertyBlock.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/SceneDecorationFlags.hpp"
#include "src/Graphics/SimpleSceneDecoration.hpp"
#include "src/Maths/Transform.hpp"

#include <optional>
#include <string>
#include <utility>

namespace osc
{
    // represents a renderable decoration for a component in a model
    struct SceneDecoration final {

        explicit SceneDecoration(Mesh const& mesh_) :
            mesh{mesh_}
        {
        }

        explicit SceneDecoration(SimpleSceneDecoration const& dec) :
            mesh{dec.mesh},
            transform{dec.transform},
            color{dec.color}
        {
        }

        SceneDecoration(
            Mesh const& mesh_,
            Transform const& transform_,
            Color const& color_) :

            mesh{mesh_},
            transform{transform_},
            color{color_}
        {
        }

        SceneDecoration(
            Mesh const& mesh_,
            Transform const& transform_,
            Color const& color_,
            std::string id_,
            SceneDecorationFlags flags_) :

            mesh{mesh_},
            transform{transform_},
            color{color_},
            id{std::move(id_)},
            flags{std::move(flags_)}
        {
        }

        SceneDecoration(
            Mesh const& mesh_,
            Transform const& transform_,
            Color const& color_,
            std::string id_,
            SceneDecorationFlags flags_,
            std::optional<Material> maybeMaterial_,
            std::optional<MaterialPropertyBlock> maybeProps_ = std::nullopt) :

            mesh{mesh_},
            transform{transform_},
            color{color_},
            id{std::move(id_)},
            flags{std::move(flags_)},
            maybeMaterial{std::move(maybeMaterial_)},
            maybeMaterialProps{std::move(maybeProps_)}
        {
        }

        Mesh mesh;
        Transform transform{};
        Color color = Color::white();
        std::string id;
        SceneDecorationFlags flags = SceneDecorationFlags_None;
        std::optional<Material> maybeMaterial = std::nullopt;
        std::optional<MaterialPropertyBlock> maybeMaterialProps = std::nullopt;
    };

    bool operator==(SceneDecoration const&, SceneDecoration const&) noexcept;
}
