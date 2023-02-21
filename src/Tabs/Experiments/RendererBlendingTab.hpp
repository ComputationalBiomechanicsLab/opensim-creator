#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc { class TabHost; }

namespace osc
{
    class RendererBlendingTab final : public Tab {
    public:
        static CStringView id() noexcept;

        explicit RendererBlendingTab(std::weak_ptr<TabHost>);
        RendererBlendingTab(RendererBlendingTab const&) = delete;
        RendererBlendingTab(RendererBlendingTab&&) noexcept;
        RendererBlendingTab& operator=(RendererBlendingTab const&) = delete;
        RendererBlendingTab& operator=(RendererBlendingTab&&) noexcept;
        ~RendererBlendingTab() noexcept override;

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
