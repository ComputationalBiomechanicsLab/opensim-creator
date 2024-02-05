#include "TabTestingScreen.hpp"

#include <oscar/Platform/App.hpp>
#include <oscar/UI/Tabs/TabRegistryEntry.hpp>
#include <oscar/UI/Tabs/ITabHost.hpp>
#include <oscar/Utils/ParentPtr.hpp>

#include <SDL_events.h>
#include <imgui.h>
#include <ImGuizmo.h>  // care: must come after imgui.h
#include <imgui_internal.h>
#include <implot.h>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class osc::TabTestingScreen::Impl final :
    public std::enable_shared_from_this<Impl>,
    public IScreen,
    public ITabHost {
public:
    Impl(TabRegistryEntry const& registryEntry) :
        m_RegistryEntry{registryEntry}
    {}

private:
    void implOnMount() override
    {
        m_CurrentTab = m_RegistryEntry.createTab(ParentPtr<Impl>{shared_from_this()});
        ImGuiInit();
        ImPlot::CreateContext();
        m_CurrentTab->onMount();
        App::upd().makeMainEventLoopPolling();
    }

    void implOnUnmount() override
    {
        App::upd().makeMainEventLoopWaiting();
        m_CurrentTab->onUnmount();
        ImPlot::DestroyContext();
        ImGuiShutdown();
    }

    void implOnEvent(SDL_Event const& e) override
    {
        ImGuiOnEvent(e);
        m_CurrentTab->onEvent(e);

        if (e.type == SDL_QUIT) {
            throw std::runtime_error{"forcibly quit"};
        }
    }

    void implOnTick() override
    {
        m_CurrentTab->onTick();
    }

    void implOnDraw() override
    {
        App::upd().clearScreen({0.0f, 0.0f, 0.0f, 0.0f});
        ImGuiNewFrame();
        ImGuizmo::BeginFrame();
        m_CurrentTab->onDraw();
        ImGuiRender();

        ++m_FramesShown;
        if (m_FramesShown >= m_MinFramesShown &&
            App::get().getFrameStartTime() >= m_CloseTime) {

            App::upd().requestQuit();
        }
    }

    UID implAddTab(std::unique_ptr<ITab>) override { return UID{}; }
    void implSelectTab(UID) override {}
    void implCloseTab(UID) override {}
    void implResetImgui() override {}

    TabRegistryEntry m_RegistryEntry;
    std::unique_ptr<ITab> m_CurrentTab;
    size_t m_MinFramesShown = 2;
    size_t m_FramesShown = 0;
    AppClock::duration m_MinOpenDuration = AppSeconds{0};
    AppClock::time_point m_CloseTime = App::get().getFrameStartTime() + m_MinOpenDuration;
};

osc::TabTestingScreen::TabTestingScreen(TabRegistryEntry const& registryEntry) :
    m_Impl{std::make_shared<Impl>(registryEntry)}
{}
void osc::TabTestingScreen::implOnMount() { m_Impl->onMount(); }
void osc::TabTestingScreen::implOnUnmount() { m_Impl->onUnmount(); }
void osc::TabTestingScreen::implOnEvent(SDL_Event const& e) { m_Impl->onEvent(e); }
void osc::TabTestingScreen::implOnTick() { m_Impl->onTick(); }
void osc::TabTestingScreen::implOnDraw() { m_Impl->onDraw(); }
