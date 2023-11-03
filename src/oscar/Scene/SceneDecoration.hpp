#pragma once

#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/MaterialPropertyBlock.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Scene/SceneDecorationFlags.hpp>
#include <oscar/Scene/SimpleSceneDecoration.hpp>

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

        explicit SceneDecoration(SimpleSceneDecoration&& dec) noexcept :
            mesh{std::move(dec.mesh)},
            transform{std::move(dec.transform)},
            color{std::move(dec.color)}
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

        friend bool operator==(SceneDecoration const&, SceneDecoration const&) = default;

        Mesh mesh;
        Transform transform{};
        Color color = Color::white();
        std::string id;
        SceneDecorationFlags flags = SceneDecorationFlags::None;
        std::optional<Material> maybeMaterial;
        std::optional<MaterialPropertyBlock> maybeMaterialProps;
    };
}
