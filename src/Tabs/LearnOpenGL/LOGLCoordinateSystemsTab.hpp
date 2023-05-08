#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class LOGLCoordinateSystemsTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit LOGLCoordinateSystemsTab(std::weak_ptr<TabHost>);
        LOGLCoordinateSystemsTab(LOGLCoordinateSystemsTab const&) = delete;
        LOGLCoordinateSystemsTab(LOGLCoordinateSystemsTab&&) noexcept;
        LOGLCoordinateSystemsTab& operator=(LOGLCoordinateSystemsTab const&) = delete;
        LOGLCoordinateSystemsTab& operator=(LOGLCoordinateSystemsTab&&) noexcept;
        ~LOGLCoordinateSystemsTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnMount() final;
        void implOnUnmount() final;
        bool implOnEvent(SDL_Event const&) final;
        void implOnTick() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
