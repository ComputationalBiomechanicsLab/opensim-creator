#include "ImGuizmoDemoTab.h"

#include <oscar/oscar.h>

#include <memory>

using namespace osc::literals;
using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "Demos/ImGuizmo";
}

class osc::ImGuizmoDemoTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {}

private:
    void impl_on_draw() final
    {
        // ImGuizmo::BeginFrame();  already done by MainUIScreen

        Mat4 view = m_SceneCamera.view_matrix();
        Rect viewportRect = ui::GetMainViewportWorkspaceScreenRect();
        Vec2 dims = dimensions_of(viewportRect);
        Mat4 projection = m_SceneCamera.projection_matrix(aspect_ratio_of(dims));

        ImGuizmo::SetRect(viewportRect.p1.x, viewportRect.p1.y, dims.x, dims.y);
        Mat4 identityMatrix = identity<Mat4>();
        ImGuizmo::DrawGrid(value_ptr(view), value_ptr(projection), value_ptr(identityMatrix), 100.f);
        ImGuizmo::DrawCubes(value_ptr(view), value_ptr(projection), value_ptr(m_ModelMatrix), 1);

        ui::Checkbox("translate", &m_IsInTranslateMode);

        ImGuizmo::Manipulate(
            value_ptr(view),
            value_ptr(projection),
            m_IsInTranslateMode ? ImGuizmo::TRANSLATE : ImGuizmo::ROTATE,
            ImGuizmo::LOCAL,
            value_ptr(m_ModelMatrix),
            nullptr,
            nullptr, //&snap[0],   // snap
            nullptr, // bound sizing?
            nullptr  // bound sizing snap
        );
    }

    PolarPerspectiveCamera m_SceneCamera = []()
    {
        PolarPerspectiveCamera rv;
        rv.focus_point = {0.0f, 0.0f, 0.0f};
        rv.phi = 1_rad;
        rv.theta = 0_rad;
        rv.radius = 5.0f;
        return rv;
    }();

    bool m_IsInTranslateMode = false;
    Mat4 m_ModelMatrix = identity<Mat4>();
};


// public API

CStringView osc::ImGuizmoDemoTab::id()
{
    return "Demos/ImGuizmo";
}

osc::ImGuizmoDemoTab::ImGuizmoDemoTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{}

osc::ImGuizmoDemoTab::ImGuizmoDemoTab(ImGuizmoDemoTab&&) noexcept = default;
osc::ImGuizmoDemoTab& osc::ImGuizmoDemoTab::operator=(ImGuizmoDemoTab&&) noexcept = default;
osc::ImGuizmoDemoTab::~ImGuizmoDemoTab() noexcept = default;

UID osc::ImGuizmoDemoTab::impl_get_id() const
{
    return m_Impl->id();
}

CStringView osc::ImGuizmoDemoTab::impl_get_name() const
{
    return m_Impl->name();
}

void osc::ImGuizmoDemoTab::impl_on_draw()
{
    m_Impl->on_draw();
}
