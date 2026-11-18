#pragma once

#include <liboscar/graphics/color.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/graphics/scene/scene_decoration_flags.h>
#include <liboscar/graphics/scene/scene_decoration_shading.h>
#include <liboscar/maths/aabb.h>
#include <liboscar/maths/transform.h>
#include <liboscar/maths/vector.h>
#include <liboscar/utilities/string_name.h>

namespace osc
{
    // a single render-able decoration element in the scene
    struct SceneDecoration final {

        friend bool operator==(const SceneDecoration&, const SceneDecoration&) = default;

        // returns a copy of this `SceneDecoration` with `translation` set the provided point.
        SceneDecoration with_translation(const Vector3& translation) const
        {
            SceneDecoration copy{*this};
            copy.transform.translation = translation;
            return copy;
        }

        // returns a copy of this `SceneDecoration` with `transform` set to the provided transform
        SceneDecoration with_transform(const Transform& new_transform) const
        {
            SceneDecoration copy{*this};
            copy.transform = new_transform;
            return copy;
        }

        // returns a copy of this `SceneDecoration` with `shading` set to the provided color
        SceneDecoration with_color(const Color& color) const
        {
            SceneDecoration copy{*this};
            copy.shading = color;
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
