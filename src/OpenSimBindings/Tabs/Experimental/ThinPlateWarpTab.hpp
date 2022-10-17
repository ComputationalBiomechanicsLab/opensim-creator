#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

namespace osc { class TabHost; }

namespace osc
{
    class ThinPlateWarpTab final : public Tab {
    public:
        ThinPlateWarpTab(TabHost*);
        ThinPlateWarpTab(ThinPlateWarpTab const&) = delete;
        ThinPlateWarpTab(ThinPlateWarpTab&&) noexcept;
        ThinPlateWarpTab& operator=(ThinPlateWarpTab const&) = delete;
        ThinPlateWarpTab& operator=(ThinPlateWarpTab&&) noexcept;
        ~ThinPlateWarpTab() noexcept override;

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