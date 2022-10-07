#include "ImGuizmoDemoTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/Maths/Rect.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <imgui.h>
#include <ImGuizmo.h>
#include <SDL_events.h>
#include <IconsFontAwesome5.h>

#include <string>
#include <utility>

class osc::ImGuizmoDemoTab::Impl final {
public:

    Impl(TabHost* parent) : m_Parent{std::move(parent)}
    {
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return m_Name;
    }

    TabHost* parent()
    {
        return m_Parent;
    }

    void onMount()
    {

    }

    void onUnmount()
    {

    }

    bool onEvent(SDL_Event const&)
    {
        return false;
    }

    void onTick()
    {

    }

    void onDrawMainMenu()
    {

    }

    void onDraw()
    {
        glm::mat4 view = m_SceneCamera.getViewMtx();
        Rect viewportRect = GetMainViewportWorkspaceScreenRect();
        glm::vec2 dims = Dimensions(viewportRect);
        glm::mat4 projection = m_SceneCamera.getProjMtx(AspectRatio(dims));

        ImGuizmo::BeginFrame();
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
            NULL,
            NULL, //&snap[0],   // snap
            NULL, // bound sizing?
            NULL  // bound sizing snap
        );
    }


private:
    UID m_ID;
    std::string m_Name = ICON_FA_HAT_WIZARD " ImGuizmoDemoTab";
    TabHost* m_Parent;

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


// public API

osc::ImGuizmoDemoTab::ImGuizmoDemoTab(TabHost* parent) :
    m_Impl{new Impl{std::move(parent)}}
{
}

osc::ImGuizmoDemoTab::ImGuizmoDemoTab(ImGuizmoDemoTab&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ImGuizmoDemoTab& osc::ImGuizmoDemoTab::operator=(ImGuizmoDemoTab&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::ImGuizmoDemoTab::~ImGuizmoDemoTab() noexcept
{
    delete m_Impl;
}

osc::UID osc::ImGuizmoDemoTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::ImGuizmoDemoTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::ImGuizmoDemoTab::implParent() const
{
    return m_Impl->parent();
}

void osc::ImGuizmoDemoTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::ImGuizmoDemoTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::ImGuizmoDemoTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::ImGuizmoDemoTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::ImGuizmoDemoTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::ImGuizmoDemoTab::implOnDraw()
{
    m_Impl->onDraw();
}
