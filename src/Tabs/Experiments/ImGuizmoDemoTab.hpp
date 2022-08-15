#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

namespace osc { class TabHost; }

namespace osc
{
    class ImGuizmoDemoTab final : public Tab {
    public:
        ImGuizmoDemoTab(TabHost*);
        ImGuizmoDemoTab(ImGuizmoDemoTab const&) = delete;
        ImGuizmoDemoTab(ImGuizmoDemoTab&&) noexcept;
        ImGuizmoDemoTab& operator=(ImGuizmoDemoTab const&) = delete;
        ImGuizmoDemoTab& operator=(ImGuizmoDemoTab&&) noexcept;
        ~ImGuizmoDemoTab() noexcept override;

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