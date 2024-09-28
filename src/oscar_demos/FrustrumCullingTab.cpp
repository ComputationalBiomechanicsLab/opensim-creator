#include "FrustrumCullingTab.h"

#include <oscar/oscar.h>

#include <array>
#include <cstddef>
#include <ranges>
#include <memory>
#include <random>

using namespace osc;
using namespace osc::literals;
namespace rgs = std::ranges;

namespace
{
    struct TransformedMesh {
        Mesh mesh;
        Transform transform;
    };

    std::vector<TransformedMesh> generateDecorations()
    {
        const auto geoms = std::to_array<Mesh>({
            SphereGeometry{},
            TorusKnotGeometry{},
            IcosahedronGeometry{},
            BoxGeometry{},
        });

        auto rng = std::default_random_engine{std::random_device{}()};
        auto dist = std::normal_distribution{0.1f, 0.1f};
        const AABB bounds = {{-5.0f, -2.0f, -5.0f}, {5.0f, 2.0f, 5.0f}};
        const Vec3 dims = dimensions_of(bounds);
        const Vec3uz cells = {10, 3, 10};

        std::vector<TransformedMesh> rv;
        rv.reserve(cells.x * cells.y * cells.z);

        for (size_t x = 0; x < cells.x; ++x) {
            for (size_t y = 0; y < cells.y; ++y) {
                for (size_t z = 0; z < cells.z; ++z) {

                    const Vec3 pos = bounds.min + dims * (Vec3{x, y, z} / Vec3{cells - 1_uz});

                    Mesh mesh;
                    rgs::sample(geoms, &mesh, 1, rng);

                    rv.push_back(TransformedMesh{
                        .mesh = mesh,
                        .transform = {
                            .scale = Vec3{dist(rng)},
                            .position = pos,
                        }
                    });
                }
            }
        }

        return rv;
    }
}

class osc::FrustrumCullingTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "Demos/FrustrumCulling"; }

    explicit Impl(FrustrumCullingTab& owner) :
        TabPrivate{owner, static_label()}
    {
        user_camera_.set_clipping_planes({0.1f, 10.0f});
        top_down_camera_.set_position({0.0f, 9.5f, 0.0f});
        top_down_camera_.set_direction({0.0f, -1.0f, 0.0f});
        top_down_camera_.set_clipping_planes({0.1f, 10.0f});
    }

    void on_mount()
    {
        App::upd().make_main_loop_polling();
        user_camera_.on_mount();
    }

    void on_unmount()
    {
        user_camera_.on_unmount();
        App::upd().make_main_loop_waiting();
    }

    bool on_event(Event& e)
    {
        return user_camera_.on_event(e);
    }

    void on_draw()
    {
        const Rect viewport_screenspace_rect = ui::get_main_viewport_workspace_screenspace_rect();
        const float xmid = midpoint(viewport_screenspace_rect.p1.x, viewport_screenspace_rect.p2.x);
        const Rect lhs_screenspace_rect = {viewport_screenspace_rect.p1, {xmid, viewport_screenspace_rect.p2.y}};
        const Rect rhs_screenspace_rect = {{xmid, viewport_screenspace_rect.p1.y}, viewport_screenspace_rect.p2};
        const FrustumPlanes frustum = calc_frustum_planes(user_camera_, aspect_ratio_of(lhs_screenspace_rect));

        user_camera_.on_draw();  // update from inputs etc.

        // render from user's perspective on left-hand side
        for (const auto& decoration : decorations_) {
            const AABB aabb = transform_aabb(decoration.transform, decoration.mesh.bounds());
            if (is_intersecting(frustum, aabb)) {
                graphics::draw(decoration.mesh, decoration.transform, material_, user_camera_, blue_material_props_);
            }
        }
        user_camera_.set_pixel_rect(lhs_screenspace_rect);
        user_camera_.render_to_screen();

        // render from top-down perspective on right-hand side
        for (const auto& decoration : decorations_) {
            const AABB aabb = transform_aabb(decoration.transform, decoration.mesh.bounds());
            const auto & props = is_intersecting(frustum, aabb) ? blue_material_props_ : red_material_props_;
            graphics::draw(decoration.mesh, decoration.transform, material_, top_down_camera_, props);
        }
        graphics::draw(
            SphereGeometry{},
            {.scale = Vec3{0.1f}, .position = user_camera_.position()},
            material_,
            top_down_camera_,
            green_material_props_
        );
        top_down_camera_.set_pixel_rect(rhs_screenspace_rect);
        top_down_camera_.set_scissor_rect(rhs_screenspace_rect);  // stops camera clear from clearing left-hand side
        top_down_camera_.set_background_color({0.1f, 1.0f});
        top_down_camera_.render_to_screen();
    }

    MouseCapturingCamera user_camera_;
    std::vector<TransformedMesh> decorations_ = generateDecorations();
    Camera top_down_camera_;
    MeshBasicMaterial material_;
    MeshBasicMaterial::PropertyBlock red_material_props_{Color::red()};
    MeshBasicMaterial::PropertyBlock blue_material_props_{Color::blue()};
    MeshBasicMaterial::PropertyBlock green_material_props_{Color::green()};
};


osc::CStringView osc::FrustrumCullingTab::id() { return Impl::static_label(); }

osc::FrustrumCullingTab::FrustrumCullingTab(Widget&) :
    Tab{std::make_unique<Impl>(*this)}
{}
void osc::FrustrumCullingTab::impl_on_mount() { private_data().on_mount(); }
void osc::FrustrumCullingTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::FrustrumCullingTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::FrustrumCullingTab::impl_on_draw() { private_data().on_draw(); }
