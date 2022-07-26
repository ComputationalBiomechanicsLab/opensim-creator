#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

namespace osc
{
    class RendererMultipleLightsTab final : public Tab {
    public:
        RendererMultipleLightsTab(TabHost*);
        RendererMultipleLightsTab(RendererMultipleLightsTab const&) = delete;
        RendererMultipleLightsTab(RendererMultipleLightsTab&&) noexcept;
        RendererMultipleLightsTab& operator=(RendererMultipleLightsTab const&) = delete;
        RendererMultipleLightsTab& operator=(RendererMultipleLightsTab&&) noexcept;
        ~RendererMultipleLightsTab() noexcept override;

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
