#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/MaterialPropertyBlock.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/Scene/SceneDecorationFlags.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Vec3.h>

#include <optional>
#include <string>

namespace osc
{
    // represents a renderable decoration for a component in a model
    struct SceneDecoration final {

        friend bool operator==(const SceneDecoration&, const SceneDecoration&) = default;

        SceneDecoration with_position(const Vec3& position_) const
        {
            SceneDecoration copy{*this};
            copy.transform.position = position_;
            return copy;
        }

        SceneDecoration with_transform(const Transform& transform_) const
        {
            SceneDecoration copy{*this};
            copy.transform = transform_;
            return copy;
        }

        SceneDecoration with_color(const Color& color_) const
        {
            SceneDecoration copy{*this};
            copy.color = color_;
            return copy;
        }

        Mesh mesh;
        Transform transform{};
        Color color = Color::white();
        std::string id;
        SceneDecorationFlags flags = SceneDecorationFlags::None;
        std::optional<Material> maybe_material;
        std::optional<MaterialPropertyBlock> maybe_material_props;
    };
}
