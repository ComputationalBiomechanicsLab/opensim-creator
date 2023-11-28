#pragma once

#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/MaterialPropertyBlock.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Scene/SceneDecorationFlags.hpp>

#include <optional>
#include <string>

namespace osc
{
    // represents a renderable decoration for a component in a model
    struct SceneDecoration final {

        friend bool operator==(SceneDecoration const&, SceneDecoration const&) = default;

        SceneDecoration withPosition(Vec3 const& position_) const
        {
            SceneDecoration copy{*this};
            copy.transform.position = position_;
            return copy;
        }

        SceneDecoration withTransform(Transform const& transform_) const
        {
            SceneDecoration copy{*this};
            copy.transform = transform_;
            return copy;
        }

        SceneDecoration withColor(Color const& color_) const
        {
            SceneDecoration copy{*this};
            copy.color = color_;
            return copy;
        }

        Mesh mesh;
        Transform transform;
        Color color = Color::white();
        std::string id;
        SceneDecorationFlags flags = SceneDecorationFlags::None;
        std::optional<Material> maybeMaterial;
        std::optional<MaterialPropertyBlock> maybeMaterialProps;
    };
}
