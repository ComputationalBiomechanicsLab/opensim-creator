#pragma once

#include "src/Screen.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc {
    struct MainEditorState;
}

namespace osc {
    
    // main editor screen
    //
    // this is the top-level screen that users see when editing a model
    class ModelEditorScreen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;

    public:
        ModelEditorScreen(std::shared_ptr<MainEditorState>);

        ModelEditorScreen(ModelEditorScreen const&) = delete;
        ModelEditorScreen(ModelEditorScreen&&) noexcept;
        ModelEditorScreen& operator=(ModelEditorScreen const&) = delete;
        ModelEditorScreen& operator=(ModelEditorScreen&&) noexcept;
        ~ModelEditorScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;
    };
}
