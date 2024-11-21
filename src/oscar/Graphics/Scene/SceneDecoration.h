#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/Scene/SceneDecorationFlags.h>
#include <oscar/Graphics/Scene/SceneDecorationShading.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Utils/StringName.h>

#include <variant>

namespace osc
{
    // a single render-able decoration element in the scene
    struct SceneDecoration final {

        friend bool operator==(const SceneDecoration&, const SceneDecoration&) = default;

        // returns a copy of this `SceneDecoration` with `position` set the provided position
        SceneDecoration with_position(const Vec3& position_) const
        {
            SceneDecoration copy{*this};
            copy.transform.position = position_;
            return copy;
        }

        // returns a copy of this `SceneDecoration` with `transform` set to the provided transform
        SceneDecoration with_transform(const Transform& transform_) const
        {
            SceneDecoration copy{*this};
            copy.transform = transform_;
            return copy;
        }

        // returns a copy of this `SceneDecoration` with `shading` set to the provided color
        SceneDecoration with_color(const Color& color_) const
        {
            SceneDecoration copy{*this};
            copy.shading = color_;
            return copy;
        }

        // returns `true` if this `SceneDecoration` is rim highlighted (any group)
        bool is_rim_highlighted() const
        {
            return flags & SceneDecorationFlag::AllRimHighlightGroups;
        }

        Mesh mesh{};
        Transform transform{};
        SceneDecorationShading shading = Color::white();
        StringName id{};
        SceneDecorationFlags flags = SceneDecorationFlag::Default;
    };
}
