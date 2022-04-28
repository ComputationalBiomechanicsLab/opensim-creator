#include "ImGuizmoDemoScreen.hpp"

#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/Platform/App.hpp"
#include "src/Screens/ExperimentsScreen.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <imgui.h>
#include <ImGuizmo.h>

#include <utility>

class osc::ImGuizmoDemoScreen::Impl final {
public:
    void onMount()
    {
        osc::ImGuiInit();
    }

    void onUnmount()
    {
        osc::ImGuiShutdown();
    }

    void onEvent(SDL_Event const& e)
    {
        if (e.type == SDL_QUIT)
        {
            App::upd().requestQuit();
        }
        else if (osc::ImGuiOnEvent(e))
        {
            return;
        }
        else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        {
            App::upd().requestTransition<ExperimentsScreen>();
            return;
        }
    }

    void draw()
    {
        osc::ImGuiNewFrame();

        osc::App::upd().clearScreen({0.0f, 0.0f, 0.0f, 0.0f});

        glm::mat4 view = m_SceneCamera.getViewMtx();
        glm::vec2 dims = App::get().dims();
        glm::mat4 projection = m_SceneCamera.getProjMtx(dims.x/dims.y);

        ImGuizmo::BeginFrame();
        ImGuizmo::SetRect(0, 0, dims.x, dims.y);
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
            NULL,   // bound sizing?
            NULL);  // bound sizing snap

        osc::ImGuiRender();
    }

private:
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

osc::ImGuizmoDemoScreen::ImGuizmoDemoScreen() :
    m_Impl{new Impl{}}
{
}

osc::ImGuizmoDemoScreen::ImGuizmoDemoScreen(ImGuizmoDemoScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ImGuizmoDemoScreen& osc::ImGuizmoDemoScreen::operator=(ImGuizmoDemoScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::ImGuizmoDemoScreen::~ImGuizmoDemoScreen() noexcept
{
    delete m_Impl;
}

void osc::ImGuizmoDemoScreen::onMount()
{
    m_Impl->onMount();
}

void osc::ImGuizmoDemoScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::ImGuizmoDemoScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::ImGuizmoDemoScreen::draw()
{
    m_Impl->draw();
}
