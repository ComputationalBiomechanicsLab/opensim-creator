#include "TabTestScreen.hpp"

#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Widgets/LogViewer.hpp"

#include <imgui.h>
#include <imgui_internal.h>  // https://github.com/ocornut/imgui/issues/3518

#include <atomic>
#include <iostream>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

namespace
{
    class TabHost;

    class Tab {
    public:
        virtual ~Tab() noexcept = default;

        void onMount() { implOnMount(); }
        void onUnmount() { implOnUnmount(); }
        bool onEvent(SDL_Event const& e) { return implOnEvent(e); }
        void tick(float dt) { implOnTick(std::move(dt)); }
        void drawMainMenu() { implDrawMainMenu(); }
        void draw() { implOnDraw(); }
        std::string const& getName() const { return m_Name; }

    protected:
        explicit Tab(TabHost* parent, std::string name) :
            m_Parent{std::move(parent)},
            m_Name{std::move(name)}
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
        virtual void implOnTick(float dt) {}
        virtual void implDrawMainMenu() {}
        virtual void implOnDraw() = 0;

        TabHost* m_Parent;
        std::string m_Name;
        bool m_DeletionRequested = false;
    };

    static std::atomic<int> g_ContentNum = 1;

    class FirstTab final : public Tab {
    public:
        FirstTab(TabHost* parent, std::string name) :
            Tab{std::move(parent), std::move(name)}
        {
        }

        void implOnDraw() override
        {
            static std::atomic<int> i = 1;

            std::string windowName = m_Content + "_subwindow";

            ImGui::Begin(windowName.c_str());

            if (ImGui::Button("add something"))
            {
                std::stringstream ss;
                ss << i++ << "_tab";
                auto tab = std::make_unique<FirstTab>(updParent(), ss.str());
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

    private:
        std::string m_Content = std::to_string(g_ContentNum++);
        osc::LogViewer m_LogViewer;
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

class osc::TabTestScreen::Impl final : public TabHost {
public:
    Impl()
    {
        m_Tabs.push_back(std::make_unique<FirstTab>(this, "first"));
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
            m_Tabs[i]->tick(dt);
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
        static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_Reorderable;

        // https://github.com/ocornut/imgui/issues/3518

        ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
        float height = ImGui::GetFrameHeight();

        if (ImGui::BeginViewportSideBar("##SecondaryMenuBar", viewport, ImGuiDir_Up, height, window_flags))
        {
            if (ImGui::BeginMenuBar())
            {
                
                ImGui::CheckboxFlags("ImGuiTabBarFlags_Reorderable", &tab_bar_flags, ImGuiTabBarFlags_Reorderable);
                ImGui::CheckboxFlags("ImGuiTabBarFlags_AutoSelectNewTabs", &tab_bar_flags, ImGuiTabBarFlags_AutoSelectNewTabs);
                ImGui::CheckboxFlags("ImGuiTabBarFlags_TabListPopupButton", &tab_bar_flags, ImGuiTabBarFlags_TabListPopupButton);
                ImGui::CheckboxFlags("ImGuiTabBarFlags_NoCloseWithMiddleMouseButton", &tab_bar_flags, ImGuiTabBarFlags_NoCloseWithMiddleMouseButton);
                if ((tab_bar_flags & ImGuiTabBarFlags_FittingPolicyMask_) == 0)
                    tab_bar_flags |= ImGuiTabBarFlags_FittingPolicyDefault_;
                if (ImGui::CheckboxFlags("ImGuiTabBarFlags_FittingPolicyResizeDown", &tab_bar_flags, ImGuiTabBarFlags_FittingPolicyResizeDown))
                    tab_bar_flags &= ~(ImGuiTabBarFlags_FittingPolicyMask_ ^ ImGuiTabBarFlags_FittingPolicyResizeDown);
                if (ImGui::CheckboxFlags("ImGuiTabBarFlags_FittingPolicyScroll", &tab_bar_flags, ImGuiTabBarFlags_FittingPolicyScroll))
                    tab_bar_flags &= ~(ImGuiTabBarFlags_FittingPolicyMask_ ^ ImGuiTabBarFlags_FittingPolicyScroll);
                ImGui::EndMenuBar();
            }
            ImGui::End();
        }

        if (ImGui::BeginViewportSideBar("##MainMenuBar", viewport, ImGuiDir_Up, height, window_flags))
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
                        bool active = true;
                        if (ImGui::BeginTabItem(m_Tabs[i]->getName().c_str(), &active, flags))
                        {
                            m_ActiveTab = i;
                            ImGui::EndTabItem();
                        }
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

        m_DeletedTabs.clear();

        return;

        if (ImGui::TreeNode("Tabs"))
        {
            if (ImGui::TreeNode("Basic"))
            {
                ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
                if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
                {
                    if (ImGui::BeginTabItem("Avocado"))
                    {
                        ImGui::Text("This is the Avocado tab!\nblah blah blah blah blah");
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Broccoli"))
                    {
                        ImGui::Text("This is the Broccoli tab!\nblah blah blah blah blah");
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Cucumber"))
                    {
                        ImGui::Text("This is the Cucumber tab!\nblah blah blah blah blah");
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
                ImGui::Separator();
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Advanced & Close Button"))
            {
                // Expose a couple of the available flags. In most cases you may just call BeginTabBar() with no flags (0).
                static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_Reorderable;
                ImGui::CheckboxFlags("ImGuiTabBarFlags_Reorderable", &tab_bar_flags, ImGuiTabBarFlags_Reorderable);
                ImGui::CheckboxFlags("ImGuiTabBarFlags_AutoSelectNewTabs", &tab_bar_flags, ImGuiTabBarFlags_AutoSelectNewTabs);
                ImGui::CheckboxFlags("ImGuiTabBarFlags_TabListPopupButton", &tab_bar_flags, ImGuiTabBarFlags_TabListPopupButton);
                ImGui::CheckboxFlags("ImGuiTabBarFlags_NoCloseWithMiddleMouseButton", &tab_bar_flags, ImGuiTabBarFlags_NoCloseWithMiddleMouseButton);
                if ((tab_bar_flags & ImGuiTabBarFlags_FittingPolicyMask_) == 0)
                    tab_bar_flags |= ImGuiTabBarFlags_FittingPolicyDefault_;
                if (ImGui::CheckboxFlags("ImGuiTabBarFlags_FittingPolicyResizeDown", &tab_bar_flags, ImGuiTabBarFlags_FittingPolicyResizeDown))
                    tab_bar_flags &= ~(ImGuiTabBarFlags_FittingPolicyMask_ ^ ImGuiTabBarFlags_FittingPolicyResizeDown);
                if (ImGui::CheckboxFlags("ImGuiTabBarFlags_FittingPolicyScroll", &tab_bar_flags, ImGuiTabBarFlags_FittingPolicyScroll))
                    tab_bar_flags &= ~(ImGuiTabBarFlags_FittingPolicyMask_ ^ ImGuiTabBarFlags_FittingPolicyScroll);

                // Tab Bar
                const char* names[4] = { "Artichoke", "Beetroot", "Celery", "Daikon" };
                static bool opened[4] = { true, true, true, true }; // Persistent user state
                for (int n = 0; n < IM_ARRAYSIZE(opened); n++)
                {
                    if (n > 0) { ImGui::SameLine(); }
                    ImGui::Checkbox(names[n], &opened[n]);
                }

                // Passing a bool* to BeginTabItem() is similar to passing one to Begin():
                // the underlying bool will be set to false when the tab is closed.
                if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(opened); n++)
                        if (opened[n] && ImGui::BeginTabItem(names[n], &opened[n], ImGuiTabItemFlags_None))
                        {
                            ImGui::Text("This is the %s tab!", names[n]);
                            if (n & 1)
                                ImGui::Text("I am an odd tab.");
                            ImGui::EndTabItem();
                        }
                    ImGui::EndTabBar();
                }
                ImGui::Separator();
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("TabItemButton & Leading/Trailing flags"))
            {
                static ImVector<int> active_tabs;
                static int next_tab_id = 0;
                if (next_tab_id == 0) // Initialize with some default tabs
                    for (int i = 0; i < 3; i++)
                        active_tabs.push_back(next_tab_id++);

                // TabItemButton() and Leading/Trailing flags are distinct features which we will demo together.
                // (It is possible to submit regular tabs with Leading/Trailing flags, or TabItemButton tabs without Leading/Trailing flags...
                // but they tend to make more sense together)
                static bool show_leading_button = true;
                static bool show_trailing_button = true;
                ImGui::Checkbox("Show Leading TabItemButton()", &show_leading_button);
                ImGui::Checkbox("Show Trailing TabItemButton()", &show_trailing_button);

                // Expose some other flags which are useful to showcase how they interact with Leading/Trailing tabs
                static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyResizeDown;
                ImGui::CheckboxFlags("ImGuiTabBarFlags_TabListPopupButton", &tab_bar_flags, ImGuiTabBarFlags_TabListPopupButton);
                if (ImGui::CheckboxFlags("ImGuiTabBarFlags_FittingPolicyResizeDown", &tab_bar_flags, ImGuiTabBarFlags_FittingPolicyResizeDown))
                    tab_bar_flags &= ~(ImGuiTabBarFlags_FittingPolicyMask_ ^ ImGuiTabBarFlags_FittingPolicyResizeDown);
                if (ImGui::CheckboxFlags("ImGuiTabBarFlags_FittingPolicyScroll", &tab_bar_flags, ImGuiTabBarFlags_FittingPolicyScroll))
                    tab_bar_flags &= ~(ImGuiTabBarFlags_FittingPolicyMask_ ^ ImGuiTabBarFlags_FittingPolicyScroll);

                if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
                {
                    // Demo a Leading TabItemButton(): click the "?" button to open a menu
                    if (show_leading_button)
                        if (ImGui::TabItemButton("?", ImGuiTabItemFlags_Leading | ImGuiTabItemFlags_NoTooltip))
                            ImGui::OpenPopup("MyHelpMenu");
                    if (ImGui::BeginPopup("MyHelpMenu"))
                    {
                        ImGui::Selectable("Hello!");
                        ImGui::EndPopup();
                    }

                    // Demo Trailing Tabs: click the "+" button to add a new tab (in your app you may want to use a font icon instead of the "+")
                    // Note that we submit it before the regular tabs, but because of the ImGuiTabItemFlags_Trailing flag it will always appear at the end.
                    if (show_trailing_button)
                        if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
                            active_tabs.push_back(next_tab_id++); // Add new tab

                                                                  // Submit our regular tabs
                    for (int n = 0; n < active_tabs.Size; )
                    {
                        bool open = true;
                        char name[16];
                        snprintf(name, IM_ARRAYSIZE(name), "%04d", active_tabs[n]);
                        if (ImGui::BeginTabItem(name, &open, ImGuiTabItemFlags_None))
                        {
                            ImGui::Text("This is the %s tab!", name);
                            ImGui::EndTabItem();
                        }

                        if (!open)
                            active_tabs.erase(active_tabs.Data + n);
                        else
                            n++;
                    }

                    ImGui::EndTabBar();
                }
                ImGui::Separator();
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }  
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
