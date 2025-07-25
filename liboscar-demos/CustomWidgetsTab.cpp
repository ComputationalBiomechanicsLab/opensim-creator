#include "CustomWidgetsTab.h"

#include <liboscar/oscar.h>

#include <memory>

using namespace osc;

namespace
{
    void draw_widget_title(CStringView title, Vec2 pos)
    {
        const Vec2 text_top_left = pos + ui::get_style_frame_padding();
        ui::get_panel_draw_list().add_text(text_top_left, ui::get_color(ui::ColorVar::Text), title);
    }
}

// toggle
namespace
{
    void draw_toggler(bool enabled, bool hovered, Vec2 pos, Vec2 size)
    {
        const float radius = size.y * 0.5f;
        const float rounding = size.y * 0.25f;
        const float slot_half_height = size.y * 0.5f;
        const bool draw_circular_grabber = false;

        const Color bg_color = hovered ?
            ui::get_color(enabled ? ui::ColorVar::FrameBgActive : ui::ColorVar::FrameBgHovered) :
            ui::get_color(enabled ? ui::ColorVar::CheckMark : ui::ColorVar::FrameBg);

        const Vec2 pmid{
            pos.x + radius + (enabled ? 1.0f : 0.0f) * (size.x - radius * 2),
            pos.y + size.y / 2.0f,
        };
        const Rect bg_rect = Rect::from_corners(
            {pos.x, pmid.y - slot_half_height},
            {pos.x + size.x, pmid.y + slot_half_height}
        );

        ui::DrawListView draw_list = ui::get_panel_draw_list();
        draw_list.add_rect_filled(bg_rect, bg_color, rounding);

        if (draw_circular_grabber) {
            draw_list.add_circle_filled({pmid, radius * 0.8f}, ui::get_color(ui::ColorVar::SliderGrab));
        }
        else {
            const Vec2 offs = {radius*0.8f, radius*0.8f};
            draw_list.add_rect_filled(Rect::from_origin_and_dimensions(pmid, 2.0f*offs), ui::get_color(ui::ColorVar::SliderGrab), rounding);
        }
    }

    bool draw_toggle(CStringView label, bool* v)
    {
        ui::push_style_color(ui::ColorVar::Button, Color::clear());

        const float title_height = ui::get_text_line_height_in_current_panel();

        const Vec2 p = ui::get_cursor_ui_pos();
        const Vec2 bb(ui::get_column_width(), ui::get_frame_height());
        ui::push_style_var(ui::StyleVar::ButtonTextAlign, {0.0f, 0.0f});
        ui::push_id(label);
        const bool status = ui::draw_button("###toggle_button", bb);

        if (status) {
            *v = !*v;
        }

        ui::pop_id();
        ui::pop_style_var();
        const Vec2 button_top_left = ui::get_item_top_left_ui_pos();
        const Vec2 button_bottom_right = ui::get_item_bottom_right_ui_pos();

        draw_widget_title(label, p);

        const float toggle_height = title_height * 0.9f;
        const Vec2 frame_padding = ui::get_style_frame_padding();
        const Vec2 toggle_size = {toggle_height * 1.75f, toggle_height};
        const Vec2 toggle_pos{
            button_bottom_right.x - toggle_size.x - frame_padding.x,
            button_top_left.y + (title_height - toggle_size.y)/2.0f + frame_padding.y,
        };
        draw_toggler(*v, ui::is_item_hovered(), toggle_pos, toggle_size);

        ui::pop_style_color();

        return status;
    }
}

class osc::CustomWidgetsTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "oscar_demos/CustomWidgets"; }

    explicit Impl(CustomWidgetsTab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {}

    void on_draw()
    {
        ui::begin_panel("window");
        ui::draw_float_input("standardinput", &float_value_);
        ui::draw_float_circular_slider("custom slider", &float_value_, 15.0f, 5.0f);
        ui::draw_text("%f", float_value_);
        draw_toggle("custom toggle", &toggle_state_);
        ui::end_panel();
    }

private:
    float float_value_ = 10.0f;
    bool toggle_state_ = false;
};


CStringView osc::CustomWidgetsTab::id() { return Impl::static_label(); }

osc::CustomWidgetsTab::CustomWidgetsTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::CustomWidgetsTab::impl_on_draw() { private_data().on_draw(); }
