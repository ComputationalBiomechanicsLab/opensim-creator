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
#include <utility>

class osc::ErrorScreen::Impl final {
public:
    Impl(std::exception const& ex) : m_ErrorMessage{ex.what()}
    {
    }

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
            App::cur().requestQuit();
            return;
        }
        else if (osc::ImGuiOnEvent(e))
        {
            return;
        }
        else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        {
            App::cur().requestTransition<SplashScreen>();
            return;
        }
    }

    void draw()
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
                ImGui::TextWrapped("%s", m_ErrorMessage.c_str());
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
                m_LogViewer.draw();
            }
            ImGui::End();
        }

        osc::ImGuiRender();
    }

private:
    std::string m_ErrorMessage;
    LogViewer m_LogViewer;
};


// public API (PIMPL)

osc::ErrorScreen::ErrorScreen(std::exception const& ex) :
    m_Impl{new Impl{ex}}
{
}

osc::ErrorScreen::ErrorScreen(ErrorScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ErrorScreen& osc::ErrorScreen::operator=(ErrorScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::ErrorScreen::~ErrorScreen() noexcept
{
    delete m_Impl;
}

void osc::ErrorScreen::onMount()
{
    m_Impl->onMount();
}

void osc::ErrorScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::ErrorScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::ErrorScreen::draw()
{
    m_Impl->draw();
}
