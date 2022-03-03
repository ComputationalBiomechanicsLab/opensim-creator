#pragma once

#include "src/Screen.hpp"

#include <memory>

namespace osc
{
    class MainEditorState;
}

namespace osc
{
    // top-level splash screen
    //
    // this is shown when OSC boots and contains a list of previously opened files, etc.
    class SplashScreen final : public Screen {
    public:
        SplashScreen();

        // recycles an existing main editor state (so the user's tabs etc. persist)
        SplashScreen(std::shared_ptr<MainEditorState>);

        ~SplashScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;

        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
