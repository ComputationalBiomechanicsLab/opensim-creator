#pragma once

#include <oscar/Tabs/Tab.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <SDL_events.h>

#include <memory>

namespace osc { class MainUIStateAPI; }

namespace osc
{
    class SplashTab final : public Tab {
    public:
        explicit SplashTab(std::weak_ptr<MainUIStateAPI>);
        SplashTab(SplashTab const&) = delete;
        SplashTab(SplashTab&&) noexcept;
        SplashTab& operator=(SplashTab const&) = delete;
        SplashTab& operator=(SplashTab&&) noexcept;
        ~SplashTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnMount() final;
        void implOnUnmount() final;
        bool implOnEvent(SDL_Event const&) final;
        void implOnDrawMainMenu() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
