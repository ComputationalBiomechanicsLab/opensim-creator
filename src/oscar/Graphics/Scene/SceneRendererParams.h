#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>

namespace osc
{
    struct SceneRendererParams final {

        static constexpr Color default_light_color()
        {
            return {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
        }

        static constexpr Color default_background_color()
        {
            return {0.89f, 0.89f, 0.89f};
        }

        static constexpr Vec3 default_floor_location()
        {
            return {0.0f, -0.001f, 0.0f};
        }

        SceneRendererParams();

        friend bool operator==(const SceneRendererParams&, const SceneRendererParams&) = default;

        Vec2i dimensions;
        AntiAliasingLevel antialiasing_level;
        bool draw_mesh_normals;
        bool draw_rims;
        bool draw_shadows;
        bool draw_floor;
        float near_clipping_plane;
        float far_clipping_plane;
        Mat4 view_matrix;
        Mat4 projection_matrix;
        Vec3 view_pos;
        Vec3 light_direction;
        Color light_color;  // ignores alpha
        float ambient_strength;
        float diffuse_strength;
        float specular_strength;
        float specular_shininess;
        Color background_color;
        Color rim_color;
        Vec2 rim_thickness_in_pixels;
        Vec3 floor_location;
        float fixup_scale_factor;
    };
}
