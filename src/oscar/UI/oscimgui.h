#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Circle.h>
#include <oscar/Maths/ClosedInterval.h>
#include <oscar/Maths/EulerAngles.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/Vec.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Shims/Cpp23/utility.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/Flags.h>
#include <oscar/Utils/UID.h>

#include <concepts>
#include <cstdarg>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace osc { class Camera; }
namespace osc { struct PolarPerspectiveCamera; }
namespace osc { class RenderTexture; }
namespace osc { class Texture2D; }

struct ImDrawList;

namespace osc::ui
{
    // vertically align upcoming text baseline to FramePadding.y so that it will align properly to regularly framed items (call if you have text on a line before a framed item)
    void align_text_to_frame_padding();

    namespace detail
    {
        void draw_text_v(CStringView fmt, va_list);
        inline void draw_text(CStringView fmt, ...)
        {
            va_list args;
            va_start(args, fmt);
            draw_text_v(fmt, args);
            va_end(args);
        }
    }
    void draw_text(CStringView);
    template<typename... Args>
    void draw_text(CStringView fmt, Args&&... args)
    {
        detail::draw_text(fmt, std::forward<Args>(args)...);
    }

    namespace detail
    {
        void draw_text_disabled_v(CStringView fmt, va_list);
        inline void draw_text_disabled(CStringView fmt, ...)
        {
            va_list args;
            va_start(args, fmt);
            draw_text_disabled_v(fmt, args);
            va_end(args);
        }
    }
    void draw_text_disabled(CStringView);
    template<typename... Args>
    void draw_text_disabled(CStringView fmt, Args&&... args)
    {
        detail::draw_text_disabled(fmt, std::forward<Args>(args)...);
    }

    namespace detail
    {
        void draw_text_wrapped_v(CStringView fmt, va_list);
        inline void draw_text_wrapped(CStringView fmt, ...)
        {
            va_list args;
            va_start(args, fmt);
            draw_text_wrapped_v(fmt, args);
            va_end(args);
        }
    }
    void draw_text_wrapped(CStringView);
    template<typename... Args>
    void draw_text_wrapped(CStringView fmt, Args&&... args)
    {
        detail::draw_text_wrapped(fmt, std::forward<Args>(args)...);
    }

    void draw_text_unformatted(std::string_view);

    void draw_bullet_point();

    void draw_text_bullet_pointed(CStringView);

    bool draw_text_link(CStringView);

    enum class TreeNodeFlag : unsigned {
        None        = 0,
        DefaultOpen = 1<<0,
        OpenOnArrow = 1<<1,  // only open when clicking on the arrow part
        Leaf        = 1<<2,  // no collapsing, no arrow (use as a convenience for leaf nodes)
        Bullet      = 1<<3,  // display a bullet instead of arrow. IMPORTANT: node can still be marked open/close if you don't also set the `Leaf` flag!
        NUM_FLAGS   = 4,
    };
    using TreeNodeFlags = Flags<TreeNodeFlag>;

    bool draw_tree_node_ex(CStringView, TreeNodeFlags = {});

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

    enum class TabItemFlag : unsigned {
        None            = 0,
        NoReorder       = 1<<0,  // disable reordering this tab or having another tab cross over this tab
        NoCloseButton   = 1<<1,  // track whether `p_open` was set or not (we'll need this info on the next frame to recompute ContentWidth during layout)
        UnsavedDocument = 1<<2,  // display a dot next to the title + (internally) set `ImGuiTabItemFlags_NoAssumedClosure`
        SetSelected     = 1<<3,  // trigger flag to programmatically make the tab selected when calling `begin_tab_item`
        NUM_FLAGS       =    4,
    };
    using TabItemFlags = Flags<TabItemFlag>;

    bool begin_tab_item(CStringView label, bool* p_open = nullptr, TabItemFlags = {});
    void end_tab_item();

    bool draw_tab_item_button(CStringView label);

    void set_num_columns(int count = 1, std::optional<CStringView> id = std::nullopt, bool border = true);
    float get_column_width(int column_index = -1);
    void next_column();

    void same_line(float offset_from_start_x = 0.0f, float spacing = -1.0f);

    enum class MouseButton {
        Left,
        Right,
        Middle,
        NUM_OPTIONS,
    };

    struct ID final {
    public:
        explicit ID() = default;
        explicit ID(unsigned int value) : value_{value} {}

        unsigned int value() const { return value_; }
    private:
        unsigned int value_ = 0;  // defaults to "no ID"
    };

    bool is_mouse_clicked(MouseButton, bool repeat = false);
    bool is_mouse_clicked(MouseButton, ID owner_id);
    bool is_mouse_released(MouseButton);
    bool is_mouse_down(MouseButton);
    bool is_mouse_dragging(MouseButton, float lock_threshold = -1.0f);

    enum class SliderFlag : unsigned {
        None        = 0,
        Logarithmic = 1<<0,
        AlwaysClamp = 1<<1,
        NoInput     = 1<<2,
        NUM_FLAGS   =    3,
    };
    using SliderFlags = Flags<SliderFlag>;

    enum class DataType {
        Float,
        NUM_OPTIONS
    };

    enum class TextInputFlag : unsigned {
        None             = 0,
        ReadOnly         = 1<<0,
        EnterReturnsTrue = 1<<1,
        NUM_FLAGS        = 2,
    };
    using TextInputFlags = Flags<TextInputFlag>;

    bool draw_selectable(CStringView label, bool* p_selected);
    bool draw_selectable(CStringView label, bool selected = false);
    bool draw_checkbox(CStringView label, bool* v);
    bool draw_float_slider(CStringView label, float* v, float v_min, float v_max, const char* format = "%.3f", SliderFlags = {});
    bool draw_scalar_input(CStringView label, DataType data_type, void* p_data, const void* p_step = nullptr, const void* p_step_fast = nullptr, const char* format = nullptr, TextInputFlags = {});
    bool draw_int_input(CStringView label, int* v, int step = 1, int step_fast = 100, TextInputFlags = {});
    bool draw_double_input(CStringView label, double* v, double step = 0.0, double step_fast = 0.0, const char* format = "%.6f", TextInputFlags = {});
    bool draw_float_input(CStringView label, float* v, float step = 0.0f, float step_fast = 0.0f, const char* format = "%.3f", TextInputFlags = {});
    bool draw_float3_input(CStringView label, float v[3], const char* format = "%.3f", TextInputFlags = {});
    bool draw_vec3_input(CStringView label, Vec3& v, const char* format = "%.3f", TextInputFlags = {});
    bool draw_rgb_color_editor(CStringView label, Color& color);
    bool draw_rgba_color_editor(CStringView label, Color& color);
    bool draw_button(CStringView label, const Vec2& size = {});
    bool draw_small_button(CStringView label);
    bool draw_invisible_button(CStringView label, Vec2 size = {});
    bool draw_radio_button(CStringView label, bool active);
    bool draw_collapsing_header(CStringView label, TreeNodeFlags = {});
    void draw_dummy(const Vec2& size);

    enum class ComboFlag : unsigned {
        None = 0,
        NoArrowButton = 1<<0,
        NUM_FLAGS = 1,
    };
    using ComboFlags = Flags<ComboFlag>;

    bool begin_combobox(CStringView label, CStringView preview_value, ComboFlags = {});
    void end_combobox();

    bool begin_listbox(CStringView label);
    void end_listbox();

    Vec2 get_main_viewport_center();
    void enable_dockspace_over_main_viewport();

    enum class WindowFlag : unsigned {
        None                    = 0,

        NoMove                  = 1<<0,
        NoTitleBar              = 1<<1,
        NoResize                = 1<<2,
        NoSavedSettings         = 1<<3,
        NoScrollbar             = 1<<4,
        NoInputs                = 1<<5,
        NoBackground            = 1<<6,
        NoCollapse              = 1<<7,
        NoDecoration            = 1<<8,
        NoDocking               = 1<<9,
        NoNav                   = 1<<10,

        MenuBar                 = 1<<11,
        AlwaysAutoResize        = 1<<12,
        HorizontalScrollbar     = 1<<13,
        AlwaysVerticalScrollbar = 1<<14,

        NUM_FLAGS               = 15,
    };
    using WindowFlags = Flags<WindowFlag>;

    bool begin_panel(CStringView name, bool* p_open = nullptr, WindowFlags = {});
    void end_panel();

    enum class ChildPanelFlag : unsigned {
        None   = 0,
        Border = 1<<0,
        NUM_FLAGS = 1,
    };
    using ChildPanelFlags = Flags<ChildPanelFlag>;

    bool begin_child_panel(CStringView str_id, const Vec2& size = {}, ChildPanelFlags child_flags = {}, WindowFlags panel_flags = {});
    void end_child_panel();

    void close_current_popup();

    namespace detail
    {
        void set_tooltip_v(CStringView fmt, va_list);
    }
    inline void set_tooltip(CStringView fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        detail::set_tooltip_v(fmt, args);
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

    enum class Conditional {
        Always,
        Once,
        Appearing,
        NUM_OPTIONS,
    };

    void set_next_panel_pos(Vec2, Conditional = Conditional::Always, Vec2 pivot = {});

    void set_next_panel_size(Vec2 size, Conditional = Conditional::Always);

    void set_next_panel_size_constraints(Vec2 size_min, Vec2 size_max);

    void set_next_panel_bg_alpha(float alpha);

    enum class HoveredFlag : unsigned {
        None                         = 0,
        AllowWhenDisabled            = 1<<0,
        AllowWhenBlockedByPopup      = 1<<1,
        AllowWhenBlockedByActiveItem = 1<<2,
        AllowWhenOverlapped          = 1<<3,
        DelayNormal                  = 1<<4,
        ForTooltip                   = 1<<5,
        RootAndChildWindows          = 1<<6,
        ChildWindows                 = 1<<7,
        NUM_FLAGS                    =    8,
    };
    using HoveredFlags = Flags<HoveredFlag>;

    bool is_panel_hovered(HoveredFlags = {});

    void begin_disabled(bool disabled = true);
    void end_disabled();

    bool begin_tooltip_nowrap();
    void end_tooltip_nowrap();

    void push_id(UID);
    void push_id(int);
    template<std::integral T> void push_id(T number) { push_id(static_cast<int>(number)); }
    void push_id(std::string_view);
    void push_id(const void*);

    void pop_id();

    ID get_id(std::string_view);

    enum class ItemFlag : unsigned {
        None      = 0,
        Disabled  = 1<<0,
        Inputable = 1<<1,
        NUM_FLAGS =    2,
    };
    using ItemFlags = Flags<ItemFlag>;

    void set_next_item_size(Rect);  // note: ImGui API assumes cursor is located at `p1` already
    bool add_item(Rect bounds, ID);
    bool is_item_hoverable(Rect bounds, ID);

    void draw_separator();

    void start_new_line();

    void indent(float indent_w = 0.0f);
    void unindent(float indent_w = 0.0f);

    enum class Key {
        Escape,
        Enter,
        Space,
        Delete,
        Tab,
        LeftCtrl,
        RightCtrl,
        Backspace,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
        _1,
        _2,
        _3,
        _4,
        _5,
        _6,
        _7,
        _8,
        _9,
        _0,
        UpArrow,
        DownArrow,
        LeftArrow,
        RightArrow,
        Minus,
        Equal,
        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,

        // legacy
        MouseLeft,
        MouseRight,

        NUM_OPTIONS,
    };

    void set_keyboard_focus_here();
    bool is_key_pressed(Key, bool repeat = true);
    bool is_key_released(Key);
    bool is_key_down(Key);

    enum class ColorVar {
        Text,
        Button,
        ButtonActive,
        ButtonHovered,
        FrameBg,
        PopupBg,
        FrameBgHovered,
        FrameBgActive,
        CheckMark,
        SliderGrab,
        NUM_OPTIONS,
    };

    Color get_style_color(ColorVar);
    Vec2 get_style_frame_padding();
    float get_style_frame_border_size();
    Vec2 get_style_panel_padding();
    Vec2 get_style_item_spacing();
    Vec2 get_style_item_inner_spacing();
    float get_style_alpha();

    float get_framerate();
    bool wants_keyboard();

    enum class StyleVar {
        Alpha,
        ButtonTextAlign,
        CellPadding,
        DisabledAlpha,
        FramePadding,
        FrameRounding,
        ItemInnerSpacing,
        ItemSpacing,
        TabRounding,
        WindowPadding,
        NUM_OPTIONS,
    };

    void push_style_var(StyleVar, const Vec2& pos);
    void push_style_var(StyleVar, float pos);
    void pop_style_var(int count = 1);

    enum class PopupFlag : unsigned {
        None             = 0,
        MouseButtonLeft  = 1<<0,
        MouseButtonRight = 1<<1,
        NUM_FLAGS        =    2,
    };
    using PopupFlags = Flags<PopupFlag>;

    void open_popup(CStringView str_id, PopupFlags = {});
    bool begin_popup(CStringView str_id, WindowFlags = {});
    bool begin_popup_context_menu(CStringView str_id = {}, PopupFlags = PopupFlag::MouseButtonRight);
    bool begin_popup_modal(CStringView name, bool* p_open = nullptr, WindowFlags = {});
    void end_popup();

    Vec2 get_mouse_pos();
    float get_mouse_wheel_amount();

    bool begin_menu_bar();
    void end_menu_bar();

    void hide_mouse_cursor();
    void show_mouse_cursor();

    void set_next_item_width(float item_width);
    void set_next_item_open(bool is_open);
    void push_item_flag(ItemFlags flags, bool enabled);
    void pop_item_flag();
    bool is_item_clicked(MouseButton = MouseButton::Left);
    bool is_item_hovered(HoveredFlags = {});
    bool is_item_deactivated_after_edit();

    enum class TableFlag : unsigned {
        None              = 0,
        BordersInner      = 1<<0,
        BordersInnerV     = 1<<1,
        NoSavedSettings   = 1<<2,
        PadOuterX         = 1<<3,
        Resizable         = 1<<4,
        ScrollY           = 1<<5,
        SizingStretchProp = 1<<6,
        SizingStretchSame = 1<<7,
        Sortable          = 1<<8,
        SortTristate      = 1<<9,
        NUM_FLAGS         =    10,
    };
    using TableFlags = Flags<TableFlag>;

    Vec2 get_item_topleft();
    Vec2 get_item_bottomright();
    bool begin_table(CStringView str_id, int column, TableFlags = {}, const Vec2& outer_size = {}, float inner_width = 0.0f);
    void table_setup_scroll_freeze(int cols, int rows);

    enum class SortDirection {
        None,
        Ascending,
        Descending,
        NUM_OPTIONS,
    };

    struct TableColumnSortSpec final {
        ID column_id{};
        size_t column_index = 0;
        size_t sort_order = 0;
        SortDirection sort_direction = SortDirection::None;
    };

    bool table_column_sort_specs_are_dirty();
    std::vector<TableColumnSortSpec> get_table_column_sort_specs();

    enum class ColumnFlag : unsigned {
        None         = 0,
        NoSort       = 1<<0,
        WidthStretch = 1<<1,
        NUM_FLAGS    =    2,
    };
    using ColumnFlags = Flags<ColumnFlag>;

    void table_headers_row();
    bool table_set_column_index(int column_n);
    void table_next_row();
    void table_setup_column(CStringView label, ColumnFlags = {}, float init_width_or_weight = 0.0f, ID = ID{});
    void end_table();

    void push_style_color(ColorVar, const Color&);
    void pop_style_color(int count = 1);

    Color get_color(ColorVar);
    float get_text_line_height();
    float get_text_line_height_with_spacing();

    float get_font_size();

    Vec2 calc_text_size(CStringView text, bool hide_text_after_double_hash = false);

    Vec2 get_panel_size();

    class DrawListView {
    public:
        explicit DrawListView(ImDrawList* inner_list) : inner_list_{inner_list} {}

        void add_rect(const Rect& rect, const Color& color, float rounding = 0.0f, float thickness = 1.0f);
        void add_rect_filled(const Rect& rect, const Color& color, float rounding = 0.0f);
        void add_circle(const Circle& circle, const Color& color, int num_segments = 0, float thickness = 1.0f);
        void add_circle_filled(const Circle& circle, const Color& color, int num_segments = 0);
        void add_text(const Vec2& position, const Color& color, CStringView text);
        void add_line(const Vec2& p1, const Vec2& p2, const Color& color, float thickness = 1.0f);
        void add_triangle_filled(const Vec2 p0, const Vec2& p1, const Vec2& p2, const Color& color);

    private:
        ImDrawList* inner_list_;
    };

    class DrawList final {
    public:
        DrawList();
        DrawList(const DrawList&) = delete;
        DrawList(DrawList&&) noexcept;
        DrawList& operator=(const DrawList&) = delete;
        DrawList& operator=(DrawList&&) noexcept;
        ~DrawList() noexcept;

        operator DrawListView () { return DrawListView{underlying_drawlist_.get()}; }

        void render_to(RenderTexture&);
    private:
        std::unique_ptr<ImDrawList> underlying_drawlist_;
    };

    DrawListView get_panel_draw_list();
    DrawListView get_foreground_draw_list();

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
    bool any_of_keys_down(std::span<const Key>);
    bool any_of_keys_down(std::initializer_list<const Key>);

    // returns `true` if any scancode in the provided range was pressed down this frame
    bool any_of_keys_pressed(std::span<const Key>);
    bool any_of_keys_pressed(std::initializer_list<const Key>);

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
    bool is_mouse_released_without_dragging(MouseButton);
    bool is_mouse_released_without_dragging(MouseButton, float drag_threshold);

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
        HoveredFlags = HoveredFlag::ForTooltip
    );

    // draws an overlay tooltip with a header and description
    void draw_tooltip(CStringView header, CStringView description = {});

    // equivalent to `if (ui::is_item_hovered(flags)) draw_tooltip(header, description);`
    void draw_tooltip_if_item_hovered(
        CStringView header,
        CStringView description = {},
        HoveredFlags = HoveredFlag::ForTooltip
    );

    // draws a help text marker `"(?)"` and display a tooltip when the user hovers over it
    void draw_help_marker(CStringView header, CStringView description);

    // draws a help text marker `"(?)"` and display a tooltip when the user hovers over it
    void draw_help_marker(CStringView);

    // draws a text input that manipulates a `std::string`
    bool draw_string_input(
        CStringView label,
        std::string& edited_string,
        TextInputFlags = {}
    );

    // behaves like `ui::draw_float_input`, but understood to manipulate the scene scale
    bool draw_float_meters_input(
        CStringView label,
        float& v,
        float step = 0.0f,
        float step_fast = 0.0f,
        TextInputFlags = {}
    );

    // behaves like `ui::draw_float3_input`, but understood to manipulate the scene scale
    bool draw_float3_meters_input(
        CStringView label,
        Vec3&,
        TextInputFlags = {}
    );

    // behaves like `ui::draw_float_slider`, but understood to manipulate the scene scale
    bool draw_float_meters_slider(
        CStringView label,
        float& v,
        float v_min,
        float v_max,
        SliderFlags flags = {}
    );

    // behaves like `ui::draw_float_input`, but edits the given value as a mass (kg)
    bool draw_float_kilogram_input(
        CStringView label,
        float& v,
        float step = 0.0f,
        float step_fast = 0.0f,
        TextInputFlags = {}
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

    // returns "minimal" panel flags (i.e. no title bar, can't move the panel - ideal for images etc.)
    WindowFlags get_minimal_panel_flags();

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

    // returns the aspect ratio (width divided by height) of the pixel dimensions of the current workspace area
    float get_main_viewport_workspace_aspect_ratio();

    // returns `true` if the user's mouse is within the current workspace area of the main viewport
    bool is_mouse_in_main_viewport_workspace();

    // begin a menu that's attached to the top of a viewport, end it with `ui::end_panel()`
    bool begin_main_viewport_top_bar(
        CStringView label,
        float height = ui::get_frame_height(),
        WindowFlags = {WindowFlag::NoScrollbar, WindowFlag::NoSavedSettings, WindowFlag::MenuBar}
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
        SliderFlags = {}
    );

    // updates a polar comera's rotation, position, etc. from UI mouse input state
    bool update_polar_camera_from_mouse_inputs(
        PolarPerspectiveCamera&,
        Vec2 viewport_dimensions
    );

    // an operation that a ui `Gizmo` shall perform
    enum class GizmoOperation : unsigned {
        None         = 0,
        Translate    = 1<<0,
        Rotate       = 1<<1,
        Scale        = 1<<2,
        NUM_FLAGS    = 3,
    };
    using GizmoOperations = Flags<GizmoOperation>;

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

        enum class PlotStyleVar {
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

        enum class PlotColorVar {
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
        void push_style_var(PlotStyleVar, float);

        // temporarily modifies a style variable of `Vec2` type. Must be paired with `pop_style_var`
        void push_style_var(PlotStyleVar, Vec2);

        // undoes `count` temporary style variable modifications that were enacted by `push_style_var`
        void pop_style_var(int count = 1);

        // temporarily modifies a style color variable. Must be paired with `pop_style_color`
        void push_style_color(PlotColorVar, const Color&);

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
        namespace detail
        {
            void draw_annotation_v(Vec2 location_dataspace, const Color& color, Vec2 pixel_offset, bool clamp, CStringView fmt, va_list args);
        }
        inline void draw_annotation(Vec2 location_dataspace, const Color& color, Vec2 pixel_offset, bool clamp, CStringView fmt, ...)
        {
            va_list args;
            va_start(args, fmt);
            detail::draw_annotation_v(location_dataspace, color, pixel_offset, clamp, fmt, args);
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
        bool begin_legend_popup(CStringView label_id, MouseButton = MouseButton::Right);

        // ends a popup for a legend entry
        void end_legend_popup();
    }
}
