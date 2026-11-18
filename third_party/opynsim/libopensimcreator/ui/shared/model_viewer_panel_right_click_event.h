#pragma once

#include <liboscar/maths/rect.h>
#include <liboscar/maths/vector.h>

#include <optional>
#include <string>
#include <utility>

namespace osc
{
    struct ModelViewerPanelRightClickEvent final {

        ModelViewerPanelRightClickEvent(
            std::string source_panel_name,
            const Rect& viewport_screen_rect,
            std::string component_abs_path_or_empty,
            std::optional<Vector3> maybe_click_position_in_ground) :

            source_panel_name{std::move(source_panel_name)},
            viewport_screen_rect{viewport_screen_rect},
            component_abs_path_or_empty{std::move(component_abs_path_or_empty)},
            maybe_click_position_in_ground{maybe_click_position_in_ground}
        {}

        std::string source_panel_name;
        Rect viewport_screen_rect;
        std::string component_abs_path_or_empty;
        std::optional<Vector3> maybe_click_position_in_ground;
    };
}
