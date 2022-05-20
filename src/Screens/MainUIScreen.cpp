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
    }

    void onUnmount()
    {
        osc::ImGuiShutdown();
    }

    void onEvent(SDL_Event const& e)
    {
        // try pumping the event into ImGui (top-prio)
        if (osc::ImGuiOnEvent(e))
        {
            m_ShouldRequestRedraw = true;
        }

        // try pumping the event into the currently-active tab
        //
        // (don't pump the event into the inactive tabs)
        if (Tab* active = getActiveTab())
        {
            bool handled = active->onEvent(e);

            // a `tab` may have requested deletions
            garbageCollectDeletedTabs();

            if (handled)
            {
                bool m_ShouldRequestRedraw = true;
                return;
            }
        }

        // finally, if it's a quit event but neither ImGui nor a tab handled it itself
        // (e.g. because the tab decided to "handle it" by prompting the user to save
        // files) then handle the quit event at this higher level
        if (e.type == SDL_QUIT)
        {
            App::upd().requestQuit();
            return;
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
        if (m_ImguiWasAggressivelyReset)
        {
            // because this marks the start of new ImGui calls
            m_ImguiWasAggressivelyReset = false;
        }

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
                        m_ImguiWasAggressivelyReset = false;
                        return;  // must return here to prevent the ImGui End calls from erroring
                    }
                }
                ImGui::EndMenuBar();
            }
            ImGui::End();
        }

        if (ImGui::BeginViewportSideBar("##TabBar", viewport, ImGuiDir_Up, height, window_flags))
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginTabBar("tabbar", tab_bar_flags))
                {
                    for (int i = 0; i < m_Tabs.size(); ++i)
                    {
                        ImGuiTabItemFlags flags = 0;
                        if (m_RequestedTab == m_Tabs[i]->getID())
                        {
                            flags |= ImGuiTabItemFlags_SetSelected;
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

                            if (m_RequestedTab == m_ActiveTab)
                            {
                                m_RequestedTab.reset();
                            }

                            if (m_ImguiWasAggressivelyReset)
                            {
                                m_ImguiWasAggressivelyReset = false;
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
        else if (!m_Tabs.empty())
        {
            m_ActiveTab = m_Tabs.front()->getID();
            m_Tabs.front()->onDraw();
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

    void implResetImgui()
    {
        osc::ImGuiShutdown();
        osc::ImGuiInit();
        osc::ImGuiNewFrame();
        m_ImguiWasAggressivelyReset = true;
        m_ShouldRequestRedraw = true;
    }

    ParamBlock m_SimulationParams = ToParamBlock(ForwardDynamicSimulatorParams{});  // TODO: make generic
    std::vector<OutputExtractor> m_UserOutputExtractors;
    std::vector<std::unique_ptr<Tab>> m_Tabs;
    std::unordered_set<UID> m_DeletedTabs;
    UID m_ActiveTab;
    UID m_RequestedTab;
    bool m_ShouldRequestRedraw = false;
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
