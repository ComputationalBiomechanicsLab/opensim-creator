#include "LOGLBloomTab.hpp"

#include "oscar/Graphics/Camera.hpp"
#include "oscar/Maths/Transform.hpp"
#include "oscar/Utils/Algorithms.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <string>
#include <utility>

namespace
{
    constexpr osc::CStringView c_TabStringID = "LearnOpenGL/Bloom";

    struct SceneLight final {
        glm::vec3 position;
        osc::Color color;
    };

    constexpr auto c_SceneLights = osc::MakeArray<SceneLight>(
        SceneLight{{ 0.0f, 0.5f,  1.5f}, { 5.0f, 5.0f,  5.0f}},
        SceneLight{{-4.0f, 0.5f, -3.0f}, {10.0f, 0.0f,  0.0f}},
        SceneLight{{ 3.0f, 0.5f,  1.0f}, { 0.0f, 0.0f, 15.0f}},
        SceneLight{{-0.8f, 2.4f, -1.0f}, { 0.0f, 5.0f,  0.0f}}
    );
}

class osc::LOGLBloomTab::Impl final {
public:

    Impl(std::weak_ptr<TabHost> parent_) :
        m_Parent{std::move(parent_)}
    {
        m_Camera.setPosition({0.0f, 0.0f, 5.0f});
        m_Camera.setNearClippingPlane(0.1f);
        m_Camera.setFarClippingPlane(100.0f);
        m_Camera.setBackgroundColor({0.0f, 0.0f, 0.0f, 1.0f});
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return c_TabStringID;
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
    }

private:
    UID m_TabID;
    std::weak_ptr<TabHost> m_Parent;



    Camera m_Camera;
    bool m_UseBloom = true;
    float m_Exposure = 1.0f;
};


// public API (PIMPL)

osc::CStringView osc::LOGLBloomTab::id() noexcept
{
    return c_TabStringID;
}

osc::LOGLBloomTab::LOGLBloomTab(std::weak_ptr<TabHost> parent_) :
    m_Impl{std::make_unique<Impl>(std::move(parent_))}
{
}

osc::LOGLBloomTab::LOGLBloomTab(LOGLBloomTab&&) noexcept = default;
osc::LOGLBloomTab& osc::LOGLBloomTab::operator=(LOGLBloomTab&&) noexcept = default;
osc::LOGLBloomTab::~LOGLBloomTab() noexcept = default;

osc::UID osc::LOGLBloomTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::LOGLBloomTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::LOGLBloomTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::LOGLBloomTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::LOGLBloomTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::LOGLBloomTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::LOGLBloomTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::LOGLBloomTab::implOnDraw()
{
    m_Impl->onDraw();
}
