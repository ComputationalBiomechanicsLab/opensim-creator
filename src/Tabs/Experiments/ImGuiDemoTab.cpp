#include "ImGuiDemoTab.hpp"

#include <SDL_events.h>
#include <IconsFontAwesome5.h>
#include <imgui.h>

#include <string>
#include <utility>

class osc::ImGuiDemoTab::Impl final {
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
        ImGui::ShowDemoWindow();
    }


private:
    UID m_ID;
    std::string m_Name = ICON_FA_HAT_WIZARD " ImGuiDemoTab";
    TabHost* m_Parent;
};


// public API (PIMPL)

osc::CStringView osc::ImGuiDemoTab::id() noexcept
{
    return "Demos/ImGui";
}

osc::ImGuiDemoTab::ImGuiDemoTab(TabHost* parent) :
    m_Impl{std::make_unique<Impl>(std::move(parent))}
{
}

osc::ImGuiDemoTab::ImGuiDemoTab(ImGuiDemoTab&&) noexcept = default;
osc::ImGuiDemoTab& osc::ImGuiDemoTab::operator=(ImGuiDemoTab&&) noexcept = default;
osc::ImGuiDemoTab::~ImGuiDemoTab() noexcept = default;

osc::UID osc::ImGuiDemoTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::ImGuiDemoTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::ImGuiDemoTab::implParent() const
{
    return m_Impl->parent();
}

void osc::ImGuiDemoTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::ImGuiDemoTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::ImGuiDemoTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::ImGuiDemoTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::ImGuiDemoTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::ImGuiDemoTab::implOnDraw()
{
    m_Impl->onDraw();
}
