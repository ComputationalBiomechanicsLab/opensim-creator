#include "MainUIScreen.hpp"

#include "src/Platform/App.hpp"
#include "src/Tabs/SplashTab.hpp"
#include "src/Tabs/Tab.hpp"
#include "src/Tabs/TabHost.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <utility>

class osc::MainUIScreen::Impl final : public osc::TabHost {
public:
    Impl()
    {
        m_Tabs.push_back(std::make_unique<SplashTab>(this));
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
        if (osc::ImGuiOnEvent(e))
        {
            return;
        }
        else if (0 <= m_ActiveTab && m_ActiveTab < m_Tabs.size())
        {
            if (m_Tabs[m_ActiveTab]->onEvent(e))
            {
                return;
            }
        }
    }

    void tick(float dt)
    {
        for (auto const& tab : m_Tabs)
        {
            tab->tick();
        }
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
        static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;

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
                    m_Tabs[m_ActiveTab]->drawMainMenu();
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
                        if (ImGui::BeginTabItem(m_Tabs[i]->name().c_str(), &active, flags))
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
                            m_Tabs.erase(m_Tabs.begin() + i);
                        }
                    }
                }
                ImGui::EndMainMenuBar();
            }
            ImGui::End();
        }

        if (0 <= m_ActiveTab && m_ActiveTab < m_Tabs.size())
        {
            m_Tabs[m_ActiveTab]->draw();
        }
        else if (!m_Tabs.empty())
        {
            m_ActiveTab = 0;
            m_Tabs[m_ActiveTab]->draw();
        }

        // clear the flagged-to-be-deleted tabs
        m_DeletedTabs.clear();
    }

    void implAddTab(std::unique_ptr<Tab> tab) override
    {
        m_Tabs.push_back(std::move(tab));
    }

    void implSelectTab(Tab* t) override
    {
        auto it = std::find_if(m_Tabs.begin(), m_Tabs.end(), [t](auto const& o) { return o.get() == t; });
        if (it != m_Tabs.end())
        {
            m_RequestedTab = static_cast<int>(std::distance(m_Tabs.begin(), it));
        }
    }

    void implCloseTab(Tab* t) override
    {
        auto it = std::stable_partition(m_Tabs.begin(), m_Tabs.end(), [t](auto const& o) { return o.get() != t; });

        // can't close the splash tab!
        if (std::distance(m_Tabs.begin(), it) != 0)
        {
            m_DeletedTabs.insert(m_DeletedTabs.end(), std::make_move_iterator(it), std::make_move_iterator(m_Tabs.end()));
            m_Tabs.erase(it, m_Tabs.end());
        }
    }

    std::vector<std::unique_ptr<Tab>> m_Tabs;
    std::vector<std::unique_ptr<Tab>> m_DeletedTabs;
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
