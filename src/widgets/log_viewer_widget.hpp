#pragma once

namespace osc {
    struct Log_viewer_widget_state final {
        bool autoscroll = true;
    };

    void draw_log_viewer_widget(Log_viewer_widget_state&, char const* panel_name);
}
