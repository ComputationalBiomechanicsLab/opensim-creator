#include "ImGuiDemoScreen.hpp"

#include "src/App.hpp"
#include "src/3D/Gl.hpp"
#include "src/Screens/Experimental/ExperimentsScreen.hpp"

#include <imgui.h>

using namespace osc;

void osc::ImGuiDemoScreen::onMount()
{
    osc::ImGuiInit();
}

void osc::ImGuiDemoScreen::onUnmount()
{
    osc::ImGuiShutdown();
}

void ImGuiDemoScreen::onEvent(SDL_Event const& e)
{
    if (osc::ImGuiOnEvent(e))
    {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
    {
        App::cur().requestTransition<ExperimentsScreen>();
        return;
    }
}

void ImGuiDemoScreen::draw()
{
    osc::ImGuiNewFrame();

    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui::ShowDemoWindow();

    osc::ImGuiRender();
}
