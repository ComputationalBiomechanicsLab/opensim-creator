#include "ErrorScreen.hpp"

#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Screens/SplashScreen.hpp"
#include "src/Widgets/LogViewer.hpp"

#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <imgui.h>

#include <stdexcept>
#include <string>

struct osc::ErrorScreen::Impl final {
    std::string msg;
    LogViewer log;

    Impl(std::string _msg) : msg{std::move(_msg)}
    {
    }
};

// public API

osc::ErrorScreen::ErrorScreen(std::exception const& ex) :
    m_Impl{new Impl{ex.what()}}
{
}

osc::ErrorScreen::~ErrorScreen() noexcept = default;

void osc::ErrorScreen::onMount()
{
    osc::ImGuiInit();
}

void osc::ErrorScreen::onUnmount()
{
    osc::ImGuiShutdown();
}

void osc::ErrorScreen::onEvent(SDL_Event const& e)
{
    if (osc::ImGuiOnEvent(e))
    {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
    {
        App::cur().requestTransition<SplashScreen>();
    }
}

void osc::ErrorScreen::draw()
{
    App::cur().clearScreen({0.0f, 0.0f, 0.0f, 0.0f});
    osc::ImGuiNewFrame();

    constexpr float width = 800.0f;
    constexpr float padding = 10.0f;

    // error message panel
    {
        glm::vec2 pos{App::cur().dims().x/2.0f, padding};
        ImGui::SetNextWindowPos(pos, ImGuiCond_Once, {0.5f, 0.0f});
        ImGui::SetNextWindowSize({width, 0.0f});

        if (ImGui::Begin("fatal error"))
        {
            ImGui::TextWrapped("The application threw an exception with the following message:");
            ImGui::Dummy({2.0f, 10.0f});
            ImGui::SameLine();
            ImGui::TextWrapped("%s", m_Impl->msg.c_str());
            ImGui::Dummy({0.0f, 10.0f});

            if (ImGui::Button("Return to splash screen (Escape)"))
            {
                App::cur().requestTransition<SplashScreen>();
            }
        }
        ImGui::End();
    }

    // log message panel
    {
        glm::vec2 dims = App::cur().dims();
        glm::vec2 pos{dims.x/2.0f, dims.y - padding};
        ImGui::SetNextWindowPos(pos, ImGuiCond_Once, ImVec2(0.5f, 1.0f));
        ImGui::SetNextWindowSize(ImVec2(width, 0.0f));

        if (ImGui::Begin("Error Log", nullptr, ImGuiWindowFlags_MenuBar))
        {
            m_Impl->log.draw();
        }
        ImGui::End();
    }

    osc::ImGuiRender();
}
