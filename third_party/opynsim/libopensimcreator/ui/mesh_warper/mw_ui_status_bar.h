#pragma once

#include <libopensimcreator/ui/mesh_warper/mw_ui_shared_state.h>

#include <liboscar/graphics/color.h>
#include <liboscar/maths/vector.h>
#include <liboscar/ui/oscimgui.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace osc
{
    // widget: bottom status bar (shows status messages, hover information, etc.)
    class MwUiStatusBar final {
    public:
        MwUiStatusBar(std::string_view label, std::shared_ptr<const MwUiSharedState> tab_state) :

            label_{label},
            state_{std::move(tab_state)}
        {}

        void on_draw()
        {
            if (ui::begin_main_window_bottom_bar(label_)) {
                draw_content();
            }
            ui::end_panel();
        }

    private:
        void draw_content()
        {
            if (state_->is_hovering_something()) {
                draw_current_hover_info(state_->get_current_hover());
            } else {
                ui::draw_text_disabled("(nothing hovered)");
            }
        }

        void draw_current_hover_info(const MwUiHovertestResult& hover)
        {
            draw_color_coded_xyz(hover.get_world_space_location());
            ui::same_line();
            if (hover.is_hovering_a_scene_element()) {
                ui::draw_text_disabled(
                    "(Click: select {})",
                    find_element_name_or(
                        state_->get_scratch(), hover.get_scene_element_id().value()));
            } else if (hover.get_input() == MiDocumentInputIdentifier::Source) {
                ui::draw_text_disabled(
                    "(Click: add a landmark, Ctrl+Click: add non-participating landmark)");
            } else {
                static_assert(num_options_v<MiDocumentInputIdentifier> == 2);
                ui::draw_text_disabled("(Click: add a landmark)");
            }
        }

        void draw_color_coded_xyz(Vector3 position)
        {
            ui::draw_text("(");
            ui::same_line();
            for (int i = 0; i < 3; ++i) {
                Color color = {0.5f, 0.5f, 0.5f, 1.0f};
                color[i] = 1.0f;

                ui::push_style_color(ui::ColorVar::Text, color);
                ui::draw_text("{}", position[i]);
                ui::same_line();
                ui::pop_style_color();
            }
            ui::draw_text(")");
        }

        std::string label_;
        std::shared_ptr<const MwUiSharedState> state_;
    };
}
