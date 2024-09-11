#include "CameraViewAxes.h"

#include <oscar/Graphics/Color.h>
#include <oscar/Maths/Circle.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/CStringView.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <ranges>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    struct AxesMetrics final {
        float font_size = ui::get_font_size();
        float line_length = 2.0f * font_size;
        float circle_radius = 0.6f * font_size;
        float max_edge_length = 2.0f * (line_length + std::sqrt(2.0f * circle_radius * circle_radius));
        Vec2 dimensions = {max_edge_length, max_edge_length};
    };
}

Vec2 osc::CameraViewAxes::dimensions() const
{
    return AxesMetrics{}.dimensions;
}

bool osc::CameraViewAxes::draw(PolarPerspectiveCamera& camera)
{
    // calculate widget metrics
    const auto metrics = AxesMetrics{};

    // calculate widget screen-space metrics
    const Vec2 topleft = ui::get_cursor_screen_pos();
    const Rect bounds = {topleft, topleft + metrics.dimensions};
    const Vec2 origin = centroid_of(bounds);

    // figure out rendering order (back-to-front)
    const Mat4 view_matrix = camera.view_matrix();
    auto axis_indices = std::to_array<Vec4::size_type>({0, 1, 2});
    rgs::sort(axis_indices, rgs::less{}, [&view_matrix](auto axis_index)
    {
        return (view_matrix * Vec4{}.with_element(axis_index, 1.0f)).z;
    });

    // draw each edge back-to-front
    bool edited = false;
    ImDrawList& drawlist = *ui::get_panel_draw_list();
    for (auto axis_index : axis_indices) {
        // calc direction vector in screen space
        Vec2 view = Vec2{view_matrix * Vec4{}.with_element(axis_index, 1.0f)};
        view.y = -view.y;  // y goes down in screen-space

        Color base_color = {0.15f, 0.15f, 0.15f, 1.0f};
        base_color[axis_index] = 0.7f;

        // draw line from origin to end with a labelled (clickable) circle ending
        {
            const Vec2 end = origin + metrics.line_length*view;
            const Circle circ = {.origin = end, .radius = metrics.circle_radius};
            const Rect circle_bounds = bounding_rect_of(circ);

            const auto labels = std::to_array<CStringView>({ "X", "Y", "Z" });
            const auto id = ui::get_id(labels[axis_index]);
            ui::set_cursor_screen_pos(circle_bounds.p1);
            ui::set_next_item_size(circle_bounds);
            if (ui::add_item(circle_bounds, id)) {
                const Vec2 label_size = ui::calc_text_size(labels[axis_index]);

                const bool hovered = ui::is_item_hoverable(circle_bounds, id);
                const ImU32 color = ui::to_ImU32(hovered ? Color::white() : base_color);
                const ImU32 text_color_u32 = ui::to_ImU32(hovered ? Color::black() : Color::white());

                drawlist.AddLine(origin, end, color, 3.0f);
                drawlist.AddCircleFilled(circ.origin, circ.radius, color);
                drawlist.AddText(end - 0.5f*label_size, text_color_u32, labels[axis_index].c_str());

                if (hovered and ui::is_mouse_clicked(ui::MouseButton::Left, id)) {
                    focus_along_axis(camera, axis_index);
                    edited = true;
                }
            }
        }

        // negative axes: draw a faded (clickable) circle ending - no line
        {
            const Vec2 end = origin - metrics.line_length*view;
            const Circle circ = {.origin = end, .radius = metrics.circle_radius};
            const Rect circle_bounds = bounding_rect_of(circ);

            const auto labels = std::to_array<CStringView>({ "-X", "-Y", "-Z" });
            const auto id = ui::get_id(labels[axis_index]);
            ui::set_cursor_screen_pos(circle_bounds.p1);
            ui::set_next_item_size(circle_bounds);
            if (ui::add_item(circle_bounds, id)) {
                const bool hovered = ui::is_item_hoverable(circle_bounds, id);
                const ImU32 color = ui::to_ImU32(hovered ? Color::white() : base_color.with_alpha(0.3f));

                drawlist.AddCircleFilled(circ.origin, circ.radius, color);

                if (hovered and ui::is_mouse_clicked(ui::MouseButton::Left, id)) {
                    focus_along_axis(camera, axis_index, true);
                    edited = true;
                }
            }
        }
    }

    return edited;
}
