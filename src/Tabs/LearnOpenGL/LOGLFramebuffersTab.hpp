#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class LOGLFramebuffersTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit LOGLFramebuffersTab(std::weak_ptr<TabHost>);
        LOGLFramebuffersTab(LOGLFramebuffersTab const&) = delete;
        LOGLFramebuffersTab(LOGLFramebuffersTab&&) noexcept;
        LOGLFramebuffersTab& operator=(LOGLFramebuffersTab const&) = delete;
        LOGLFramebuffersTab& operator=(LOGLFramebuffersTab&&) noexcept;
        ~LOGLFramebuffersTab() noexcept override;

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
