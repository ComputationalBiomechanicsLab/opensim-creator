#include "RendererLightingMapsTab.hpp"

#include "src/Graphics/Renderer.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <utility>

class osc::RendererLightingMapsTab::Impl final {
public:
    Impl(TabHost* parent) : m_Parent{parent}
    {
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return "Lighting Maps (LearnOpenGL)";
    }

    TabHost* getParent() const
    {
        return m_Parent;
    }

    void onMount()
    {
    }

    void onUnmount()
    {
    }

    bool onEvent(SDL_Event const& e)
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
    }

private:
    UID m_ID;
    TabHost* m_Parent;
    experimental::Texture2D m_ContainerColor = experimental::LoadTexture2DFromImageResource("textures/container2.png");
    experimental::Texture2D m_ContainerSpecular = experimental::LoadTexture2DFromImageResource("textures/container2_specular.png");
};


// public API (PIMPL)

osc::RendererLightingMapsTab::RendererLightingMapsTab(TabHost* parent) :
    m_Impl{new Impl{std::move(parent)}}
{
}

osc::RendererLightingMapsTab::RendererLightingMapsTab(RendererLightingMapsTab&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::RendererLightingMapsTab& osc::RendererLightingMapsTab::operator=(RendererLightingMapsTab&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::RendererLightingMapsTab::~RendererLightingMapsTab() noexcept
{
    delete m_Impl;
}

osc::UID osc::RendererLightingMapsTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererLightingMapsTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::RendererLightingMapsTab::implParent() const
{
    return m_Impl->getParent();
}

void osc::RendererLightingMapsTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererLightingMapsTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererLightingMapsTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererLightingMapsTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererLightingMapsTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::RendererLightingMapsTab::implOnDraw()
{
    m_Impl->onDraw();
}
