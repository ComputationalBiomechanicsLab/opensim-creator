#include "UiModelViewerScreen.hpp"

#include "src/OpenSimBindings/AutoFinalizingModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Widgets/UiModelViewer.hpp"

#include <imgui.h>

#include <string>

class osc::UiModelViewerScreen::Impl final {
public:
    void onMount()
    {
        App& app = App::upd();
        app.enableDebugMode();
        app.disableVsync();
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
            return;
        }
        else if (osc::ImGuiOnEvent(e))
        {
            return;  // ImGui handled this particular event
        }
    }

    void tick(float)
    {
    }

    void draw()
    {
        osc::ImGuiNewFrame();
        App::upd().clearScreen({0.0f, 0.0f, 0.0f, 0.0f});

        ImGui::Begin("cookiecutter panel");
        ImGui::Text("%.2f", ImGui::GetIO().Framerate);
        ImGui::End();

        ImGui::Begin("viewer", nullptr, ImGuiWindowFlags_MenuBar);
        auto resp = m_ModelViewer.draw(m_UiModel);
        if (resp.hovertestResult)
        {
            ImGui::BeginTooltip();
            ImGui::Text("hello");
            ImGui::EndTooltip();
        }
        m_UiModel.setHovered(resp.hovertestResult);
        ImGui::End();

        osc::ImGuiRender();
    }

private:
    std::string m_ModelPath = App::resource("models/RajagopalModel/Rajagopal2015.osim").string();
    AutoFinalizingModelStatePair m_UiModel{m_ModelPath};
    UiModelViewer m_ModelViewer;
};

// public API

osc::UiModelViewerScreen::UiModelViewerScreen() :
    m_Impl{new Impl{}}
{
}

osc::UiModelViewerScreen::UiModelViewerScreen(UiModelViewerScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::UiModelViewerScreen& osc::UiModelViewerScreen::operator=(UiModelViewerScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::UiModelViewerScreen::~UiModelViewerScreen() noexcept
{
    delete m_Impl;
}

void osc::UiModelViewerScreen::onMount()
{
    m_Impl->onMount();
}

void osc::UiModelViewerScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::UiModelViewerScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::UiModelViewerScreen::tick(float dt)
{
    m_Impl->tick(std::move(dt));
}

void osc::UiModelViewerScreen::draw()
{
    m_Impl->draw();
}
