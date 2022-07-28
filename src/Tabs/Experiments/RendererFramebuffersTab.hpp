#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

namespace osc
{
    class RendererFramebuffersTab final : public Tab {
    public:
        RendererFramebuffersTab(TabHost*);
        RendererFramebuffersTab(RendererFramebuffersTab const&) = delete;
        RendererFramebuffersTab(RendererFramebuffersTab&&) noexcept;
        RendererFramebuffersTab& operator=(RendererFramebuffersTab const&) = delete;
        RendererFramebuffersTab& operator=(RendererFramebuffersTab&&) noexcept;
        ~RendererFramebuffersTab() noexcept override;

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
