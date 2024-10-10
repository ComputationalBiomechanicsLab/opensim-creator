#pragma once

namespace osc { class App; }
namespace osc { class Event; }

namespace osc::ui::context
{
    // init global UI context
    void init(App&);

    // shutdown UI context
    void shutdown(App&);

    // returns true if the UI handled the event
    bool on_event(Event&);

    // should be called at the start of each frame (e.g. `IScreen::on_draw()`)
    void on_start_new_frame(App&);

    // should be called at the end of each frame (e.g. the end of `IScreen::on_draw()`)
    void render();
}
