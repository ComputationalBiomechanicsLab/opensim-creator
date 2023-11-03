#include "MandelbrotTab.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/Tabs/StandardTabBase.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <SDL_events.h>

#include <limits>
#include <string>
#include <utility>

using osc::Mat4;

namespace
{
    constexpr osc::CStringView c_TabStringID = "Demos/Mandelbrot";

    osc::Camera CreateIdentityCamera()
    {
        osc::Camera camera;
        camera.setViewMatrixOverride(Mat4{1.0f});
        camera.setProjectionMatrixOverride(Mat4{1.0f});
        return camera;
    }
}

class osc::MandelbrotTab::Impl final : public osc::StandardTabBase {
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

    bool implOnEvent(SDL_Event const& e) final
    {
        if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_PAGEUP && m_NumIterations < std::numeric_limits<decltype(m_NumIterations)>::max())
        {
            m_NumIterations *= 2;
            return true;
        }
        if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_PAGEDOWN && m_NumIterations > 1)
        {
            m_NumIterations /= 2;
            return true;
        }
        if (e.type == SDL_MOUSEWHEEL)
        {
            float const factor = e.wheel.y > 0 ? 0.9f : 1.11111111f;

            applyZoom(ImGui::GetIO().MousePos, factor);
            return true;
        }
        if (e.type == SDL_MOUSEMOTION && (e.motion.state & SDL_BUTTON_LMASK) != 0)
        {
            Vec2 const screenSpacePanAmount = {static_cast<float>(e.motion.xrel), static_cast<float>(e.motion.yrel)};
            applyPan(screenSpacePanAmount);
            return true;
        }
        return false;
    }

    void implOnTick() final
    {
    }

    void implOnDraw() final
    {
        m_MainViewportWorkspaceScreenRect = GetMainViewportWorkspaceScreenRect();

        m_Material.setVec2("uRescale", {1.0f, 1.0f});
        m_Material.setVec2("uOffset", {});
        m_Material.setInt("uNumIterations", m_NumIterations);
        Graphics::DrawMesh(m_QuadMesh, Transform{}, m_Material, m_Camera);
        m_Camera.setPixelRect(m_MainViewportWorkspaceScreenRect);
        m_Camera.renderToScreen();
    }

    void applyZoom(Vec2, float)
    {
        // TODO: zoom the mandelbrot viewport into the given screen-space location by the given factor
    }

    void applyPan(Vec2)
    {
        // TODO: pan the mandelbrot viewport by the given screen-space offset vector
    }

    int m_NumIterations = 16;
    Rect m_NormalizedMandelbrotViewport = {{}, {1.0f, 1.0f}};
    Rect m_MainViewportWorkspaceScreenRect = {};
    Mesh m_QuadMesh = GenTexturedQuad();
    Material m_Material
    {
        Shader
        {
            App::slurp("oscar_demos/shaders/Mandelbrot.vert"),
            App::slurp("oscar_demos/shaders/Mandelbrot.frag"),
        },
    };
    Camera m_Camera = CreateIdentityCamera();
};


// public API

osc::CStringView osc::MandelbrotTab::id() noexcept
{
    return c_TabStringID;
}

osc::MandelbrotTab::MandelbrotTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::MandelbrotTab::MandelbrotTab(MandelbrotTab&&) noexcept = default;
osc::MandelbrotTab& osc::MandelbrotTab::operator=(MandelbrotTab&&) noexcept = default;
osc::MandelbrotTab::~MandelbrotTab() noexcept = default;

osc::UID osc::MandelbrotTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::MandelbrotTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::MandelbrotTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::MandelbrotTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::MandelbrotTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::MandelbrotTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::MandelbrotTab::implOnDraw()
{
    m_Impl->onDraw();
}
