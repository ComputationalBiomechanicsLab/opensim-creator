#pragma once

#include "src/Platform/Screen.hpp"

#include <SDL_events.h>

#include <filesystem>

namespace osc
{
    class MainEditorState;
}

namespace osc
{
    // shows a basic loading message while an .osim file loads
    class LoadingScreen final : public Screen {
    public:
        // load the supplied path (assumed to be an .osim) and then transition
        // to the editor screen
        explicit LoadingScreen(std::filesystem::path);
        
        // as above, but recycle a previous editor state (to maintain sims, etc.)
        LoadingScreen(std::shared_ptr<MainEditorState>, std::filesystem::path);

        LoadingScreen(LoadingScreen const&) = delete;
        LoadingScreen(LoadingScreen&&) noexcept;
        LoadingScreen& operator=(LoadingScreen const&) = delete;
        LoadingScreen& operator=(LoadingScreen&&) noexcept;
        ~LoadingScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;

        class Impl;
    private:
        Impl* m_Impl;
    };
}
