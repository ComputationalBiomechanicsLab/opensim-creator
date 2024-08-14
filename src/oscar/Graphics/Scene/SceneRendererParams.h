#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>

#include <array>

namespace osc
{
    // the parameters associated with a single call to `SceneRenderer::render`
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

    private:
        static inline constexpr size_t c_num_rim_groups = 2;
    public:
        static constexpr size_t num_rim_groups()
        {
            return c_num_rim_groups;
        }

        friend bool operator==(const SceneRendererParams&, const SceneRendererParams&) = default;

        // output parameters
        Vec2i dimensions = {1, 1};
        AntiAliasingLevel antialiasing_level = AntiAliasingLevel::none();

        // flags
        bool draw_mesh_normals = false;
        bool draw_rims = true;
        bool draw_shadows = true;
        bool draw_floor = true;

        // camera parameters
        float near_clipping_plane = 0.1f;
        float far_clipping_plane = 100.0f;
        Mat4 view_matrix = identity<Mat4>();
        Mat4 projection_matrix = identity<Mat4>();
        Vec3 view_pos = {0.0f, 0.0f, 0.0f};

        // shading parameters
        Vec3 light_direction = {-0.34f, -0.25f, 0.05f};
        Color light_color = default_light_color();
        float ambient_strength = 0.01f;
        float diffuse_strength = 0.55f;
        float specular_strength = 0.7f;
        float specular_shininess = 6.0f;
        Color background_color = default_background_color();
        std::array<Color, c_num_rim_groups> rim_group_colors = std::to_array<Color>({
            Color{0.95f, 0.35f, 0.0f, 0.95f},
            Color{0.95f, 0.35f, 0.0f, 0.35f},
        });
        Vec2 rim_thickness_in_pixels = {1.0f, 1.0f};

        // scene parameters
        Vec3 floor_location = default_floor_location();
        float fixup_scale_factor = 1.0f;
    };
}
