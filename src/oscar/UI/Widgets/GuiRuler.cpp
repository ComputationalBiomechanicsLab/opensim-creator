#include "GuiRuler.h"

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Scene/SceneCollision.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/CStringView.h>

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
    if (ui::is_key_down(ImGuiKey_Escape) or ui::is_mouse_released(ui::MouseButton::Right)) {
        stop_measuring();
        return;
    }

    // users can "finish" the measurement through these actions
    if (state_ == State::WaitingForSecondPoint and ui::is_mouse_released(ui::MouseButton::Left)) {
        stop_measuring();
        return;
    }

    const Vec2 mouse_pos = ui::get_mouse_pos();
    const ImU32 circle_moused_over_nothing_color = ui::to_ImU32(Color::red().with_alpha(0.6f));
    const ImU32 circle_color = ui::to_ImU32(Color::white().with_alpha(0.8f));
    const ImU32 line_color = circle_color;
    const ImU32 text_background_color = ui::to_ImU32(Color::white());
    const ImU32 text_color = ui::to_ImU32(Color::black());
    const float circle_radius = 5.0f;
    const float line_thickness = 3.0f;

    ImDrawList& drawlist = *ui::get_panel_draw_list();
    const auto draw_tooltip_with_bg = [&drawlist, &text_background_color, &text_color](const Vec2& pos, CStringView tooltip_text)
    {
        const Vec2 text_size = ui::calc_text_size(tooltip_text);
        const float background_padding = 5.0f;
        const float edge_rounding = background_padding - 2.0f;

        drawlist.AddRectFilled(pos - background_padding, pos + text_size + background_padding, text_background_color, edge_rounding);
        drawlist.AddText(pos, text_color, tooltip_text.c_str());
    };

    if (state_ == State::WaitingForFirstPoint) {
        if (not maybe_mouseover) {
            // not mousing over anything
            drawlist.AddCircleFilled(mouse_pos, circle_radius, circle_moused_over_nothing_color);
            return;
        }
        else {
            // mousing over something
            drawlist.AddCircleFilled(mouse_pos, circle_radius, circle_color);

            if (ui::is_mouse_released(ui::MouseButton::Left)) {
                state_ = State::WaitingForSecondPoint;
                start_world_pos_ = maybe_mouseover->worldspace_location;
            }
            return;
        }
    }
    else if (state_ == State::WaitingForSecondPoint) {
        const Vec2 start_screenpos = camera.project_onto_screen_rect(start_world_pos_, render_rect);

        if (maybe_mouseover) {
            // user is moused over something, so draw a line + circle between the two hitlocs
            const Vec2 end_screenpos = mouse_pos;
            const Vec2 line_screen_direction = normalize(start_screenpos - end_screenpos);
            const Vec2 offset_vec = 15.0f * Vec2{line_screen_direction.y, -line_screen_direction.x};
            const Vec2 line_midpoint = (start_screenpos + end_screenpos) / 2.0f;
            const float line_world_length = length(maybe_mouseover->worldspace_location - start_world_pos_);

            drawlist.AddCircleFilled(start_screenpos, circle_radius, circle_color);
            drawlist.AddLine(start_screenpos, end_screenpos, line_color, line_thickness);
            drawlist.AddCircleFilled(end_screenpos, circle_radius, circle_color);

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
            drawlist.AddCircleFilled(start_screenpos, circle_radius, circle_color);
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
