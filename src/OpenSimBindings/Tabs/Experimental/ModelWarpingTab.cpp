#include "ModelWarpingTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Rect.hpp"

#include <glm/vec3.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <SDL_events.h>

#include <string>
#include <utility>

class osc::ModelWarpingTab::Impl final {
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
        // set the size+pos (central) of the main menu
        {
            Rect const mainMenuRect = calcMenuRect();
            glm::vec2 const mainMenuDims = Dimensions(mainMenuRect);
            ImGui::SetNextWindowPos(mainMenuRect.p1);
            ImGui::SetNextWindowSize({mainMenuDims.x, -1.0f});
            ImGui::SetNextWindowSizeConstraints(mainMenuDims, mainMenuDims);
        }

        if (ImGui::Begin("Input Screen", nullptr, ImGuiWindowFlags_NoTitleBar))
        {
            drawMenuContent();
        }
        ImGui::End();
    }

    void drawMenuContent()
    {
        ImGui::Text("hi");
    }

    Rect calcMenuRect()
    {
        glm::vec2 constexpr menuMaxDims = {640.0f, 512.0f};

        Rect const tabRect = osc::GetMainViewportWorkspaceScreenRect();
        glm::vec2 const menuDims = osc::Min(osc::Dimensions(tabRect), menuMaxDims);
        glm::vec2 const menuTopLeft = tabRect.p1 + 0.5f*(Dimensions(tabRect) - menuDims);

        return Rect{menuTopLeft, menuTopLeft + menuDims};
    }

private:
    UID m_ID;
    std::string m_Name = ICON_FA_BEZIER_CURVE " ModelWarping";
    TabHost* m_Parent;
};


// public API (PIMPL)

osc::ModelWarpingTab::ModelWarpingTab(TabHost* parent) :
    m_Impl{std::make_unique<Impl>(std::move(parent))}
{
}

osc::ModelWarpingTab::~ModelWarpingTab() noexcept = default;

osc::UID osc::ModelWarpingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::ModelWarpingTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::ModelWarpingTab::implParent() const
{
    return m_Impl->parent();
}

void osc::ModelWarpingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::ModelWarpingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::ModelWarpingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::ModelWarpingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::ModelWarpingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::ModelWarpingTab::implOnDraw()
{
    m_Impl->onDraw();
}
