#include "ImPlotDemoScreen.hpp"

#include "src/Platform/App.hpp"

#include <imgui.h>
#include <implot.h>

#include <utility>

class osc::ImPlotDemoScreen::Impl final {
public:
    void onMount()
    {
        osc::ImGuiInit();
        ImPlot::CreateContext();
    }

    void onUnmount()
    {
        osc::ImGuiShutdown();
        ImPlot::DestroyContext();
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

    void onTick()
    {
    }

    void onDraw()
    {
        osc::ImGuiNewFrame();
        App::upd().clearScreen({0.0f, 0.0f, 0.0f, 0.0f});

        ImPlot::ShowDemoWindow();

        osc::ImGuiRender();
    }

private:
    bool m_CheckboxState = false;
};


// public API (PIMPL)

osc::ImPlotDemoScreen::ImPlotDemoScreen() :
    m_Impl{new Impl{}}
{
}

osc::ImPlotDemoScreen::ImPlotDemoScreen(ImPlotDemoScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ImPlotDemoScreen& osc::ImPlotDemoScreen::operator=(ImPlotDemoScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::ImPlotDemoScreen::~ImPlotDemoScreen() noexcept
{
    delete m_Impl;
}

void osc::ImPlotDemoScreen::onMount()
{
    m_Impl->onMount();
}

void osc::ImPlotDemoScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::ImPlotDemoScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::ImPlotDemoScreen::onTick()
{
    m_Impl->onTick();
}

void osc::ImPlotDemoScreen::onDraw()
{
    m_Impl->onDraw();
}
