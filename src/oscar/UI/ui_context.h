#pragma once

#include <SDL_events.h>

namespace osc::ui::context
{
    // init global UI context
    void init();

    // shutdown UI context
    void shutdown();

    // returns true if the UI handled the event
    bool on_event(const SDL_Event&);

    // should be called at the start of each frame (e.g. `IScreen::on_draw()`)
    void on_start_new_frame();

    // should be called at the end of each frame (e.g. the end of `IScreen::on_draw()`)
    void render();
}
