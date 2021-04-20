#pragma once

namespace osc::ui::log_viewer {
    struct State final {
        bool autoscroll = true;
    };

    void draw(State&, char const* panel_name);
}
