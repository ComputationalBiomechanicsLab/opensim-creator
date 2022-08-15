#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

namespace osc
{
    class TabHost;
}

namespace osc
{
    class ImGuiDemoTab final : public Tab {
    public:
        ImGuiDemoTab(TabHost*);
        ImGuiDemoTab(ImGuiDemoTab const&) = delete;
        ImGuiDemoTab(ImGuiDemoTab&&) noexcept;
        ImGuiDemoTab& operator=(ImGuiDemoTab const&) = delete;
        ImGuiDemoTab& operator=(ImGuiDemoTab&&) noexcept;
        ~ImGuiDemoTab() noexcept override;

    private:
        UID implGetID() const override;
        CStringView implGetName() const override;
        TabHost* implParent() const override;
        void implOnMount() override;
        void implOnUnmount() override;
        bool implOnEvent(SDL_Event const&) override;
        void implOnTick() override;
        void implOnDrawMainMenu() override;
        void implOnDraw() override;

        class Impl;
        Impl* m_Impl;
    };
}