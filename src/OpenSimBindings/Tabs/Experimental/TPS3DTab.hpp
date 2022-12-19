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

        TPS3DTab(TabHost*);
        TPS3DTab(TPS3DTab const&) = delete;
        TPS3DTab(TPS3DTab&&) noexcept;
        TPS3DTab& operator=(TPS3DTab const&) = delete;
        TPS3DTab& operator=(TPS3DTab&&) noexcept;
        ~TPS3DTab() noexcept override;

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
        std::unique_ptr<Impl> m_Impl;
    };
}