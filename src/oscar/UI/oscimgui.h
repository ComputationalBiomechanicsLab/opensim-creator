#pragma once

#define IMGUI_USER_CONFIG <oscar/UI/oscimgui_config.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <oscar/Graphics/Color.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/ClosedInterval.h>
#include <oscar/Maths/EulerAngles.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Vec.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Shims/Cpp23/utility.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <utility>

namespace osc { class Camera; }
namespace osc { struct PolarPerspectiveCamera; }
namespace osc { class RenderTexture; }
namespace osc { class Texture2D; }

namespace osc::ui
{
    // vertically align upcoming text baseline to FramePadding.y so that it will align properly to regularly framed items (call if you have text on a line before a framed item)
    void align_text_to_frame_padding();

    void draw_text(CStringView);
    void draw_text_v(CStringView fmt, va_list);
    inline void draw_text(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        draw_text_v(fmt, args);
        va_end(args);
    }

    void draw_text_disabled(CStringView);
    void draw_text_disabled_v(CStringView fmt, va_list);
    inline void draw_text_disabled(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        draw_text_disabled_v(fmt, args);
        va_end(args);
    }

    void draw_text_wrapped(CStringView);
    void draw_text_wrapped_v(CStringView fmt, va_list);
    inline void draw_text_wrapped(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        draw_text_wrapped_v(fmt, args);
        va_end(args);
    }

    void draw_text_unformatted(CStringView);

    void draw_bullet_point();

    void draw_text_bullet_pointed(CStringView);

    bool draw_tree_node_ex(CStringView, ImGuiTreeNodeFlags = 0);

    float get_tree_node_to_label_spacing();

    void tree_pop();

    void draw_progress_bar(float fraction);

    bool begin_menu(CStringView sv, bool enabled = true);

    void end_menu();

    bool draw_menu_item(
        CStringView label,
        CStringView shortcut = {},
        bool selected = false,
        bool enabled = true
    );

    bool draw_menu_item(
        CStringView label,
        CStringView shortcut,
        bool* p_selected,
        bool enabled = true
    );

    bool begin_tab_bar(CStringView str_id);
    void end_tab_bar();

    bool begin_tab_item(CStringView label, bool* p_open = nullptr, ImGuiTabItemFlags flags = 0);

    void end_tab_item();

    bool draw_tab_item_button(CStringView label);

    void set_num_columns(int count = 1, const char* id = nullptr, bool border = true);
    float get_column_width(int column_index = -1);
    void next_column();

    void same_line(float offset_from_start_x = 0.0f, float spacing = -1.0f);

    bool is_mouse_clicked(ImGuiMouseButton button, bool repeat = false);

    bool is_mouse_clicked(ImGuiMouseButton button, ImGuiID owner_id, ImGuiInputFlags flags = 0);

    bool is_mouse_released(ImGuiMouseButton button);

    bool is_mouse_down(ImGuiMouseButton button);

    bool is_mouse_dragging(ImGuiMouseButton button, float lock_threshold = -1.0f);

    bool draw_selectable(CStringView label, bool* p_selected, ImGuiSelectableFlags flags = 0, const Vec2& size = {});

    bool draw_selectable(CStringView label, bool selected = false, ImGuiSelectableFlags flags = 0, const Vec2& size = {});

    bool draw_checkbox(CStringView label, bool* v);

    bool draw_checkbox_flags(CStringView label, int* flags, int flags_value);

    bool draw_checkbox_flags(CStringView label, unsigned int* flags, unsigned int flags_value);

    bool draw_float_slider(CStringView label, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);

    bool draw_scalar_input(CStringView label, ImGuiDataType data_type, void* p_data, const void* p_step = NULL, const void* p_step_fast = NULL, const char* format = NULL, ImGuiInputTextFlags flags = 0);

    bool draw_int_input(CStringView label, int* v, int step = 1, int step_fast = 100, ImGuiInputTextFlags flags = 0);

    bool draw_double_input(CStringView label, double* v, double step = 0.0, double step_fast = 0.0, const char* format = "%.6f", ImGuiInputTextFlags flags = 0);

    bool draw_float_input(CStringView label, float* v, float step = 0.0f, float step_fast = 0.0f, const char* format = "%.3f", ImGuiInputTextFlags flags = 0);

    bool draw_float3_input(CStringView label, float v[3], const char* format = "%.3f", ImGuiInputTextFlags flags = 0);

    bool draw_vec3_input(CStringView label, Vec3& v, const char* format = "%.3f", ImGuiInputTextFlags flags = 0);

    bool draw_rgb_color_editor(CStringView label, Color& color);

    bool draw_rgba_color_editor(CStringView label, Color& color);

    bool draw_button(CStringView label, const Vec2& size = {});

    bool draw_small_button(CStringView label);

    bool draw_invisible_button(CStringView label, Vec2 size = {});

    bool draw_radio_button(CStringView label, bool active);

    bool draw_collapsing_header(CStringView label, ImGuiTreeNodeFlags flags = 0);

    void draw_dummy(const Vec2& size);

    bool begin_combobox(CStringView label, CStringView preview_value, ImGuiComboFlags flags = 0);
    void end_combobox();

    bool draw_combobox(CStringView label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1);

    bool begin_listbox(CStringView label);
    void end_listbox();

    ImGuiViewport* get_main_viewport();

    ImGuiID enable_dockspace_over_viewport(const ImGuiViewport* viewport = NULL, ImGuiDockNodeFlags flags = 0, const ImGuiWindowClass* window_class = NULL);

    bool begin_panel(CStringView name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0);
    void end_panel();

    bool begin_child_panel(CStringView str_id, const Vec2& size = {}, ImGuiChildFlags child_flags = 0, ImGuiWindowFlags panel_flags = 0);
    void end_child_panel();

    void close_current_popup();

    void set_tooltip_v(const char* fmt, va_list);
    inline void set_tooltip(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        set_tooltip_v(fmt, args);
        va_end(args);
    }

    void set_scroll_y_here();

    float get_frame_height();

    Vec2 get_content_region_available();

    Vec2 get_cursor_start_pos();

    Vec2 get_cursor_pos();

    float get_cursor_pos_x();
    void set_cursor_pos(Vec2);
    void set_cursor_pos_x(float local_x);

    Vec2 get_cursor_screen_pos();
    void set_cursor_screen_pos(Vec2);

    void set_next_panel_pos(Vec2, ImGuiCond = 0, Vec2 pivot = {});

    void set_next_panel_size(Vec2 size, ImGuiCond cond = 0);

    void set_next_panel_size_constraints(Vec2 size_min, Vec2 size_max);

    void set_next_panel_bg_alpha(float alpha);

    bool is_panel_hovered(ImGuiHoveredFlags flags = 0);

    void begin_disabled(bool disabled = true);
    void end_disabled();

    bool begin_tooltip_nowrap();
    void end_tooltip_nowrap();

    void push_id(UID);
    void push_id(ptrdiff_t);
    void push_id(size_t);
    void push_id(int);
    void push_id(const void*);
    void push_id(CStringView);
    void pop_id();

    ImGuiID get_id(CStringView str_id);

    ImGuiItemFlags get_item_flags();
    void set_next_item_size(Rect);  // note: ImGui API assumes cursor is located at `p1` already
    bool add_item(Rect bounds, ImGuiID);
    bool is_item_hoverable(Rect bounds, ImGuiID id, ImGuiItemFlags item_flags);

    void draw_separator();
    void draw_separator(ImGuiSeparatorFlags);

    void start_new_line();

    void indent(float indent_w = 0.0f);
    void unindent(float indent_w = 0.0f);

    void set_keyboard_focus_here();
    bool is_key_pressed(ImGuiKey, bool repeat = true);
    bool is_key_released(ImGuiKey);
    bool is_key_down(ImGuiKey);

    ImGuiStyle& get_style();
    Color get_style_color(ImGuiCol);
    Vec2 get_style_frame_padding();
    float get_style_frame_border_size();
    Vec2 get_style_panel_padding();
    Vec2 get_style_item_spacing();
    Vec2 get_style_item_inner_spacing();
    float get_style_alpha();

    ImGuiIO& get_io();

    void push_style_var(ImGuiStyleVar style, const Vec2& pos);
    void push_style_var(ImGuiStyleVar style, float pos);
    void pop_style_var(int count = 1);

    void open_popup(CStringView str_id, ImGuiPopupFlags popup_flags = 0);
    bool begin_popup(CStringView str_id, ImGuiWindowFlags flags = 0);
    bool begin_popup_context_menu(CStringView str_id = nullptr, ImGuiPopupFlags popup_flags = 1);
    bool begin_popup_modal(CStringView name, bool* p_open = NULL, ImGuiWindowFlags flags = 0);
    void end_popup();

    Vec2 get_mouse_pos();

    bool begin_menu_bar();
    void end_menu_bar();

    void set_mouse_cursor(ImGuiMouseCursor cursor_type);

    void set_next_item_width(float item_width);
    void set_next_item_open(bool is_open);
    void push_item_flag(ImGuiItemFlags option, bool enabled);
    void pop_item_flag();
    bool is_item_clicked(ImGuiMouseButton mouse_button = 0);
    bool is_item_hovered(ImGuiHoveredFlags flags = 0);
    bool is_item_deactivated_after_edit();

    Vec2 get_item_topleft();
    Vec2 get_item_bottomright();
    bool begin_table(CStringView str_id, int column, ImGuiTableFlags flags = 0, const Vec2& outer_size = {}, float inner_width = 0.0f);
    void table_setup_scroll_freeze(int cols, int rows);

    ImGuiTableSortSpecs* table_get_sort_specs();

    void table_headers_row();
    bool table_set_column_index(int column_n);
    void table_next_row(ImGuiTableRowFlags row_flags = 0, float min_row_height = 0.0f);
    void table_setup_column(CStringView label, ImGuiTableColumnFlags flags = 0, float init_width_or_weight = 0.0f, ImGuiID user_id = 0);
    void end_table();

    void push_style_color(ImGuiCol index, ImU32 col);
    void push_style_color(ImGuiCol index, const Vec4& col);
    void push_style_color(ImGuiCol index, const Color& c);
    void pop_style_color(int count = 1);

    ImU32 get_color_ImU32(ImGuiCol index);
    ImU32 to_ImU32(const Vec4& color);
    float get_text_line_height();
    float get_text_line_height_with_spacing();

    float get_font_size();

    Vec2 calc_text_size(CStringView text, bool hide_text_after_double_hash = false);

    Vec2 get_panel_size();

    ImDrawList* get_panel_draw_list();
    ImDrawList* get_foreground_draw_list();
    ImDrawListSharedData* get_draw_list_shared_data();

    void show_demo_panel();

    // applies "dark" theme to current UI context
    void apply_dark_theme();

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
        EulerAngles&
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

    // draws a text input that manipulates a `std::string`
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
    // the returned `Rect` is given in UI-compatible UI-space, such that:
    //
    // - it's measured in pixels
    // - starts in the top-left corner
    // - ends in the bottom-right corner
    Rect get_main_viewport_workspace_uiscreenspace_rect();

    // returns a `Rect` that indicates where the current workspace area is in the main viewport
    //
    // the returned `Rect` is given in osc-graphics-API-compatible screen-space, rather than UI
    // space, such that:
    //
    // - it's measured in pixels
    // - starts in the bottom-left corner
    // - ends in the top-right corner
    Rect get_main_viewport_workspace_screenspace_rect();

    // returns the dimensions of the current workspace area in pixels in the main viewport
    Vec2 get_main_viewport_workspace_screen_dimensions();

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

    // updates a polar comera's rotation, position, etc. from UI mouse input state
    bool update_polar_camera_from_mouse_inputs(
        PolarPerspectiveCamera&,
        Vec2 viewport_dimensions
    );

    // an operation that a ui `Gizmo` shall perform
    enum class GizmoOperation {
        Translate,
        Rotate,
        Scale,
        NUM_OPTIONS,
    };

    // the mode (coordinate space) that a `Gizmo` presents its manipulations in
    enum class GizmoMode {
        Local,
        World,
        NUM_OPTIONS,
    };

    // a UI gizmo that manipulates the given model matrix using user-interactable drag handles, arrows, etc.
    class Gizmo final {
    public:

        // if the user manipulated the gizmo, updates `model_matrix` to match the
        // post-manipulation transform and returns a world-space transform that
        // represents the "difference" added by the user's manipulation. I.e.:
        //
        //     transform_returned * model_matrix_before = model_matrix_after
        //
        // note: a user-enacted rotation/scale may not happen at the origin, so if
        //       you're thinking "oh I'm only handling rotation/scaling, so I'll
        //       ignore the translational part of the transform" then you're in
        //       for a nasty surprise: T(origin)*R*S*T(-origin)
        std::optional<Transform> draw(
            Mat4& model_matrix,  // edited in-place
            const Mat4& view_matrix,
            const Mat4& projection_matrix,
            const Rect& screenspace_rect
        );
        bool is_using() const;
        bool was_using() const { return was_using_last_frame_; }
        bool is_over() const;
        GizmoOperation operation() const { return operation_; }
        void set_operation(GizmoOperation op) { operation_ = op; }
        GizmoMode mode() const { return mode_; }
        void set_mode(GizmoMode mode) { mode_ = mode; }

        // updates the gizmo based on keyboard inputs (e.g. pressing `G` enables grab mode)
        bool handle_keyboard_inputs();
    private:
        UID id_;
        GizmoOperation operation_ = GizmoOperation::Translate;
        GizmoMode mode_ = GizmoMode::World;
        bool was_using_last_frame_ = false;
    };

    bool draw_gizmo_mode_selector(
        Gizmo&
    );

    bool draw_gizmo_mode_selector(
        GizmoMode&
    );

    bool draw_gizmo_op_selector(
        Gizmo&,
        bool can_translate = true,
        bool can_rotate = true,
        bool can_scale = true
    );

    bool draw_gizmo_op_selector(
        GizmoOperation&,
        bool can_translate = true,
        bool can_rotate = true,
        bool can_scale = true
    );

    // oscar bindings for `ImPlot`
    //
    // the design/types used here differ from `ImPlot` in order to provide
    // consistency with the rest of the `osc` ecosystem
    namespace plot
    {
        enum class PlotFlags {
            None        = 0,
            NoTitle     = 1<<0,
            NoLegend    = 1<<1,
            NoMenus     = 1<<4,
            NoBoxSelect = 1<<5,
            NoFrame     = 1<<6,
            NoInputs    = 1<<3,

            Default     = None,
        };
        constexpr PlotFlags operator|(PlotFlags lhs, PlotFlags rhs)
        {
            return static_cast<PlotFlags>(cpp23::to_underlying(lhs) | cpp23::to_underlying(rhs));
        }
        constexpr PlotFlags operator^(PlotFlags lhs, PlotFlags rhs)
        {
            return static_cast<PlotFlags>(cpp23::to_underlying(lhs) ^ cpp23::to_underlying(rhs));
        }
        constexpr bool operator&(PlotFlags lhs, PlotFlags rhs)
        {
            return (cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs)) != 0;
        }

        enum class StyleVar {
            // `Vec2`: additional fit padding as a percentage of the fit extents (e.g. Vec2{0.1, 0.2} would add 10 % to the X axis and 20 % to the Y axis)
            FitPadding,

            // `Vec2`: padding between widget frame and plot area, labels, or outside legends (i.e. main padding)
            PlotPadding,

            // `float`: thickness of the border around plot area
            PlotBorderSize,

            // `Vec2`: text padding around annotation labels
            AnnotationPadding,

            NUM_OPTIONS
        };

        enum class ColorVar {
            // plot line/outline color (defaults to the next unused color in the current colormap)
            Line,

            // plot area background color (defaults to `ImGuiCol_WindowBg`)
            PlotBackground,

            NUM_OPTIONS,
        };

        enum class Axis {
            X1,  // enabled by default
            Y1,  // enabled by default

            NUM_OPTIONS,
        };

        enum class AxisFlags {
            None         = 0,
            NoLabel      = 1<<0,
            NoGridLines  = 1<<1,
            NoTickMarks  = 1<<2,
            NoTickLabels = 1<<3,
            NoMenus      = 1<<5,
            AutoFit      = 1<<11,
            LockMin      = 1<<14,
            LockMax      = 1<<15,

            Default = None,
            Lock = LockMin | LockMax,
            NoDecorations = NoLabel | NoGridLines | NoTickMarks | NoTickLabels,
        };

        constexpr AxisFlags operator|(AxisFlags lhs, AxisFlags rhs)
        {
            return static_cast<AxisFlags>(cpp23::to_underlying(lhs) | cpp23::to_underlying(rhs));
        }

        enum class Condition {
            Always,  // unconditional, i.e. the function should always do what is being asked
            Once,    // only perform the action once per runtime session (only the first call will succeed)
            NUM_OPTIONS,
        };

        enum class MarkerType {
            None,  // i.e. no marker
            Circle,
            NUM_OPTIONS,
        };

        enum class DragToolFlags {
            None     = 0,
            NoInputs = 1<<2,
            NoFit    = 1<<1,

            Default = None,
        };

        constexpr DragToolFlags operator|(DragToolFlags lhs, DragToolFlags rhs)
        {
            return static_cast<DragToolFlags>(cpp23::to_underlying(lhs) | cpp23::to_underlying(rhs));
        }

        enum class Location {
            Center,
            North,
            NorthEast,
            East,
            SouthEast,
            South,
            SouthWest,
            West,
            NorthWest,
            NUM_OPTIONS,

            Default = Center,
        };

        enum class LegendFlags {
            None    = 0,
            Outside = 1<<4,

            Default = None,
        };

        constexpr bool operator&(LegendFlags lhs, LegendFlags rhs)
        {
            return (cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs)) != 0;
        }

        constexpr LegendFlags operator^(LegendFlags lhs, LegendFlags rhs)
        {
            return static_cast<LegendFlags>(cpp23::to_underlying(lhs) ^ cpp23::to_underlying(rhs));
        }

        // draws the plotting demo in its own panel
        void show_demo_panel();

        // starts a 2D plotting context
        //
        // - if this function returns `true`, then `plot::end` MUST be called
        // - `title` must be unique to the current ID scope/stack
        // - `size` is the total size of the plot including axis labels, title, etc.
        //   you can call `get_plot_screen_rect` after finishing setup to figure out
        //   the bounds of the plot area (i.e. where the data is)
        bool begin(CStringView title, Vec2 size, PlotFlags = PlotFlags::Default);

        // ends a 2D plotting context
        //
        // this must be called if `plot::begin` returns `true`
        void end();

        // temporarily modifies a style variable of `float` type. Must be paired with `pop_style_var`
        void push_style_var(StyleVar, float);

        // temporarily modifies a style variable of `Vec2` type. Must be paired with `pop_style_var`
        void push_style_var(StyleVar, Vec2);

        // undoes `count` temporary style variable modifications that were enacted by `push_style_var`
        void pop_style_var(int count = 1);

        // temporarily modifies a style color variable. Must be paired with `pop_style_color`
        void push_style_color(ColorVar, const Color&);

        // undoes `count` temporary style color modifications that were enacted by `push_style_color`
        void pop_style_color(int count = 1);

        // enables an axis or sets the label and/or the flags of an existing axis
        void setup_axis(Axis, std::optional<CStringView> label = std::nullopt, AxisFlags = AxisFlags::Default);

        // sets the label and/or the flags for the primary X and Y axes (shorthand for two calls to `setup_axis`)
        void setup_axes(CStringView x_label, CStringView y_label, AxisFlags x_flags = AxisFlags::Default, AxisFlags y_flags = AxisFlags::Default);

        // sets an axis's range limits. If `Condition::Always` is used, the axes limits will be locked
        void setup_axis_limits(Axis axis, ClosedInterval<float> data_range, float padding_percentage, Condition = Condition::Once);

        // explicitly finalizes plot setup
        //
        // - once this is called, you cannot call any `plot::setup_*` functions for
        //   in the current 2D plotting context
        // - calling this function is OPTIONAL - the implementation will automatically
        //   call it on the first setup-locking API call (e.g. `plot::draw_line`)
        void setup_finish();

        // sets the marker style for the next item only
        void set_next_marker_style(
            MarkerType = MarkerType::None,
            std::optional<float> size = std::nullopt,
            std::optional<Color> fill = std::nullopt,
            std::optional<float> weight = std::nullopt,
            std::optional<Color> outline = std::nullopt
        );

        // plots a standard 2D line plot
        void plot_line(CStringView name, std::span<const Vec2> points);
        void plot_line(CStringView name, std::span<const float> points);

        // returns the plot's rectangle in screen-space
        //
        // must be called between `plot::begin` and `plot::end`
        Rect get_plot_screen_rect();

        // draws an annotation callout at a chosen point
        //
        // - clamping keeps annotations in the plot area
        // - annotations are always rendered on top of the plot area
        void draw_annotation_v(Vec2 location_dataspace, const Color& color, Vec2 pixel_offset, bool clamp, const char* fmt, va_list args);
        inline void draw_annotation(Vec2 location_dataspace, const Color& color, Vec2 pixel_offset, bool clamp, const char* fmt, ...)
        {
            va_list args;
            va_start(args, fmt);
            draw_annotation_v(location_dataspace, color, pixel_offset, clamp, fmt, args);
            va_end(args);
        }

        // draws a draggable point at `location` in the plot area
        //
        // - returns `true` if the user has interacted with the point. In this
        //   case, `location` will be updated with the new location
        bool drag_point(
            int id,
            Vec2d* location,
            const Color&,
            float size = 4,
            DragToolFlags = DragToolFlags::Default
        );

        // draws a draggable vertical guide line at an x-value in the plot area
        bool drag_line_x(
            int id,
            double* x,
            const Color&,
            float thickness = 1,
            DragToolFlags = DragToolFlags::Default
        );

        // draws a draggable horizontal guide line at a y-value in the plot area
        bool drag_line_y(
            int id,
            double* y,
            const Color&,
            float thickness = 1,
            DragToolFlags = DragToolFlags::Default
        );

        // draws a tag on the x axis at the specified x value
        void tag_x(double x, const Color&, bool round = false);

        // returns `true` if the plot area in the current plot is hovered
        bool is_plot_hovered();

        // returns the mouse position in the coordinate system of the current axes
        Vec2 get_plot_mouse_pos();

        // returns the mouse position in the coordinate system of the given axes
        Vec2 get_plot_mouse_pos(Axis x_axis, Axis y_axis);

        // sets up the plot legend
        void setup_legend(Location, LegendFlags = LegendFlags::Default);

        // begins a popup for a legend entry
        bool begin_legend_popup(CStringView label_id, ImGuiMouseButton mouse_button = 1);

        // ends a popup for a legend entry
        void end_legend_popup();
    }
}
