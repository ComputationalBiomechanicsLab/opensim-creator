#include "LOGLPBRLightingTab.hpp"

#include "oscar/Tabs/StandardTabBase.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <SDL_events.h>

#include <string>
#include <utility>

namespace
{
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/PBR/Lighting";
}

class osc::LOGLPBRLightingTab::Impl final : public osc::StandardTabBase {
public:
    Impl() : StandardTabBase{c_TabStringID}
    {
    }

private:
    void implOnMount() final
    {
    }

    void implOnUnmount() final
    {
    }

    bool implOnEvent(SDL_Event const&) final
    {
        return false;
    }

    void implOnTick() final
    {
    }

    void implOnDrawMainMenu() final
    {
    }

    void implOnDraw() final
    {
        draw2DUI();
    }

    void draw2DUI()
    {
        ImGui::Begin("note");
        ImGui::Text("Work in progress");
        ImGui::End();
    }
};


// public API

osc::CStringView osc::LOGLPBRLightingTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLPBRLightingTab::LOGLPBRLightingTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::LOGLPBRLightingTab::LOGLPBRLightingTab(LOGLPBRLightingTab&&) noexcept = default;
osc::LOGLPBRLightingTab& osc::LOGLPBRLightingTab::operator=(LOGLPBRLightingTab&&) noexcept = default;
osc::LOGLPBRLightingTab::~LOGLPBRLightingTab() noexcept = default;

osc::UID osc::LOGLPBRLightingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LOGLPBRLightingTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLPBRLightingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLPBRLightingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLPBRLightingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLPBRLightingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::LOGLPBRLightingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::LOGLPBRLightingTab::implOnDraw()
{
    m_Impl->onDraw();
}
