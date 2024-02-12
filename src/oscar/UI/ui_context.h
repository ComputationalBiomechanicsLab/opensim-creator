#pragma once

#include <SDL_events.h>

namespace osc::ui::context
{
    // init global UI context
    void Init();

    // shutdown UI context
    void Shutdown();

    // returns true if the UI handled the event
    bool OnEvent(SDL_Event const&);

    // should be called at the start of each frame (e.g. `IScreen::onDraw()`)
    void NewFrame();

    // should be called at the end of each frame (e.g. the end of `IScreen::onDraw()`)
    void Render();
}
