#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

namespace osc { class MainUIStateAPI; }
namespace osc { class TabHost; }

namespace osc
{
    class SplashTab final : public Tab {
    public:
        SplashTab(MainUIStateAPI*);
        SplashTab(SplashTab const&) = delete;
        SplashTab(SplashTab&&) noexcept;
        SplashTab& operator=(SplashTab const&) = delete;
        SplashTab& operator=(SplashTab&&) noexcept;
        ~SplashTab() noexcept override;

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
