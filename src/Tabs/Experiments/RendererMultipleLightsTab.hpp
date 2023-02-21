#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class RendererMultipleLightsTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit RendererMultipleLightsTab(std::weak_ptr<TabHost>);
        RendererMultipleLightsTab(RendererMultipleLightsTab const&) = delete;
        RendererMultipleLightsTab(RendererMultipleLightsTab&&) noexcept;
        RendererMultipleLightsTab& operator=(RendererMultipleLightsTab const&) = delete;
        RendererMultipleLightsTab& operator=(RendererMultipleLightsTab&&) noexcept;
        ~RendererMultipleLightsTab() noexcept final;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnMount() final;
        void implOnUnmount() final;
        bool implOnEvent(SDL_Event const&) final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
