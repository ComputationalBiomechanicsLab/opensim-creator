#include "camera_view_axes.h"

#include <liboscar/graphics/camera.h>
#include <liboscar/graphics/color.h>
#include <liboscar/graphics/orbit_camera_controller.h>
#include <liboscar/graphics/polar_perspective_camera.h>
#include <liboscar/maths/circle.h>
#include <liboscar/maths/coordinate_direction.h>
#include <liboscar/maths/geometric_functions.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/rect.h>
#include <liboscar/maths/rect_functions.h>
#include <liboscar/maths/vector.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/utilities/c_string_view.h>

#include <algorithm>
#include <array>
#include <functional>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    struct AxesMetrics final {
        float font_size = ui::get_font_base_size();
        float line_length = 2.0f * font_size;
        float circle_radius = 0.6f * font_size;
        float max_edge_length = 2.0f * (line_length + sqrt(2.0f * circle_radius * circle_radius));
        Vector2 dimensions = {max_edge_length, max_edge_length};
    };
}

Vector2 osc::CameraViewAxes::dimensions() const
{
    return AxesMetrics{}.dimensions;
}

bool osc::CameraViewAxes::draw(PolarPerspectiveCamera& camera)
{
    // calculate widget metrics
    const auto metrics = AxesMetrics{};

    // calculate widget ui space metrics
    const Vector2 top_left = ui::get_cursor_ui_position();
    const Rect bounds = Rect::from_corners(top_left, top_left + metrics.dimensions);
    const Vector2 origin = bounds.origin();

    // figure out rendering order (back-to-front)
    const Matrix4x4 view_matrix = camera.view_matrix();
    auto axes = std::array{CoordinateDirection::x(), CoordinateDirection::y(), CoordinateDirection::z()};
    rgs::sort(axes, rgs::less{}, [&view_matrix](auto axis)
    {
        return transform_vector(view_matrix, axis.direction_vector()).z();
    });

    // draw each edge back-to-front
    bool edited = false;
    ui::DrawListView draw_list = ui::get_panel_draw_list();
    for (const auto& axis : axes) {
        // calc direction vector in ui space
        Vector2 view_space_pos = transform_vector(view_matrix, axis.direction_vector()).xy();
        view_space_pos.y() = -view_space_pos.y();  // y goes down in ui space

        Color base_color = {0.15f, 0.15f, 0.15f, 1.0f};
        base_color[axis.index()] = 0.7f;

        // draw line from origin to end with a labelled (clickable) circle ending
        {
            const Vector2 end = origin + metrics.line_length*view_space_pos;
            const Circle circ = {.origin = end, .radius = metrics.circle_radius};
            const Rect circle_bounds = bounding_rect_of(circ);

            const auto labels = std::to_array<CStringView>({ "X", "Y", "Z" });
            const auto id = ui::get_id(labels[axis.index()]);
            ui::set_cursor_ui_position(circle_bounds.ypd_top_left());
            ui::set_next_item_size(circle_bounds);
            if (ui::add_item(circle_bounds, id)) {
                const Vector2 label_size = ui::calc_text_size(labels[axis.index()]);

                const bool hovered = ui::is_item_hoverable(circle_bounds, id);
                const Color color = hovered ? Color::white() : base_color;
                const Color text_color = hovered ? Color::black() : Color::white();

                draw_list.add_line(origin, end, color, 3.0f);
                draw_list.add_circle_filled(circ, color);
                draw_list.add_text(end - 0.5f*label_size, text_color, labels[axis.index()]);

                if (hovered and ui::is_mouse_clicked(ui::MouseButton::Left, id)) {
                    camera.focus_along(axis);
                    edited = true;
                }
            }
        }

        // negative axes: draw a faded (clickable) circle ending - no line
        {
            const Vector2 end = origin - metrics.line_length*view_space_pos;
            const Circle circ = {.origin = end, .radius = metrics.circle_radius};
            const Rect circle_bounds = bounding_rect_of(circ);

            const auto labels = std::to_array<CStringView>({ "-X", "-Y", "-Z" });
            const auto id = ui::get_id(labels[axis.index()]);
            ui::set_cursor_ui_position(circle_bounds.ypd_top_left());
            ui::set_next_item_size(circle_bounds);
            if (ui::add_item(circle_bounds, id)) {
                const bool hovered = ui::is_item_hoverable(circle_bounds, id);
                const Color color = hovered ? Color::white() : base_color.with_alpha(0.3f);

                draw_list.add_circle_filled(circ, color);

                if (hovered and ui::is_mouse_clicked(ui::MouseButton::Left, id)) {
                    camera.focus_along(-axis);
                    edited = true;
                }
            }
        }
    }

    return edited;
}

bool osc::CameraViewAxes::draw(OrbitCameraController& camera_controller, const Camera& camera)
{
    PolarPerspectiveCamera tmp;
    tmp.theta = camera_controller.theta;
    tmp.phi = camera_controller.phi;
    tmp.radius = camera_controller.radius;
    tmp.focus_point = -camera_controller.focus_point;
    tmp.vertical_field_of_view = camera.vertical_field_of_view();
    tmp.znear = camera.near_clipping_plane();
    tmp.zfar = camera.far_clipping_plane();
    if (draw(tmp)) {
        camera_controller.theta = tmp.theta;
        camera_controller.phi = tmp.phi;
        camera_controller.radius = tmp.radius;
        camera_controller.focus_point = tmp.focus_point;
        return true;
    }
    return false;
}
