#include "ImGuizmoDemoTab.h"

#include <oscar/oscar.h>

#include <memory>

using namespace osc::literals;
using namespace osc;

class osc::ImGuizmoDemoTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "Demos/ImGuizmo"; }

    explicit Impl(ImGuizmoDemoTab& owner, Widget& parent) :
        TabPrivate{owner, &parent, static_label()}
    {}

    void on_draw()
    {
        const Mat4 view_matrix = scene_camera_.view_matrix();
        const Rect viewport_ui_rect = ui::get_main_viewport_workspace_uiscreenspace_rect();
        const Mat4 projection_matrix = scene_camera_.projection_matrix(aspect_ratio_of(dimensions_of(viewport_ui_rect)));

        ui::gizmo_demo_draw_grid(identity<Mat4>(), view_matrix, projection_matrix, 100.0f, viewport_ui_rect);
        ui::gizmo_demo_draw_cube(model_matrix_, view_matrix, projection_matrix, viewport_ui_rect);
        gizmo_.draw_to_foreground(
            model_matrix_,
            view_matrix,
            projection_matrix,
            viewport_ui_rect
        );
        ui::draw_gizmo_mode_selector(gizmo_);
        ui::draw_gizmo_op_selector(gizmo_);
    }

private:
    PolarPerspectiveCamera scene_camera_ = []()
    {
        PolarPerspectiveCamera rv;
        rv.focus_point = {0.0f, 0.0f, 0.0f};
        rv.phi = 1_rad;
        rv.theta = 0_rad;
        rv.radius = 5.0f;
        return rv;
    }();

    ui::Gizmo gizmo_;
    Mat4 model_matrix_ = identity<Mat4>();
};


CStringView osc::ImGuizmoDemoTab::id() { return Impl::static_label(); }

osc::ImGuizmoDemoTab::ImGuizmoDemoTab(Widget& parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::ImGuizmoDemoTab::impl_on_draw() { private_data().on_draw(); }
