#include "GuiRuler.h"

#include <liboscar/Graphics/Color.h>
#include <liboscar/Graphics/Scene/SceneCollision.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/PolarPerspectiveCamera.h>
#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/Utils/CStringView.h>

#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

void osc::GuiRuler::on_draw(
    const PolarPerspectiveCamera& camera,
    const Rect& render_rect,
    std::optional<SceneCollision> maybe_mouseover)
{
    if (state_ == State::Inactive) {
        return;
    }

    // users can exit measuring through these actions
    if (ui::is_key_down(Key::Escape) or ui::is_mouse_released(ui::MouseButton::Right)) {
        stop_measuring();
        return;
    }

    // users can "finish" the measurement through these actions
    if (state_ == State::WaitingForSecondPoint and ui::is_mouse_released(ui::MouseButton::Left)) {
        stop_measuring();
        return;
    }

    const Vec2 mouse_ui_pos = ui::get_mouse_ui_pos();
    const Color circle_moused_over_nothing_color = Color::red().with_alpha(0.6f);
    const Color circle_color = Color::white().with_alpha(0.8f);
    const Color line_color = circle_color;
    const Color text_background_color = Color::white();
    const Color text_color = Color::black();
    const float circle_radius = 5.0f;
    const float line_thickness = 3.0f;

    ui::DrawListView draw_list = ui::get_panel_draw_list();
    const auto draw_tooltip_with_bg = [&draw_list, &text_background_color, &text_color](const Vec2& pos, CStringView tooltip_text)
    {
        const Vec2 text_size = ui::calc_text_size(tooltip_text);
        const float background_padding = 5.0f;
        const float edge_rounding = background_padding - 2.0f;

        const Rect background_rect = {
            {pos - background_padding},
            {pos + text_size + background_padding},
        };
        draw_list.add_rect_filled(background_rect, text_background_color, edge_rounding);
        draw_list.add_text(pos, text_color, tooltip_text);
    };

    if (state_ == State::WaitingForFirstPoint) {
        if (not maybe_mouseover) {
            // not mousing over anything
            draw_list.add_circle_filled(Circle{mouse_ui_pos, circle_radius}, circle_moused_over_nothing_color);
            return;
        }
        else {
            // mousing over something
            draw_list.add_circle_filled(Circle{mouse_ui_pos, circle_radius}, circle_color);

            if (ui::is_mouse_released(ui::MouseButton::Left)) {
                state_ = State::WaitingForSecondPoint;
                start_world_pos_ = maybe_mouseover->worldspace_location;
            }
            return;
        }
    }
    else if (state_ == State::WaitingForSecondPoint) {
        const Vec2 start_ui_pos = camera.project_onto_viewport(start_world_pos_, render_rect);

        if (maybe_mouseover) {
            // user is moused over something, so draw a line + circle between the two hitlocs
            const Vec2 end_ui_pos = mouse_ui_pos;
            const Vec2 line_ui_direction = normalize(start_ui_pos - end_ui_pos);
            const Vec2 offset_vec = 15.0f * Vec2{line_ui_direction.y, -line_ui_direction.x};
            const Vec2 line_midpoint = (start_ui_pos + end_ui_pos) / 2.0f;
            const float line_world_length = length(maybe_mouseover->worldspace_location - start_world_pos_);

            draw_list.add_circle_filled({start_ui_pos, circle_radius}, circle_color);
            draw_list.add_line(start_ui_pos, end_ui_pos, line_color, line_thickness);
            draw_list.add_circle_filled({end_ui_pos, circle_radius}, circle_color);

            // label the line's length
            {
                const std::string line_len_label = [&line_world_length]()
                {
                    std::stringstream ss;
                    ss << std::setprecision(5) << line_world_length;
                    return std::move(ss).str();
                }();
                draw_tooltip_with_bg(line_midpoint + offset_vec, line_len_label);
            }
        }
        else {
            draw_list.add_circle_filled({start_ui_pos, circle_radius}, circle_color);
        }
    }
}

void osc::GuiRuler::start_measuring()
{
    state_ = State::WaitingForFirstPoint;
}

void osc::GuiRuler::stop_measuring()
{
    state_ = State::Inactive;
}

void osc::GuiRuler::toggle_measuring()
{
    if (state_ == State::Inactive) {
        state_ = State::WaitingForFirstPoint;
    }
    else {
        state_ = State::Inactive;
    }
}

bool osc::GuiRuler::is_measuring() const
{
    return state_ != State::Inactive;
}
