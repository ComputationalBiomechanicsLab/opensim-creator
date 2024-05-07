#pragma once

#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Eulers.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>

namespace osc { class Camera; }
namespace osc { struct Color; }
namespace osc { struct PolarPerspectiveCamera; }
namespace osc { class RenderTexture; }
namespace osc { class Texture2D; }
namespace osc { class UID; }

namespace osc::ui
{
    // applies "dark" theme to current UI context
    void apply_dark_theme();

    // updates a polar comera's rotation, position, etc. from UI mouse input state
    bool update_polar_camera_from_mouse_inputs(
        PolarPerspectiveCamera&,
        Vec2 viewport_dimensions
    );

    // updates a polar comera's rotation, position, etc. from UI keyboard input state
    bool update_polar_camera_from_keyboard_inputs(
        PolarPerspectiveCamera&,
        const Rect& viewport_rect,
        std::optional<AABB> maybe_scene_aabb
    );

    // updates a polar comera's rotation, position, etc. from UI input state (all)
    bool update_polar_camera_from_all_inputs(
        PolarPerspectiveCamera&,
        const Rect& viewport_rect,
        std::optional<AABB> maybe_scene_aabb
    );

    void update_camera_from_all_inputs(
        Camera&,
        Eulers&
    );

    // returns the UI content region available in screenspace as a `Rect`
    Rect content_region_avail_as_screen_rect();

    // draws a texture within the 2D UI
    //
    // assumes the texture coordinates are [(0.0, 1.0), (1.0, 0.0)]
    void draw_image(
        const Texture2D&
    );
    void draw_image(
        const Texture2D&,
        Vec2 dimensions
    );
    void draw_image(
        const Texture2D&,
        Vec2 dimensions,
        Vec2 top_left_texture_coordinate,
        Vec2 bottom_right_texture_coordinate
    );
    void draw_image(
        const RenderTexture&
    );
    void draw_image(
        const RenderTexture&,
        Vec2 dimensions
    );

    // returns the dimensions of a button with the given content
    Vec2 calc_button_size(CStringView);
    float calc_button_width(CStringView);

    bool draw_button_nobg(
        CStringView,
        Vec2 dimensions = {0.0f, 0.0f}
    );

    // draws a texture within the UI as a clickable button
    bool draw_image_button(
        CStringView,
        const Texture2D&,
        Vec2 dimensions,
        const Rect& texture_coordinates
    );
    bool draw_image_button(
        CStringView,
        const Texture2D&,
        Vec2 dimensions
    );

    // returns the screenspace bounding rectangle of the last-drawn item
    Rect get_last_drawn_item_screen_rect();

    // hittest the last-drawn item in the UI
    struct HittestResult final {
        Rect item_screen_rect = {};
        bool is_hovered = false;
        bool is_left_click_released_without_dragging = false;
        bool is_right_click_released_without_dragging = false;
    };
    HittestResult hittest_last_drawn_item();
    HittestResult hittest_last_drawn_item(float drag_threshold);

    // returns `true` if any scancode in the provided range is currently pressed down
    bool any_of_keys_down(std::span<const ImGuiKey>);
    bool any_of_keys_down(std::initializer_list<const ImGuiKey>);

    // returns `true` if any scancode in the provided range was pressed down this frame
    bool any_of_keys_pressed(std::span<const ImGuiKey>);
    bool any_of_keys_pressed(std::initializer_list<const ImGuiKey>);

    // returns true if the user is pressing either left- or right-Ctrl
    bool is_ctrl_down();

    // returns `true` if the user is pressing either:
    //
    // - left Ctrl
    // - right Ctrl
    // - left Super (mac)
    // - right Super (mac)
    bool is_ctrl_or_super_down();

    // returns `true` if the user is pressing either left- or right-shift
    bool is_shift_down();

    // returns `true` if the user is pressing either left- or right-alt
    bool is_alt_down();

    // returns `true` if the specified mouse button was released without the user dragging
    bool is_mouse_released_without_dragging(ImGuiMouseButton);
    bool is_mouse_released_without_dragging(ImGuiMouseButton, float drag_threshold);

    // returns `true` if the user is dragging their mouse with any button pressed
    bool is_mouse_dragging_with_any_button_down();

    // (lower-level tooltip methods: prefer using higher-level 'draw_tooltip(txt)' methods)
    void begin_tooltip(std::optional<float> wrap_width = std::nullopt);
    void end_tooltip(std::optional<float> wrap_width = std::nullopt);

    void draw_tooltip_header_text(CStringView);
    void draw_tooltip_description_spacer();
    void draw_tooltip_description_text(CStringView);

    // draws an overlay tooltip (content only)
    void draw_tooltip_body_only(CStringView);

    // draws an overlay tooltip (content only) if the last item is hovered
    void draw_tooltip_body_only_if_item_hovered(
        CStringView,
        ImGuiHoveredFlags flags = ImGuiHoveredFlags_ForTooltip
    );

    // draws an overlay tooltip with a header and description
    void draw_tooltip(CStringView header, CStringView description = {});

    // equivalent to `if (ui::is_item_hovered(flags)) draw_tooltip(header, description);`
    void draw_tooltip_if_item_hovered(
        CStringView header,
        CStringView description = {},
        ImGuiHoveredFlags flags = ImGuiHoveredFlags_ForTooltip
    );

    // draws a help text marker `"(?)"` and display a tooltip when the user hovers over it
    void draw_help_marker(CStringView header, CStringView description);

    // draws a help text marker `"(?)"` and display a tooltip when the user hovers over it
    void draw_help_marker(CStringView);

    // draws a `ImGui::InputText` that manipulates a `std::string`
    bool draw_string_input(
        CStringView label,
        std::string& edited_string,
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_None
    );

    // behaves like `ui::draw_float_input`, but understood to manipulate the scene scale
    bool draw_float_meters_input(
        CStringView label,
        float& v,
        float step = 0.0f,
        float step_fast = 0.0f,
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_None
    );

    // behaves like `ui::draw_float3_input`, but understood to manipulate the scene scale
    bool draw_float3_meters_input(
        CStringView label,
        Vec3&,
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_None
    );

    // behaves like `ui::draw_float_slider`, but understood to manipulate the scene scale
    bool draw_float_meters_slider(
        CStringView label,
        float& v,
        float v_min,
        float v_max,
        ImGuiSliderFlags flags = ImGuiInputTextFlags_None
    );

    // behaves like `ui::draw_float_input`, but edits the given value as a mass (kg)
    bool draw_float_kilogram_input(
        CStringView label,
        float& v,
        float step = 0.0f,
        float step_fast = 0.0f,
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_None
    );

    // behaves like `ui::draw_float_input`, but edits the given angular value in degrees
    bool draw_angle_input(
        CStringView label,
        Radians& v
    );

    // behaves like `ui::draw_float3_input`, but edits the given angular value in degrees
    bool draw_angle3_input(
        CStringView label,
        Vec<3, Radians>&,
        CStringView format = "%.3f"
    );

    // behaves like `ui::draw_float_slider`, but edits the given angular value as degrees
    bool draw_angle_slider(
        CStringView label,
        Radians& v,
        Radians min,
        Radians max
    );

    // returns an `ImU32` converted from the given `Color`
    ImU32 to_ImU32(const Color&);

    // returns a `Color` converted from the given LDR 8-bit `ImU32` format
    Color to_color(ImU32);

    // returns a `Color` converted from the given LDR `ImVec4` color
    Color to_color(const ImVec4&);

    // returns an `ImVec4` converted from the given `Color`
    ImVec4 to_ImVec4(const Color&);

    // returns "minimal" panel flags (i.e. no title bar, can't move the panel - ideal for images etc.)
    ImGuiWindowFlags get_minimal_panel_flags();

    // returns a `Rect` that indicates where the current workspace area is in the main viewport
    //
    // handy if (e.g.) you want to know the rect of a tab area
    Rect get_main_viewport_workspace_screen_rect();

    // returns `true` if the user's mouse is within the current workspace area of the main viewport
    bool is_mouse_in_main_viewport_workspace();

    // begin a menu that's attached to the top of a viewport, end it with `ui::end_panel()`
    bool begin_main_viewport_top_bar(
        CStringView label,
        float height = ui::get_frame_height(),
        ImGuiWindowFlags panel_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar
    );

    // begin a menu that's attached to the bottom of a viewport, end it with `ui::end_panel()`
    bool begin_main_viewport_bottom_bar(CStringView);

    // behaves like `ui::draw_button`, but is centered on the current line
    bool draw_button_centered(CStringView);

    // behaves like `ui::draw_text`, but is centered on the current line
    void draw_text_centered(CStringView);

    // behaves like `ui::draw_text`, but is vertically and horizontally centered in the remaining content of the current panel
    void draw_text_panel_centered(CStringView);

    // behaves like `ui::draw_text`, but with a disabled style and centered on the current line
    void draw_text_disabled_and_centered(CStringView);

    // behaves like `ui::draw_text`, but with a disabled style and centered in the remaining content of the current panel
    void draw_text_disabled_and_panel_centered(CStringView);

    // behaves like `ui::draw_text`, but centered in the current table column
    void draw_text_column_centered(CStringView);

    // behaves like `ui::draw_text`, but with a faded/muted style
    void draw_text_faded(CStringView);

    // behaves like `ui::draw_text`, but with a warning style (e.g. orange)
    void draw_text_warning(CStringView);

    // returns `true` if the last drawn item (e.g. an input) should be saved based on heuristics
    //
    // - if the item was deactivated (e.g. due to focusing something else), it should be saved
    // - if there's an active edit and the user presses enter, it should be saved
    // - if there's an active edit and the user presses tab, it should be saved
    bool should_save_last_drawn_item_value();

    void pop_item_flags(int n = 1);

    bool draw_combobox(
        CStringView label,
        size_t* current,
        size_t size,
        const std::function<CStringView(size_t)>& accessor
    );

    bool draw_combobox(
        CStringView label,
        size_t* current,
        std::span<const CStringView> items
    );

    void draw_vertical_separator();
    void draw_same_line_with_vertical_separator();

    bool draw_float_circular_slider(
        CStringView label,
        float* v,
        float min,
        float max,
        CStringView format = "%.3f",
        ImGuiSliderFlags flags = 0
    );
}
