#pragma once

#include <liboscar/Graphics/Color.h>
#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Graphics/Scene/SceneDecorationFlags.h>
#include <liboscar/Graphics/Scene/SceneDecorationShading.h>
#include <liboscar/Maths/AABB.h>
#include <liboscar/Maths/Transform.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Utils/StringName.h>

namespace osc
{
    // a single render-able decoration element in the scene
    struct SceneDecoration final {

        friend bool operator==(const SceneDecoration&, const SceneDecoration&) = default;

        // returns a copy of this `SceneDecoration` with `position` set the provided position
        SceneDecoration with_translation(const Vector3& position_) const
        {
            SceneDecoration copy{*this};
            copy.transform.translation = position_;
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

        // Returns `true` if this `SceneDecoration` has the given flag set in its `flags` field.
        bool has_flag(SceneDecorationFlag flag) const { return flags.get(flag); }

        // Returns the world-space bounds of this `SceneDecoration`
        std::optional<AABB> world_space_bounds() const;

        Mesh mesh{};
        Transform transform{};
        SceneDecorationShading shading = Color::white();
        StringName id{};
        SceneDecorationFlags flags = SceneDecorationFlag::Default;
    };
}
