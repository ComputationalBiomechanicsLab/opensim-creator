#include "RendererBlendingTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Color.hpp"
#include "src/Graphics/Renderer.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <SDL_events.h>

#include <cstdint>
#include <utility>

class osc::RendererBlendingTab::Impl final {
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
        return "Blending (LearnOpenGL)";
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
};


// public API (PIMPL)

osc::RendererBlendingTab::RendererBlendingTab(TabHost* parent) :
    m_Impl{new Impl{std::move(parent)}}
{
}

osc::RendererBlendingTab::RendererBlendingTab(RendererBlendingTab&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::RendererBlendingTab& osc::RendererBlendingTab::operator=(RendererBlendingTab&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::RendererBlendingTab::~RendererBlendingTab() noexcept
{
    delete m_Impl;
}

osc::UID osc::RendererBlendingTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererBlendingTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::RendererBlendingTab::implParent() const
{
    return m_Impl->getParent();
}

void osc::RendererBlendingTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::RendererBlendingTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::RendererBlendingTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::RendererBlendingTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::RendererBlendingTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::RendererBlendingTab::implOnDraw()
{
    m_Impl->onDraw();
}
