#pragma once

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

#include <future>
#include <memory>

namespace osc { struct AnnotatedImage; }
namespace osc { class TabHost; }

namespace osc
{
    class ScreenshotTab final : public Tab {
    public:
        ScreenshotTab(TabHost*, AnnotatedImage&&);
        ScreenshotTab(ScreenshotTab const&) = delete;
        ScreenshotTab(ScreenshotTab&&) noexcept;
        ScreenshotTab& operator=(ScreenshotTab const&) = delete;
        ScreenshotTab& operator=(ScreenshotTab&&) noexcept;
        ~ScreenshotTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        TabHost* implParent() const final;
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