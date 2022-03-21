#pragma once

#include "src/OpenSimBindings/MainEditorState.hpp"
#include "src/Platform/Screen.hpp"

#include <SDL_events.h>

#include <filesystem>
#include <memory>

namespace osc
{
    // shows a basic loading message while an .osim file loads
    class LoadingScreen final : public Screen {
    public:
        // load the supplied path (assumed to be an .osim) and then transition
        // to the editor screen
        LoadingScreen(std::shared_ptr<MainEditorState>, std::filesystem::path);
        ~LoadingScreen() noexcept override;

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
