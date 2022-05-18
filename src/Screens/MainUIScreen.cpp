#include "MainUIScreen.hpp"

#include "src/Platform/App.hpp"
#include "src/Tabs/CookiecutterTab.hpp"
#include "src/Tabs/ErrorTab.hpp"
#include "src/Tabs/MeshImporterTab.hpp"
#include "src/Tabs/SplashTab.hpp"
#include "src/Tabs/Tab.hpp"
#include "src/Tabs/TabHost.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <utility>
#include <unordered_set>

class osc::MainUIScreen::Impl final : public osc::TabHost {
public:
    Impl()
    {
        m_Tabs.push_back(std::make_unique<SplashTab>(this));
        m_Tabs.push_back(std::make_unique<ErrorTab>(this, std::runtime_error{ "hi" }));
        m_Tabs.push_back(std::make_unique<CookiecutterTab>(this));
        m_Tabs.push_back(std::make_unique<MeshImporterTab>(this));
        m_Tabs.push_back(std::make_unique<MeshImporterTab>(this));
        m_RequestedTab = 0;
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
            return;
        }

        // try pumping the event into the currently-active tab
        //
        // (don't pump the event into the inactive tabs)
        if (0 <= m_ActiveTab && m_ActiveTab < m_Tabs.size())
        {
            bool handled = m_Tabs[m_ActiveTab]->onEvent(e);

            // a `tab` may have requested deletions
            garbageCollectDeletedTabs();

            if (handled)
            {
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
                if (0 <= m_ActiveTab && m_ActiveTab < m_Tabs.size())
                {
                    m_Tabs[m_ActiveTab]->onDrawMainMenu();
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
                        if (m_RequestedTab == i)
                        {
                            flags |= ImGuiTabItemFlags_SetSelected;
                            m_RequestedTab = -1;
                        }
                        ImGui::PushID(m_Tabs[i].get());
                        bool active = true;
                        if (ImGui::BeginTabItem(m_Tabs[i]->getName().c_str(), &active, flags))
                        {
                            if (i != m_ActiveTab)
                            {
                                if (0 <= m_ActiveTab && m_ActiveTab < m_Tabs.size())
                                {
                                    m_Tabs[m_ActiveTab]->onUnmount();
                                }
                                m_ActiveTab = i;
                                m_Tabs[m_ActiveTab]->onMount();
                            }
                            m_ActiveTab = i;
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

        if (0 <= m_ActiveTab && m_ActiveTab < m_Tabs.size())
        {
            m_Tabs[m_ActiveTab]->onDraw();
        }
        else if (!m_Tabs.empty())
        {
            m_ActiveTab = 0;
            m_Tabs[m_ActiveTab]->onDraw();
        }

        // clear the flagged-to-be-deleted tabs
        garbageCollectDeletedTabs();
    }

    void implAddTab(std::unique_ptr<Tab> tab) override
    {
        m_Tabs.push_back(std::move(tab));
    }

    void implSelectTab(UID id) override
    {
        auto it = std::find_if(m_Tabs.begin(), m_Tabs.end(), [id](auto const& o) { return o->getID() == id; });
        if (it != m_Tabs.end())
        {
            m_RequestedTab = static_cast<int>(std::distance(m_Tabs.begin(), it));
        }
    }

    void implCloseTab(UID id) override
    {
        m_DeletedTabs.insert(id);
    }

    void garbageCollectDeletedTabs()
    {
        for (UID id : m_DeletedTabs)
        {
            auto it = std::remove_if(m_Tabs.begin(), m_Tabs.end(), [id](auto const& o) { return o->getID() == id; });
            m_Tabs.erase(it, m_Tabs.end());
        }
        m_DeletedTabs.clear();

        // coerce active tab, if it has become stale due to a deletion
        if (m_RequestedTab == -1 && !(0 <= m_ActiveTab && m_ActiveTab < m_Tabs.size()))
        {
            m_RequestedTab = m_Tabs.empty() ? 0 : static_cast<int>(m_Tabs.size() - 1);
        }
    }

    std::vector<std::unique_ptr<Tab>> m_Tabs;
    std::unordered_set<UID> m_DeletedTabs;
    int m_ActiveTab = -1;
    int m_RequestedTab = -1;
};


// public API (PIMPL)

osc::MainUIScreen::MainUIScreen() :
    m_Impl{new Impl{}}
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
