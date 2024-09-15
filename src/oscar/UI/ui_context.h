#pragma once

namespace osc { class Event; }

namespace osc::ui::context
{
    // init global UI context
    void init();

    // shutdown UI context
    void shutdown();

    // returns true if the UI handled the event
    bool on_event(const Event&);

    // should be called at the start of each frame (e.g. `IScreen::on_draw()`)
    void on_start_new_frame();

    // should be called at the end of each frame (e.g. the end of `IScreen::on_draw()`)
    void render();
}
