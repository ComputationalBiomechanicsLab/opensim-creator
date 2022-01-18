#pragma once

#include "src/Screen.hpp"

#include <memory>

namespace osc
{
    // top-level "experiments" screen
    //
    // for development and featuretest use. This is where new functionality etc.
    // that isn't quite ready for the main UI gets dumped
    class ExperimentsScreen final : public Screen {
    public:
        ExperimentsScreen();
        ~ExperimentsScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void draw() override;

        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
