#include "ImGuizmoDemoTab.hpp"

#include "oscar/Bindings/ImGuiHelpers.hpp"
#include "oscar/Maths/MathHelpers.hpp"
#include "oscar/Maths/PolarPerspectiveCamera.hpp"
#include "oscar/Maths/Rect.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>
#include <ImGuizmo.h>

#include <memory>

namespace
{
    constexpr osc::CStringView c_TabStringID = "Demos/ImGuizmo";
}

class osc::ImGuizmoDemoTab::Impl final {
public:

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return c_TabStringID;
    }

    void onDraw()
    {
        // ImGuizmo::BeginFrame();  already done by MainUIScreen

        glm::mat4 view = m_SceneCamera.getViewMtx();
        Rect viewportRect = GetMainViewportWorkspaceScreenRect();
        glm::vec2 dims = Dimensions(viewportRect);
        glm::mat4 projection = m_SceneCamera.getProjMtx(AspectRatio(dims));

        ImGuizmo::SetRect(viewportRect.p1.x, viewportRect.p1.y, dims.x, dims.y);
        glm::mat4 identity{1.0f};
        ImGuizmo::DrawGrid(glm::value_ptr(view), glm::value_ptr(projection), glm::value_ptr(identity), 100.f);
        ImGuizmo::DrawCubes(glm::value_ptr(view), glm::value_ptr(projection), glm::value_ptr(m_ModelMatrix), 1);

        ImGui::Checkbox("translate", &m_IsInTranslateMode);

        ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(projection),
            m_IsInTranslateMode ? ImGuizmo::TRANSLATE : ImGuizmo::ROTATE,
            ImGuizmo::LOCAL,
            glm::value_ptr(m_ModelMatrix),
            nullptr,
            nullptr, //&snap[0],   // snap
            nullptr, // bound sizing?
            nullptr  // bound sizing snap
        );
    }

private:
    UID m_TabID;

    PolarPerspectiveCamera m_SceneCamera = []()
    {
        PolarPerspectiveCamera rv;
        rv.focusPoint = {0.0f, 0.0f, 0.0f};
        rv.phi = 1.0f;
        rv.theta = 0.0f;
        rv.radius = 5.0f;
        return rv;
    }();

    bool m_IsInTranslateMode = false;
    glm::mat4 m_ModelMatrix{1.0f};
};


// public API (PIMPL)

osc::CStringView osc::ImGuizmoDemoTab::id() noexcept
{
    return "Demos/ImGuizmo";
}

osc::ImGuizmoDemoTab::ImGuizmoDemoTab(std::weak_ptr<TabHost>) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::ImGuizmoDemoTab::ImGuizmoDemoTab(ImGuizmoDemoTab&&) noexcept = default;
osc::ImGuizmoDemoTab& osc::ImGuizmoDemoTab::operator=(ImGuizmoDemoTab&&) noexcept = default;
osc::ImGuizmoDemoTab::~ImGuizmoDemoTab() noexcept = default;

osc::UID osc::ImGuizmoDemoTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::ImGuizmoDemoTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::ImGuizmoDemoTab::implOnDraw()
{
    m_Impl->onDraw();
}
