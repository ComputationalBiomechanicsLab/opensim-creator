#pragma once

#include "oscar/Tabs/Tab.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/Utils/UID.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class LOGLPointShadowsTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit LOGLPointShadowsTab(std::weak_ptr<TabHost>);
        LOGLPointShadowsTab(LOGLPointShadowsTab const&) = delete;
        LOGLPointShadowsTab(LOGLPointShadowsTab&&) noexcept;
        LOGLPointShadowsTab& operator=(LOGLPointShadowsTab const&) = delete;
        LOGLPointShadowsTab& operator=(LOGLPointShadowsTab&&) noexcept;
        ~LOGLPointShadowsTab() noexcept override;

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
