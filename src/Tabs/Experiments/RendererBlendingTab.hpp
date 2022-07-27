#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

namespace osc
{
    class RendererBlendingTab final : public Tab {
    public:
        RendererBlendingTab(TabHost*);
        RendererBlendingTab(RendererBlendingTab const&) = delete;
        RendererBlendingTab(RendererBlendingTab&&) noexcept;
        RendererBlendingTab& operator=(RendererBlendingTab const&) = delete;
        RendererBlendingTab& operator=(RendererBlendingTab&&) noexcept;
        ~RendererBlendingTab() noexcept override;

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
