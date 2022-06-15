#include "ImGuiDemoScreen.hpp"

#include "src/Platform/App.hpp"
#include "src/Screens/ExperimentsScreen.hpp"

#include <imgui.h>


void osc::ImGuiDemoScreen::onMount()
{
    osc::ImGuiInit();
}

void osc::ImGuiDemoScreen::onUnmount()
{
    osc::ImGuiShutdown();
}

void osc::ImGuiDemoScreen::onEvent(SDL_Event const& e)
{
    if (e.type == SDL_QUIT)
    {
        App::upd().requestQuit();
        return;
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

void osc::ImGuiDemoScreen::onDraw()
{
    osc::ImGuiNewFrame();

    osc::App::upd().clearScreen({0.0f, 0.0f, 0.0f, 0.0f});

    ImGui::ShowDemoWindow();

    osc::ImGuiRender();
}
