#include "ImGuizmoDemoTab.h"

#include <oscar/oscar.h>

#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_tab_string_id = "Demos/ImGuizmo";
}

class osc::ImGuizmoDemoTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_tab_string_id}
    {}

private:
    void impl_on_draw() final
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


CStringView osc::ImGuizmoDemoTab::id()
{
    return "Demos/ImGuizmo";
}

osc::ImGuizmoDemoTab::ImGuizmoDemoTab(const ParentPtr<ITabHost>&) :
    impl_{std::make_unique<Impl>()}
{}

osc::ImGuizmoDemoTab::ImGuizmoDemoTab(ImGuizmoDemoTab&&) noexcept = default;
osc::ImGuizmoDemoTab& osc::ImGuizmoDemoTab::operator=(ImGuizmoDemoTab&&) noexcept = default;
osc::ImGuizmoDemoTab::~ImGuizmoDemoTab() noexcept = default;

UID osc::ImGuizmoDemoTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::ImGuizmoDemoTab::impl_get_name() const
{
    return impl_->name();
}

void osc::ImGuizmoDemoTab::impl_on_draw()
{
    impl_->on_draw();
}
