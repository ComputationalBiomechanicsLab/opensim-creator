#include "ImGuizmoDemoTab.h"

#include <oscar/oscar.h>

#include <imgui.h>
#include <ImGuizmo.h>

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
        // ImGuizmo::BeginFrame();  already done by MainUIScreen

        Mat4 view_matrix = scene_camera_.view_matrix();
        Rect viewport_ui_rect = ui::get_main_viewport_workspace_uiscreenspace_rect();
        Vec2 viewport_dimensions = dimensions_of(viewport_ui_rect);
        Mat4 projection_matrix = scene_camera_.projection_matrix(aspect_ratio_of(viewport_dimensions));

        ImGuizmo::SetRect(viewport_ui_rect.p1.x, viewport_ui_rect.p1.y, viewport_dimensions.x, viewport_dimensions.y);
        Mat4 identity_matrix = identity<Mat4>();
        ImGuizmo::DrawGrid(value_ptr(view_matrix), value_ptr(projection_matrix), value_ptr(identity_matrix), 100.f);
        ImGuizmo::DrawCubes(value_ptr(view_matrix), value_ptr(projection_matrix), value_ptr(model_matrix_), 1);

        ui::draw_checkbox("translate", &translate_mode_enabled_);

        ImGuizmo::Manipulate(
            value_ptr(view_matrix),
            value_ptr(projection_matrix),
            translate_mode_enabled_ ? ImGuizmo::TRANSLATE : ImGuizmo::ROTATE,
            ImGuizmo::LOCAL,
            value_ptr(model_matrix_),
            nullptr,
            nullptr, //&snap[0],   // snap
            nullptr, // bound sizing?
            nullptr  // bound sizing snap
        );
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

    bool translate_mode_enabled_ = false;
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
