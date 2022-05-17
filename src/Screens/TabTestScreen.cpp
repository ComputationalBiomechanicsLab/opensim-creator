#include "TabTestScreen.hpp"

#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Widgets/LogViewer.hpp"
#include "src/Utils/CStringView.hpp"

#include <imgui.h>
#include <imgui_internal.h>  // https://github.com/ocornut/imgui/issues/3518

#include <atomic>
#include <iostream>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>


// example tab API
namespace
{
    class TabHost;

    class Tab {
    public:
        virtual ~Tab() noexcept = default;

        void onMount() { implOnMount(); }
        void onUnmount() { implOnUnmount(); }
        bool onEvent(SDL_Event const& e) { return implOnEvent(e); }
        void tick() { implOnTick(); }
        void drawMainMenu() { implDrawMainMenu(); }
        void draw() { implOnDraw(); }
        osc::CStringView name() { return implName(); }

    protected:
        explicit Tab(TabHost* parent) : m_Parent{ std::move(parent) }
        {
        }

        TabHost* updParent() { return m_Parent; }
        void addTab(std::unique_ptr<Tab>);
        void selectTab(Tab*);
        void closeTab(Tab*);

    private:
        virtual void implOnMount() {}
        virtual void implOnUnmount() {}
        virtual bool implOnEvent(SDL_Event const& e) { return false; }
        virtual void implOnTick() {}
        virtual void implDrawMainMenu() {}
        virtual void implOnDraw() = 0;
        virtual osc::CStringView implName() = 0;

        TabHost* m_Parent;
    };

    class TabHost {
    public:
        virtual ~TabHost() noexcept = default;
        void addTab(std::unique_ptr<Tab> tab) { implAddTab(std::move(tab)); }
        void selectTab(Tab* t) { implSelectTab(std::move(t)); }
        void closeTab(Tab* t) { implCloseTab(std::move(t)); }

    private:
        virtual void implAddTab(std::unique_ptr<Tab> tab) = 0;
        virtual void implSelectTab(Tab*) = 0;
        virtual void implCloseTab(Tab*) = 0;
    };

    void Tab::addTab(std::unique_ptr<Tab> tab)
    {
        m_Parent->addTab(std::move(tab));
    }

    void Tab::selectTab(Tab* t)
    {
        m_Parent->selectTab(std::move(t));
    }

    void Tab::closeTab(Tab* t)
    {
        m_Parent->closeTab(std::move(t));
    }
}

namespace
{
    static std::atomic<int> g_ContentNum = 1;

    std::unique_ptr<Tab> MakeTabType2(TabHost*, std::string);

    // example tab impl #1
    class TabDemo1 final : public Tab {
    public:
        TabDemo1(TabHost* parent, std::string name) :
            Tab{std::move(parent)},
            m_BaseName{std::move(name)}
        {
        }

    private:

        void implOnDraw() override
        {
            static std::atomic<int> i = 1;

            std::string windowName = m_BaseName + "_subwindow";

            ImGui::Begin(windowName.c_str());

            if (ImGui::Button("add tab type 1"))
            {
                std::stringstream ss;
                ss << i++ << "_tab";
                auto tab = std::make_unique<TabDemo1>(updParent(), ss.str());
                auto* tabPtr = tab.get();
                addTab(std::move(tab));
                selectTab(tabPtr);
            }

            if (ImGui::Button("add tab type 2"))
            {
                std::stringstream ss;
                ss << i++ << "_tab";
                auto tab = MakeTabType2(updParent(), ss.str());
                auto* tabPtr = tab.get();
                addTab(std::move(tab));
                selectTab(tabPtr);
            }

            if (ImGui::Button("remove me"))
            {
                closeTab(this);
            }

            ImGui::Text("%s", m_Content.c_str());

            ImGui::End();

            ImGui::Begin("log");
            m_LogViewer.draw();
            ImGui::End();
        }

        void implDrawMainMenu() override
        {
            if (ImGui::MenuItem("set content"))
            {
                m_Content = std::to_string(g_ContentNum++);
            }
        }

        osc::CStringView implName() override
        {
            return m_BaseName;
        }

        std::string m_BaseName;
        std::string m_Content = std::to_string(g_ContentNum++);
        osc::LogViewer m_LogViewer;
    };

    // example impl 2
    class TabDemo2 final : public Tab {
    public:
        TabDemo2(TabHost* parent, std::string name) :
            Tab{std::move(parent)},
            m_BaseName{std::move(name)}
        {
        }

    private:
        void implOnDraw() override
        {
            std::string windowName = m_BaseName + "_subwindow";

            ImGui::Begin(windowName.c_str());
            ImGui::Text("this just shows that the tab host can host tabs with different types");
            ImGui::End();
        }

        void implDrawMainMenu() override
        {
            ImGui::MenuItem("menu2");
        }

        osc::CStringView implName() override
        {
            return m_BaseName;
        }

        std::string m_BaseName;
    };

    std::unique_ptr<Tab> MakeTabType2(TabHost* parent, std::string name)
    {
        return std::make_unique<TabDemo2>(std::move(parent), std::move(name));
    }
}

class osc::TabTestScreen::Impl final : public TabHost {
public:
    Impl()
    {
        m_Tabs.push_back(std::make_unique<TabDemo1>(this, "first"));
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
            App::upd().requestQuit();
            return;
        }
        else if (osc::ImGuiOnEvent(e))
        {
            return;
        }
        if (0 <= m_ActiveTab && m_ActiveTab < m_Tabs.size())
        {
            if (m_Tabs[m_ActiveTab]->onEvent(e))
            {
                return;
            }
        }
    }

    void tick(float dt)
    {
        for (int i = 0; i < m_Tabs.size(); ++i)
        {
            m_Tabs[i]->tick();
        }
    }

    void draw()
    {

        osc::ImGuiNewFrame();
        App::upd().clearScreen({0.0f, 0.0f, 0.0f, 0.0f});
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
        drawUI();
        osc::ImGuiRender();
    }

    void drawUI()
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
                            m_ActiveTab = i;
                            ImGui::EndTabItem();
                        }
                        ImGui::PopID();
                        if (!active)
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

private:
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
        m_DeletedTabs.insert(m_DeletedTabs.end(), std::make_move_iterator(it), std::make_move_iterator(m_Tabs.end()));
        m_Tabs.erase(it, m_Tabs.end());
    }

    std::vector<std::unique_ptr<Tab>> m_Tabs;
    std::vector<std::unique_ptr<Tab>> m_DeletedTabs;
    int m_ActiveTab = -1;
    int m_RequestedTab = -1;
};


// public API (PIMPL)

osc::TabTestScreen::TabTestScreen() :
    m_Impl{new Impl{}}
{
}

osc::TabTestScreen::TabTestScreen(TabTestScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::TabTestScreen& osc::TabTestScreen::operator=(TabTestScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::TabTestScreen::~TabTestScreen() noexcept
{
    delete m_Impl;
}

void osc::TabTestScreen::onMount()
{
    m_Impl->onMount();
}

void osc::TabTestScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::TabTestScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::TabTestScreen::tick(float dt)
{
    m_Impl->tick(std::move(dt));
}

void osc::TabTestScreen::draw()
{
    m_Impl->draw();
}
