#include "TabTestingScreen.h"

#include <oscar/Platform/App.h>
#include <oscar/UI/Tabs/TabRegistryEntry.h>
#include <oscar/UI/Tabs/ITabHost.h>
#include <oscar/Utils/ParentPtr.h>

#include <SDL_events.h>
#include <oscar/UI/ui_context.h>

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
    explicit Impl(TabRegistryEntry registryEntry) :
        m_RegistryEntry{std::move(registryEntry)}
    {}

private:
    void impl_on_mount() override
    {
        m_CurrentTab = m_RegistryEntry.createTab(ParentPtr<Impl>{shared_from_this()});
        ui::context::init();
        m_CurrentTab->onMount();
        App::upd().make_main_loop_polling();
    }

    void impl_on_unmount() override
    {
        App::upd().make_main_loop_waiting();
        m_CurrentTab->onUnmount();
        ui::context::shutdown();
    }

    void impl_on_event(const SDL_Event& e) override
    {
        ui::context::on_event(e);
        m_CurrentTab->onEvent(e);

        if (e.type == SDL_QUIT) {
            throw std::runtime_error{"forcibly quit"};
        }
    }

    void impl_on_tick() override
    {
        m_CurrentTab->onTick();
    }

    void impl_on_draw() override
    {
        App::upd().clear_screen({0.0f, 0.0f, 0.0f, 0.0f});
        ui::context::on_start_new_frame();
        m_CurrentTab->onDraw();
        ui::context::render();

        ++m_FramesShown;
        if (m_FramesShown >= m_MinFramesShown &&
            App::get().frame_start_time() >= m_CloseTime) {

            App::upd().request_quit();
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
    AppClock::time_point m_CloseTime = App::get().frame_start_time() + m_MinOpenDuration;
};

osc::TabTestingScreen::TabTestingScreen(const TabRegistryEntry& registryEntry) :
    m_Impl{std::make_shared<Impl>(registryEntry)}
{}
void osc::TabTestingScreen::impl_on_mount() { m_Impl->on_mount(); }
void osc::TabTestingScreen::impl_on_unmount() { m_Impl->on_unmount(); }
void osc::TabTestingScreen::impl_on_event(const SDL_Event& e) { m_Impl->on_event(e); }
void osc::TabTestingScreen::impl_on_tick() { m_Impl->on_tick(); }
void osc::TabTestingScreen::impl_on_draw() { m_Impl->on_draw(); }
