#include "HittestTab.h"

#include <oscar/oscar.h>
#include <SDL_events.h>

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

using namespace osc;

namespace
{
    constexpr CStringView c_tab_string_id = "Demos/Hittest";

    constexpr auto c_triangle_verts = std::to_array<Vec3>({
        {-10.0f, -10.0f, 0.0f},
        {+0.0f, +10.0f, 0.0f},
        {+10.0f, -10.0f, 0.0f},
    });

    struct SceneSphere final {

        explicit SceneSphere(Vec3 pos_) :
            pos{pos_}
        {}

        Vec3 pos;
        bool is_hovered = false;
    };

    std::vector<SceneSphere> generate_scene_spheres()
    {
        constexpr int32_t min = -30;
        constexpr int32_t max = 30;
        constexpr int32_t step = 6;

        std::vector<SceneSphere> rv;
        for (int32_t x = min; x <= max; x += step) {
            for (int32_t y = min; y <= max; y += step) {
                for (int32_t z = min; z <= max; z += step) {
                    rv.emplace_back(Vec3{
                        static_cast<float>(x),
                        50.0f + 2.0f*static_cast<float>(y),
                        static_cast<float>(z),
                    });
                }
            }
        }
        return rv;
    }

    Mesh generate_crosshair_mesh()
    {
        Mesh rv;
        rv.set_topology(MeshTopology::Lines);
        rv.set_vertices({
            // -X to +X
            {-0.05f, 0.0f, 0.0f},
            {+0.05f, 0.0f, 0.0f},

            // -Y to +Y
            {0.0f, -0.05f, 0.0f},
            {0.0f, +0.05f, 0.0f},
        });
        rv.set_indices({0, 1, 2, 3});
        return rv;
    }

    Mesh generate_triangle_mesh()
    {
        Mesh rv;
        rv.set_vertices(c_triangle_verts);
        rv.set_indices({0, 1, 2});
        return rv;
    }

    Line get_camera_ray(const Camera& camera)
    {
        return {
            camera.position(),
            camera.direction(),
        };
    }
}

class osc::HittestTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_tab_string_id}
    {
        camera_.set_background_color({1.0f, 1.0f, 1.0f, 0.0f});
    }

private:
    void impl_on_mount() final
    {
        App::upd().make_main_loop_polling();
        is_mouse_captured_ = true;
    }

    void impl_on_unmount() final
    {
        is_mouse_captured_ = false;
        App::upd().make_main_loop_waiting();
        App::upd().set_show_cursor(true);
    }

    bool impl_on_event(const SDL_Event& e) final
    {
        if (e.type == SDL_KEYDOWN and e.key.keysym.sym == SDLK_ESCAPE) {
            is_mouse_captured_ = false;
            return true;
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN and ui::is_mouse_in_main_viewport_workspace()) {
            is_mouse_captured_ = true;
            return true;
        }
        return false;
    }

    void impl_on_tick() final
    {
        // hittest spheres

        const Line ray = get_camera_ray(camera_);
        float closest_distance = std::numeric_limits<float>::max();
        SceneSphere* closest_sphere = nullptr;

        for (SceneSphere& ss : scene_spheres_) {
            ss.is_hovered = false;

            Sphere s{
                ss.pos,
                sphere_bounding_sphere_.radius
            };

            std::optional<RayCollision> collision = find_collision(ray, s);
            if (collision and collision->distance >= 0.0f and collision->distance < closest_distance) {
                closest_distance = collision->distance;
                closest_sphere = &ss;
            }
        }

        if (closest_sphere) {
            closest_sphere->is_hovered = true;
        }
    }

    void impl_on_draw() final
    {
        // handle mouse capturing
        if (is_mouse_captured_) {
            ui::update_camera_from_all_inputs(camera_, camera_eulers);
            ui::set_mouse_cursor(ImGuiMouseCursor_None);
            App::upd().set_show_cursor(false);
        }
        else {
            ui::set_mouse_cursor(ImGuiMouseCursor_Arrow);
            App::upd().set_show_cursor(true);
        }

        // render spheres
        for (const SceneSphere& sphere : scene_spheres_) {

            graphics::draw(
                sphere_mesh_,
                {.position = sphere.pos},
                material_,
                camera_,
                sphere.is_hovered ? blue_color_material_props_ : red_color_material_props_
            );

            // draw sphere AABBs
            if (showing_aabbs_) {

                graphics::draw(
                    wireframe_mesh_,
                    {.scale = half_widths_of(scene_sphere_aabb_), .position = sphere.pos},
                    material_,
                    camera_,
                    black_color_material_props_
                );
            }
        }

        // hittest + draw disc
        {
            const Line ray = get_camera_ray(camera_);

            const Disc scene_disc{
                .origin = {0.0f, 0.0f, 0.0f},
                .normal = {0.0f, 1.0f, 0.0f},
                .radius = 10.0f,
            };

            const std::optional<RayCollision> maybe_collision = find_collision(ray, scene_disc);

            const Disc mesh_disc{
                .origin = {0.0f, 0.0f, 0.0f},
                .normal = {0.0f, 0.0f, 1.0f},
                .radius = 1.0f,
            };

            graphics::draw(
                circle_mesh_,
                mat4_transform_between(mesh_disc, scene_disc),
                material_,
                camera_,
                maybe_collision ? blue_color_material_props_ : red_color_material_props_
            );
        }

        // hittest + draw triangle
        {
            const Line ray = get_camera_ray(camera_);
            const std::optional<RayCollision> maybe_collision = find_collision(
                ray,
                Triangle{c_triangle_verts.at(0), c_triangle_verts.at(1), c_triangle_verts.at(2)}
            );

            graphics::draw(
                triangle_mesh_,
                identity<Transform>(),
                material_,
                camera_,
                maybe_collision ? blue_color_material_props_ : red_color_material_props_
            );
        }

        const Rect viewport_screenspace_rect = ui::get_main_viewport_workspace_screenspace_rect();

        // draw crosshair overlay
        graphics::draw(
            crosshair_mesh_,
            camera_.inverse_view_projection_matrix(aspect_ratio_of(viewport_screenspace_rect)),
            material_,
            camera_,
            black_color_material_props_
        );

        // draw scene to screen
        camera_.set_pixel_rect(viewport_screenspace_rect);
        camera_.render_to_screen();
    }

    Camera camera_;
    MeshBasicMaterial material_;
    Mesh sphere_mesh_ = SphereGeometry{1.0f, 12, 12};
    Mesh wireframe_mesh_ = AABBGeometry{};
    Mesh circle_mesh_ = CircleGeometry{1.0f, 36};
    Mesh crosshair_mesh_ = generate_crosshair_mesh();
    Mesh triangle_mesh_ = generate_triangle_mesh();
    MeshBasicMaterial::PropertyBlock black_color_material_props_{Color::black()};
    MeshBasicMaterial::PropertyBlock blue_color_material_props_{Color::blue()};
    MeshBasicMaterial::PropertyBlock red_color_material_props_{Color::red()};

    // scene state
    std::vector<SceneSphere> scene_spheres_ = generate_scene_spheres();
    AABB scene_sphere_aabb_ = sphere_mesh_.bounds();
    Sphere sphere_bounding_sphere_ = bounding_sphere_of(sphere_mesh_);
    bool is_mouse_captured_ = false;
    EulerAngles camera_eulers{};
    bool showing_aabbs_ = true;
};


CStringView osc::HittestTab::id()
{
    return c_tab_string_id;
}

osc::HittestTab::HittestTab(const ParentPtr<ITabHost>&) :
    impl_{std::make_unique<Impl>()}
{}

osc::HittestTab::HittestTab(HittestTab&&) noexcept = default;
osc::HittestTab& osc::HittestTab::operator=(HittestTab&&) noexcept = default;
osc::HittestTab::~HittestTab() noexcept = default;

UID osc::HittestTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::HittestTab::impl_get_name() const
{
    return impl_->name();
}

void osc::HittestTab::impl_on_mount()
{
    impl_->on_mount();
}

void osc::HittestTab::impl_on_unmount()
{
    impl_->on_unmount();
}

bool osc::HittestTab::impl_on_event(const SDL_Event& e)
{
    return impl_->on_event(e);
}

void osc::HittestTab::impl_on_tick()
{
    impl_->on_tick();
}

void osc::HittestTab::impl_on_draw()
{
    impl_->on_draw();
}
