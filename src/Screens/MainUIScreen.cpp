#include "MainUIScreen.hpp"

#include "src/OpenSimBindings/ForwardDynamicSimulatorParams.hpp"
#include "src/OpenSimBindings/OutputExtractor.hpp"
#include "src/OpenSimBindings/ParamBlock.hpp"
#include "src/Platform/App.hpp"
#include "src/Tabs/CookiecutterTab.hpp"
#include "src/Tabs/ErrorTab.hpp"
#include "src/Tabs/LoadingTab.hpp"
#include "src/Tabs/MeshImporterTab.hpp"
#include "src/Tabs/SplashTab.hpp"
#include "src/Tabs/Tab.hpp"
#include "src/MainUIStateAPI.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <utility>
#include <unordered_set>


class osc::MainUIScreen::Impl final : public osc::MainUIStateAPI {
public:
    Impl()
    {
        m_Tabs.push_back(std::make_unique<SplashTab>(this));
        m_Tabs.push_back(std::make_unique<ErrorTab>(this, std::runtime_error{"hi"}));
        m_Tabs.push_back(std::make_unique<CookiecutterTab>(this));
        m_Tabs.push_back(std::make_unique<MeshImporterTab>(this));
        m_Tabs.push_back(std::make_unique<MeshImporterTab>(this));
        m_Tabs.push_back(std::make_unique<LoadingTab>(this, osc::App::get().resource("models/Arm26/arm26.osim")));
        m_RequestedTab = m_Tabs.back()->getID();
    }

    Impl(std::filesystem::path p)
    {
        m_Tabs.push_back(std::make_unique<LoadingTab>(this, p));
    }

    void onMount()
    {
        osc::ImGuiInit();
        if (Tab* active = getActiveTab())
        {
            active->onMount();
        }
    }

    void onUnmount()
    {
        if (Tab* active = getActiveTab())
        {
            active->onUnmount();
            m_ActiveTab = UID::empty();
        }
        osc::ImGuiShutdown();
    }

    void onEvent(SDL_Event const& e)
    {
        if (osc::ImGuiOnEvent(e))
        {
            // event was pumped into ImGui

            m_ShouldRequestRedraw = true;
            return;
        }

        if (e.type == SDL_QUIT)
        {
            // it's a quit event, so try pumping it into all tabs

            bool quitHandled = false;
            for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i)
            {
                quitHandled = m_Tabs[i]->onEvent(e) || quitHandled;
                garbageCollectDeletedTabs();
            }

            if (!quitHandled)
            {
                App::upd().requestQuit();
            }

            return;
        }

        // all other events are only pumped into the active tab
        //
        // (don't pump the event into the inactive tabs)
        if (Tab* active = getActiveTab())
        {
            bool handled = active->onEvent(e);

            // a `tab` may have requested deletions
            garbageCollectDeletedTabs();

            if (handled)
            {
                m_ShouldRequestRedraw = true;
                return;
            }
        }
    }

    void tick(float dt)
    {
        // tick all the tabs, because they may internally be polling something (e.g.
        // updating something as a simulation runs)
        for (int i = 0; i < m_Tabs.size(); ++i)
        {
            m_Tabs[i]->onTick();
        }

        // clear the flagged-to-be-deleted tabs
        garbageCollectDeletedTabs();
    }

    void draw()
    {
        App::upd().clearScreen({0.0f, 0.0f, 0.0f, 0.0f});

        osc::ImGuiNewFrame();

        drawTabUI();

        if (m_ImguiWasAggressivelyReset)
        {
            if (m_RequestedTab == UID::empty())
            {
                m_RequestedTab = m_ActiveTab;
            }
            m_ActiveTab = UID::empty();

            osc::ImGuiShutdown();
            osc::ImGuiInit();
            osc::App::upd().requestRedraw();
            m_ImguiWasAggressivelyReset = false;

            return;
        }

        osc::ImGuiRender();

        if (m_ShouldRequestRedraw)
        {
            osc::App::upd().requestRedraw();
            m_ShouldRequestRedraw = false;
        }
    }

    ParamBlock const& getSimulationParams() const override
    {
        return m_SimulationParams;
    }

    ParamBlock& updSimulationParams() override
    {
        return m_SimulationParams;
    }

    int getNumUserOutputExtractors() const override
    {
        return static_cast<int>(m_UserOutputExtractors.size());
    }

    OutputExtractor const& getUserOutputExtractor(int idx) const override
    {
        return m_UserOutputExtractors.at(idx);
    }

    void addUserOutputExtractor(OutputExtractor const& output) override
    {
        m_UserOutputExtractors.push_back(output);
    }

    void removeUserOutputExtractor(int idx) override
    {
        OSC_ASSERT(0 <= idx && idx < static_cast<int>(m_UserOutputExtractors.size()));
        m_UserOutputExtractors.erase(m_UserOutputExtractors.begin() + idx);
    }

private:
    void drawTabUI()
    {
        constexpr ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;

        // https://github.com/ocornut/imgui/issues/3518

        ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
        float height = ImGui::GetFrameHeight();

        if (ImGui::BeginViewportSideBar("##TabSpecificMenuBar", viewport, ImGuiDir_Up, height, window_flags))
        {
            if (ImGui::BeginMenuBar())
            {
                if (Tab* active = getActiveTab())
                {
                    active->onDrawMainMenu();

                    if (m_ImguiWasAggressivelyReset)
                    {
                        return;  // must return here to prevent the ImGui End calls from erroring
                    }
                }
                ImGui::EndMenuBar();
            }
            ImGui::End();
        }

        if (ImGui::BeginViewportSideBar("##TabBarViewport", viewport, ImGuiDir_Up, height, window_flags))
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginTabBar("##TabBar", tab_bar_flags))
                {
                    for (int i = 0; i < m_Tabs.size(); ++i)
                    {
                        ImGuiTabItemFlags flags = ImGuiTabItemFlags_NoReorder;

                        if (m_Tabs[i]->isUnsaved())
                        {
                            flags |= ImGuiTabItemFlags_UnsavedDocument;
                        }

                        if (m_Tabs[i]->getID() == m_RequestedTab)
                        {
                            flags |= ImGuiTabItemFlags_SetSelected;
                        }

                        if (m_Tabs[i]->getID() == m_ActiveTab && m_Tabs[i]->getName() != m_ActiveTabNameLastFrame)
                        {
                            flags |= ImGuiTabItemFlags_SetSelected;
                            m_ActiveTabNameLastFrame = m_Tabs[i]->getName();
                        }

                        ImGui::PushID(m_Tabs[i].get());
                        bool active = true;

                        if (ImGui::BeginTabItem(m_Tabs[i]->getName().c_str(), &active, flags))
                        {
                            if (m_Tabs[i]->getID() != m_ActiveTab)
                            {
                                if (Tab* activeTab = getActiveTab())
                                {
                                    activeTab->onUnmount();
                                }
                                m_Tabs[i]->onMount();
                            }

                            m_ActiveTab = m_Tabs[i]->getID();
                            m_ActiveTabNameLastFrame = m_Tabs[i]->getName();

                            if (m_RequestedTab == m_ActiveTab)
                            {
                                m_RequestedTab = UID::empty();
                            }

                            if (m_ImguiWasAggressivelyReset)
                            {
                                return;
                            }

                            ImGui::EndTabItem();
                        }
                        ImGui::PopID();
                        if (!active && i != 0)  // can't close the splash tab
                        {
                            m_DeletedTabs.insert(m_Tabs[i]->getID());
                        }
                    }
                }
                ImGui::EndMainMenuBar();
            }
            ImGui::End();
        }

        garbageCollectDeletedTabs();

        if (Tab* active = getActiveTab())
        {
            active->onDraw();
        }

        // clear the flagged-to-be-deleted tabs
        garbageCollectDeletedTabs();
    }

    Tab* getTabByID(UID id)
    {
        auto it = std::find_if(m_Tabs.begin(), m_Tabs.end(), [id](auto const& p)
        {
            return p->getID() == id;
        });

        return it != m_Tabs.end() ? it->get() : nullptr;
    }

    Tab* getActiveTab()
    {
        return getTabByID(m_ActiveTab);
    }

    Tab* getRequestedTab()
    {
        return getTabByID(m_RequestedTab);
    }

    UID implAddTab(std::unique_ptr<Tab> tab) override
    {
        return m_Tabs.emplace_back(std::move(tab))->getID();
    }

    void implSelectTab(UID id) override
    {
        m_RequestedTab = id;
    }

    void implCloseTab(UID id) override
    {
        m_DeletedTabs.insert(id);
    }

    void garbageCollectDeletedTabs()
    {
        for (UID id : m_DeletedTabs)
        {
            auto it = std::find_if(m_Tabs.begin(), m_Tabs.end(), [id](auto const& o) { return o->getID() == id; });

            if (it != m_Tabs.end())
            {
                if (id == m_ActiveTab)
                {
                    it->get()->onUnmount();
                    m_ActiveTab = UID::empty();
                }
                m_Tabs.erase(it);
            }
        }
        m_DeletedTabs.clear();

        // coerce active tab, if it has become stale due to a deletion
        if (!getRequestedTab() && !getActiveTab() && !m_Tabs.empty())
        {
            m_RequestedTab = m_Tabs.front()->getID();
        }
    }

    void implResetImgui() override
    {
        m_ImguiWasAggressivelyReset = true;
    }

    // global simulation params: dictates how the next simulation shall be ran
    ParamBlock m_SimulationParams = ToParamBlock(ForwardDynamicSimulatorParams{});  // TODO: make generic

    // user-initiated output extractors
    //
    // simulators should try to hook into these, if the component exists
    std::vector<OutputExtractor> m_UserOutputExtractors;

    // user-visible UI tabs
    std::vector<std::unique_ptr<Tab>> m_Tabs;

    // set of tabs that should be deleted once control returns to this screen
    std::unordered_set<UID> m_DeletedTabs;

    // currently-active UI tab
    UID m_ActiveTab = UID::empty();

    // cached version of active tab name - used to ensure ImGui can re-focus a renamed tab
    std::string m_ActiveTabNameLastFrame;

    // a tab that should become active next frame
    UID m_RequestedTab = UID::empty();

    // true if the screen should request a redraw from the application
    bool m_ShouldRequestRedraw = false;

    // true if ImGui was aggressively reset by a tab (and, therefore, this screen should reset ImGui)
    bool m_ImguiWasAggressivelyReset = false;
};


// public API (PIMPL)

osc::MainUIScreen::MainUIScreen() :
    m_Impl{new Impl{}}
{
}

osc::MainUIScreen::MainUIScreen(std::filesystem::path p) :
    m_Impl{new Impl{p}}
{
}

osc::MainUIScreen::MainUIScreen(MainUIScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::MainUIScreen& osc::MainUIScreen::operator=(MainUIScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::MainUIScreen::~MainUIScreen() noexcept
{
    delete m_Impl;
}

void osc::MainUIScreen::onMount()
{
    m_Impl->onMount();
}

void osc::MainUIScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::MainUIScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::MainUIScreen::tick(float dt)
{
    m_Impl->tick(std::move(dt));
}

void osc::MainUIScreen::draw()
{
    m_Impl->draw();
}
