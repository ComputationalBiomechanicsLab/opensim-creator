#pragma once

#include "src/Platform/Screen.hpp"

#include <SDL_events.h>

namespace osc
{
    // top-level "experiments" screen
    //
    // for development and featuretest use. This is where new functionality etc.
    // that isn't quite ready for the main UI gets dumped
    class ExperimentsScreen final : public Screen {
    public:
        ExperimentsScreen();
        ExperimentsScreen(ExperimentsScreen const&) = delete;
        ExperimentsScreen(ExperimentsScreen&&) noexcept;
        ExperimentsScreen& operator=(ExperimentsScreen const&) = delete;
        ExperimentsScreen& operator=(ExperimentsScreen&&) noexcept;
        ~ExperimentsScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void draw() override;

    private:
        class Impl;
        Impl* m_Impl;
    };
}
