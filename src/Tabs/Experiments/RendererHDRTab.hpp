#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class RendererHDRTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit RendererHDRTab(std::weak_ptr<TabHost>);
        RendererHDRTab(RendererHDRTab const&) = delete;
        RendererHDRTab(RendererHDRTab&&) noexcept;
        RendererHDRTab& operator=(RendererHDRTab const&) = delete;
        RendererHDRTab& operator=(RendererHDRTab&&) noexcept;
        ~RendererHDRTab() noexcept override;

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
