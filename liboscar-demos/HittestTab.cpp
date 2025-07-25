#include "HittestTab.h"

#include <liboscar/oscar.h>

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

using namespace osc;

namespace
{
    constexpr auto c_triangle_vertices = std::to_array<Vec3>({
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
        rv.set_vertices(c_triangle_vertices);
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

class osc::HittestTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/Hittest"; }

    explicit Impl(HittestTab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {
        camera_.set_background_color({1.0f, 1.0f, 1.0f, 0.0f});
    }

    void on_mount()
    {
        App::upd().make_main_loop_polling();
        camera_.on_mount();
    }

    void on_unmount()
    {
        camera_.on_unmount();
        App::upd().make_main_loop_waiting();
    }

    bool on_event(Event& e)
    {
        return camera_.on_event(e);
    }

    void on_tick()
    {
        // hit-test spheres

        const Line ray = get_camera_ray(camera_);
        float closest_distance = std::numeric_limits<float>::max();
        SceneSphere* closest_sphere = nullptr;

        for (SceneSphere& scene_sphere : scene_spheres_) {
            scene_sphere.is_hovered = false;

            const Sphere hittest_sphere{
                .origin = scene_sphere.pos,
                .radius = sphere_bounding_sphere_.radius,
            };

            const std::optional<RayCollision> collision = find_collision(ray, hittest_sphere);
            if (collision and collision->distance >= 0.0f and collision->distance < closest_distance) {
                closest_distance = collision->distance;
                closest_sphere = &scene_sphere;
            }
        }

        if (closest_sphere) {
            closest_sphere->is_hovered = true;
        }
    }

    void on_draw()
    {
        camera_.on_draw();

        // render spheres
        for (const SceneSphere& scene_sphere : scene_spheres_) {

            graphics::draw(
                sphere_mesh_,
                {.position = scene_sphere.pos},
                material_,
                camera_,
                scene_sphere.is_hovered ? blue_color_material_props_ : red_color_material_props_
            );

            // draw sphere AABBs
            if (showing_aabbs_) {

                graphics::draw(
                    wireframe_mesh_,
                    {.scale = half_widths_of(scene_sphere_aabb_), .position = scene_sphere.pos},
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

        // hit-test + draw triangle
        {
            const Line ray = get_camera_ray(camera_);
            const std::optional<RayCollision> maybe_collision = find_collision(
                ray,
                Triangle{c_triangle_vertices.at(0), c_triangle_vertices.at(1), c_triangle_vertices.at(2)}
            );

            graphics::draw(
                triangle_mesh_,
                identity<Transform>(),
                material_,
                camera_,
                maybe_collision ? blue_color_material_props_ : red_color_material_props_
            );
        }

        const Rect workspace_screen_space_rect = ui::get_main_window_workspace_screen_space_rect();

        // draw crosshair overlay
        graphics::draw(
            crosshair_mesh_,
            camera_.inverse_view_projection_matrix(aspect_ratio_of(workspace_screen_space_rect)),
            material_,
            camera_,
            black_color_material_props_
        );

        // draw scene to screen
        camera_.set_pixel_rect(workspace_screen_space_rect);
        camera_.render_to_main_window();
    }

private:
    MouseCapturingCamera camera_;
    MeshBasicMaterial material_;
    Mesh sphere_mesh_ = SphereGeometry{{.num_width_segments = 12, .num_height_segments = 12}};
    Mesh wireframe_mesh_ = AABBGeometry{};
    Mesh circle_mesh_ = CircleGeometry{{.radius = 1.0f, .num_segments = 36}};
    Mesh crosshair_mesh_ = generate_crosshair_mesh();
    Mesh triangle_mesh_ = generate_triangle_mesh();
    MeshBasicMaterial::PropertyBlock black_color_material_props_{Color::black()};
    MeshBasicMaterial::PropertyBlock blue_color_material_props_{Color::blue()};
    MeshBasicMaterial::PropertyBlock red_color_material_props_{Color::red()};

    // scene state
    std::vector<SceneSphere> scene_spheres_ = generate_scene_spheres();
    AABB scene_sphere_aabb_ = sphere_mesh_.bounds();
    Sphere sphere_bounding_sphere_ = bounding_sphere_of(sphere_mesh_);
    EulerAngles camera_eulers;
    bool showing_aabbs_ = true;
};


CStringView osc::HittestTab::id() { return Impl::static_label(); }

osc::HittestTab::HittestTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::HittestTab::impl_on_mount() { private_data().on_mount(); }
void osc::HittestTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::HittestTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::HittestTab::impl_on_tick() { private_data().on_tick(); }
void osc::HittestTab::impl_on_draw() { private_data().on_draw(); }
