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
    void ApplyDarkTheme();

    // updates a polar comera's rotation, position, etc. from UI mouse input state
    bool UpdatePolarCameraFromMouseInputs(
        PolarPerspectiveCamera&,
        Vec2 viewport_dimensions
    );

    // updates a polar comera's rotation, position, etc. from UI keyboard input state
    bool UpdatePolarCameraFromKeyboardInputs(
        PolarPerspectiveCamera&,
        const Rect& viewport_rect,
        std::optional<AABB> maybe_scene_aabb
    );

    // updates a polar comera's rotation, position, etc. from UI input state (all)
    bool UpdatePolarCameraFromInputs(
        PolarPerspectiveCamera&,
        const Rect& viewport_rect,
        std::optional<AABB> maybe_scene_aabb
    );

    void UpdateCameraFromInputs(
        Camera&,
        Eulers&
    );

    // returns the UI content region available in screenspace as a `Rect`
    Rect ContentRegionAvailScreenRect();

    // draws a texture within the 2D UI
    //
    // assumes the texture coordinates are [(0.0, 1.0), (1.0, 0.0)]
    void Image(
        const Texture2D&
    );
    void Image(
        const Texture2D&,
        Vec2 dimensions
    );
    void Image(
        const Texture2D&,
        Vec2 dimensions,
        Vec2 top_left_texture_coordinate,
        Vec2 bottom_right_texture_coordinate
    );
    void Image(
        const RenderTexture&
    );
    void Image(
        const RenderTexture&,
        Vec2 dimensions
    );

    // returns the dimensions of a button with the given content
    Vec2 CalcButtonSize(CStringView);
    float CalcButtonWidth(CStringView);

    bool ButtonNoBg(
        CStringView,
        Vec2 dimensions = {0.0f, 0.0f}
    );

    // draws a texture within the UI as a clickable button
    bool ImageButton(
        CStringView,
        const Texture2D&,
        Vec2 dimensions,
        const Rect& texture_coordinates
    );
    bool ImageButton(
        CStringView,
        const Texture2D&,
        Vec2 dimensions
    );

    // returns the screenspace bounding rectangle of the last-drawn item
    Rect GetItemRect();

    // hittest the last-drawn item in the UI
    struct HittestResult final {
        Rect item_rect = {};
        bool is_hovered = false;
        bool is_left_click_released_without_dragging = false;
        bool is_right_click_released_without_dragging = false;
    };
    HittestResult HittestLastItem();
    HittestResult HittestLastItem(float drag_threshold);

    // returns `true` if any scancode in the provided range is currently pressed down
    bool IsAnyKeyDown(std::span<const ImGuiKey>);
    bool IsAnyKeyDown(std::initializer_list<const ImGuiKey>);

    // returns `true` if any scancode in the provided range was pressed down this frame
    bool IsAnyKeyPressed(std::span<const ImGuiKey>);
    bool IsAnyKeyPressed(std::initializer_list<const ImGuiKey>);

    // returns true if the user is pressing either left- or right-Ctrl
    bool IsCtrlDown();

    // returns `true` if the user is pressing either:
    //
    // - left Ctrl
    // - right Ctrl
    // - left Super (mac)
    // - right Super (mac)
    bool IsCtrlOrSuperDown();

    // returns `true` if the user is pressing either left- or right-shift
    bool IsShiftDown();

    // returns `true` if the user is pressing either left- or right-alt
    bool IsAltDown();

    // returns `true` if the specified mouse button was released without the user dragging
    bool IsMouseReleasedWithoutDragging(ImGuiMouseButton);
    bool IsMouseReleasedWithoutDragging(ImGuiMouseButton, float drag_threshold);

    // returns `true` if the user is dragging their mouse with any button pressed
    bool IsDraggingWithAnyMouseButtonDown();

    // (lower-level tooltip methods: prefer using higher-level 'DrawTooltip(txt)' methods)
    void BeginTooltip(std::optional<float> wrap_width = std::nullopt);
    void EndTooltip(std::optional<float> wrap_width = std::nullopt);
    void TooltipHeaderText(CStringView);
    void TooltipDescriptionSpacer();
    void TooltipDescriptionText(CStringView);

    // draws an overlay tooltip (content only)
    void DrawTooltipBodyOnly(CStringView);

    // draws an overlay tooltip (content only) if the last item is hovered
    void DrawTooltipBodyOnlyIfItemHovered(
        CStringView,
        ImGuiHoveredFlags flags = ImGuiHoveredFlags_ForTooltip
    );

    // draws an overlay tooltip with a header and description
    void DrawTooltip(CStringView header, CStringView description = {});

    // equivalent to `if (ui::IsItemHovered(flags)) DrawTooltip(header, description);`
    void DrawTooltipIfItemHovered(
        CStringView header,
        CStringView description = {},
        ImGuiHoveredFlags flags = ImGuiHoveredFlags_ForTooltip
    );

    // draw a help text marker `"(?)"` and display a tooltip when the user hovers over it
    void DrawHelpMarker(CStringView header, CStringView description);

    // draw a help text marker `"(?)"` and display a tooltip when the user hovers over it
    void DrawHelpMarker(CStringView);

    // draw a ui::InputText that manipulates a std::string
    bool InputString(
        CStringView label,
        std::string& edited_string,
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_None
    );

    // draw a ui::InputFloat that manipulates in the scene scale (note: some users work with very very small sizes)
    bool InputMetersFloat(
        CStringView label,
        float& v,
        float step = 0.0f,
        float step_fast = 0.0f,
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_None
    );

    // draw a ui::InputFloat3 that manipulates in the scene scale (note: some users work with very very small sizes)
    bool InputMetersFloat3(
        CStringView label,
        Vec3&,
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_None
    );

    // draw a ui::SliderFloat that manipulates in the scene scale (note: some users work with very very small sizes)
    bool SliderMetersFloat(
        CStringView label,
        float& v,
        float v_min,
        float v_max,
        ImGuiSliderFlags flags = ImGuiInputTextFlags_None
    );

    // draw a ui::InputFloat for masses (note: some users work with very very small masses)
    bool InputKilogramFloat(
        CStringView label,
        float& v,
        float step = 0.0f,
        float step_fast = 0.0f,
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_None
    );

    // draw a ui::InputFloat that edits the given angular value in degrees
    bool InputAngle(
        CStringView label,
        Radians& v
    );

    // draw a ui::InputFloat3 that edits the given angular value in degrees
    bool InputAngle3(
        CStringView label,
        Vec<3, Radians>&,
        CStringView format = "%.3f"
    );

    // draw a ui::SliderFloat that edits the given angular value as degrees
    bool SliderAngle(
        CStringView label,
        Radians& v,
        Radians min,
        Radians max
    );

    // returns an `ImU32` converted from the given `Color`
    ImU32 ToImU32(const Color&);

    // returns a `Color` converted from the given LDR 8-bit `ImU32` format
    Color to_color(ImU32);

    // returns a `Color` converted from the given LDR `ImVec4` color
    Color to_color(const ImVec4&);

    // returns an `ImVec4` converted from the given `Color`
    ImVec4 ToImVec4(const Color&);

    // returns "minimal" window flags (i.e. no title bar, can't move the window - ideal for images etc.)
    ImGuiWindowFlags GetMinimalWindowFlags();

    // returns a `Rect` that indicates where the current workspace area is in the main viewport
    //
    // handy if (e.g.) you want to know the rect of a tab area
    Rect GetMainViewportWorkspaceScreenRect();

    // returns `true` if the user's mouse is within the current workspace area of the main viewport
    bool IsMouseInMainViewportWorkspaceScreenRect();

    // begin a menu that's attached to the top of a viewport, end it with ui::End();
    bool BeginMainViewportTopBar(
        CStringView label,
        float height = ui::GetFrameHeight(),
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar
    );

    // begin a menu that's attached to the bottom of a viewport, end it with ui::End();
    bool BeginMainViewportBottomBar(CStringView);

    // draws a ui::Button, but centered on the current line
    bool ButtonCentered(CStringView);

    // draw text, but centered on the current line
    void TextCentered(CStringView);

    // draw text, but centered in the current window (i.e. vertically and horizontally)
    void TextWindowCentered(CStringView);

    // draw disabled text, but centered on the current line
    void TextDisabledAndCentered(CStringView);

    // draw diabled text, but centered on the current window
    void TextDisabledAndWindowCentered(CStringView);

    // draw text that's centered in the current ui::Table column
    void TextColumnCentered(CStringView);

    // draw faded (muted) text
    void TextFaded(CStringView);

    // draw warning text
    void TextWarning(CStringView);

    // returns `true` if a given item (usually, input) should be saved based on heuristics
    //
    // - if the item was deactivated (e.g. due to focusing something else), it should be saved
    // - if there's an active edit and the user presses enter, it should be saved
    // - if there's an active edit and the user presses tab, it should be saved
    bool ItemValueShouldBeSaved();

    void PopItemFlags(int n = 1);

    bool Combo(
        CStringView label,
        size_t* current,
        size_t size,
        const std::function<CStringView(size_t)>& accessor
    );

    bool Combo(
        CStringView label,
        size_t* current,
        std::span<const CStringView> items
    );

    void VerticalSeperator();
    void SameLineWithVerticalSeperator();

    bool CircularSliderFloat(
        CStringView label,
        float* v,
        float min,
        float max,
        CStringView format = "%.3f",
        ImGuiSliderFlags flags = 0
    );
}
