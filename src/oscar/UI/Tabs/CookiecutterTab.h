#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <SDL_events.h>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class CookiecutterTab final : public ITab {
    public:
        static CStringView id();

        explicit CookiecutterTab(const ParentPtr<ITabHost>&);
        CookiecutterTab(const CookiecutterTab&) = delete;
        CookiecutterTab(CookiecutterTab&&) noexcept;
        CookiecutterTab& operator=(const CookiecutterTab&) = delete;
        CookiecutterTab& operator=(CookiecutterTab&&) noexcept;
        ~CookiecutterTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnMount() final;
        void implOnUnmount() final;
        bool implOnEvent(const SDL_Event&) final;
        void implOnTick() final;
        void implOnDrawMainMenu() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
