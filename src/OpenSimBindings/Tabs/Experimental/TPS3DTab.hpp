#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class TPS3DTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit TPS3DTab(TabHost*);
        TPS3DTab(TPS3DTab const&) = delete;
        TPS3DTab(TPS3DTab&&) noexcept;
        TPS3DTab& operator=(TPS3DTab const&) = delete;
        TPS3DTab& operator=(TPS3DTab&&) noexcept;
        ~TPS3DTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        TabHost* implParent() const final;
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