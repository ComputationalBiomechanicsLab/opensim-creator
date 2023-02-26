#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class RendererParallaxMappingTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit RendererParallaxMappingTab(std::weak_ptr<TabHost>);
        RendererParallaxMappingTab(RendererParallaxMappingTab const&) = delete;
        RendererParallaxMappingTab(RendererParallaxMappingTab&&) noexcept;
        RendererParallaxMappingTab& operator=(RendererParallaxMappingTab const&) = delete;
        RendererParallaxMappingTab& operator=(RendererParallaxMappingTab&&) noexcept;
        ~RendererParallaxMappingTab() noexcept override;

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