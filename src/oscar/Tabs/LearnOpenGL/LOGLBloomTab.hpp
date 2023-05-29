#pragma once

#include "oscar/Tabs/Tab.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/Utils/UID.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class LOGLBloomTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit LOGLBloomTab(std::weak_ptr<TabHost>);
        LOGLBloomTab(LOGLBloomTab const&) = delete;
        LOGLBloomTab(LOGLBloomTab&&) noexcept;
        LOGLBloomTab& operator=(LOGLBloomTab const&) = delete;
        LOGLBloomTab& operator=(LOGLBloomTab&&) noexcept;
        ~LOGLBloomTab() noexcept override;

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