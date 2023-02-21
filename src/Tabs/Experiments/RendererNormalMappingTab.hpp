#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class RendererNormalMappingTab final : public Tab {
    public:
        static CStringView id() noexcept;

        RendererNormalMappingTab(TabHost*);
        RendererNormalMappingTab(RendererNormalMappingTab const&) = delete;
        RendererNormalMappingTab(RendererNormalMappingTab&&) noexcept;
        RendererNormalMappingTab& operator=(RendererNormalMappingTab const&) = delete;
        RendererNormalMappingTab& operator=(RendererNormalMappingTab&&) noexcept;
        ~RendererNormalMappingTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnMount() final;
        void implOnUnmount() final;
        bool implOnEvent(SDL_Event const&) final;
        void implOnTick() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}