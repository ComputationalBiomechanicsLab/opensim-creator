#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class LOGLMultipleLightsTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit LOGLMultipleLightsTab(std::weak_ptr<TabHost>);
        LOGLMultipleLightsTab(LOGLMultipleLightsTab const&) = delete;
        LOGLMultipleLightsTab(LOGLMultipleLightsTab&&) noexcept;
        LOGLMultipleLightsTab& operator=(LOGLMultipleLightsTab const&) = delete;
        LOGLMultipleLightsTab& operator=(LOGLMultipleLightsTab&&) noexcept;
        ~LOGLMultipleLightsTab() noexcept final;

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
