#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class RendererSSAOTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit RendererSSAOTab(std::weak_ptr<TabHost>);
        RendererSSAOTab(RendererSSAOTab const&) = delete;
        RendererSSAOTab(RendererSSAOTab&&) noexcept;
        RendererSSAOTab& operator=(RendererSSAOTab const&) = delete;
        RendererSSAOTab& operator=(RendererSSAOTab&&) noexcept;
        ~RendererSSAOTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnMount() final;
        void implOnUnmount() final;
        bool implOnEvent(SDL_Event const&) final;
        void implOnTick() final;
        void implOnDrawMainMenu() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}