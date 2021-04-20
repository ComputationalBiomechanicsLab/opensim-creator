#pragma once

namespace osc::widgets::log_viewer {
    struct State final {
        bool autoscroll = true;
    };

    void draw(State&, char const* panel_name);
}
