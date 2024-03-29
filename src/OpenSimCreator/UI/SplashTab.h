#pragma once

#include <SDL_events.h>
#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

namespace osc { class IMainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }

namespace osc
{
    class SplashTab final : public ITab {
    public:
        explicit SplashTab(ParentPtr<IMainUIStateAPI> const&);
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
