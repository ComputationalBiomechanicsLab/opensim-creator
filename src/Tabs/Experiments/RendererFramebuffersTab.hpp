#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class RendererFramebuffersTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit RendererFramebuffersTab(std::weak_ptr<TabHost>);
        RendererFramebuffersTab(RendererFramebuffersTab const&) = delete;
        RendererFramebuffersTab(RendererFramebuffersTab&&) noexcept;
        RendererFramebuffersTab& operator=(RendererFramebuffersTab const&) = delete;
        RendererFramebuffersTab& operator=(RendererFramebuffersTab&&) noexcept;
        ~RendererFramebuffersTab() noexcept override;

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
