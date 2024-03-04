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
    // applies "dark" theme to current ImGui context
    void ImGuiApplyDarkTheme();

    // updates a polar comera's rotation, position, etc. based on ImGui mouse input state
    bool UpdatePolarCameraFromImGuiMouseInputs(
        PolarPerspectiveCamera&,
        Vec2 viewportDims
    );
    // updates a polar comera's rotation, position, etc. based on ImGui keyboard input state
    bool UpdatePolarCameraFromImGuiKeyboardInputs(
        PolarPerspectiveCamera&,
        Rect const& viewportRect,
        std::optional<AABB> maybeSceneAABB
    );
    // updates a polar comera's rotation, position, etc. based on ImGui input (mouse+keyboard) state
    bool UpdatePolarCameraFromImGuiInputs(
        PolarPerspectiveCamera&,
        Rect const& viewportRect,
        std::optional<AABB> maybeSceneAABB
    );

    void UpdateEulerCameraFromImGuiUserInput(
        Camera&,
        Eulers&
    );

    // returns the ImGui content region available in screenspace as a `Rect`
    Rect ContentRegionAvailScreenRect();

    // draws a texutre as a ui::Image
    //
    // assumes coords == [(0.0, 1.0), (1.0, 0.0)]
    void DrawTextureAsImGuiImage(
        Texture2D const&
    );
    void DrawTextureAsImGuiImage(
        Texture2D const&,
        Vec2 dims
    );
    void DrawTextureAsImGuiImage(
        Texture2D const&,
        Vec2 dims,
        Vec2 topLeftCoord,
        Vec2 bottomRightCoord
    );
    void DrawTextureAsImGuiImage(
        RenderTexture const&
    );
    void DrawTextureAsImGuiImage(
        RenderTexture const&,
        Vec2 dims
    );

    // returns the dimensions of a button with the given content
    Vec2 CalcButtonSize(CStringView);
    float CalcButtonWidth(CStringView);

    bool ButtonNoBg(
        CStringView,
        Vec2 size = {0.0f, 0.0f}
    );

    // draws a texture using a ui::ImageButton
    bool ImageButton(
        CStringView,
        Texture2D const&,
        Vec2 dims,
        Rect const& textureCoords
    );
    bool ImageButton(
        CStringView,
        Texture2D const&,
        Vec2 dims
    );

    // returns the screenspace bounding rectangle of the last-drawn item
    Rect GetItemRect();

    // hittest the last-drawn ImGui item
    struct ImGuiItemHittestResult final {
        Rect rect = {};
        bool isHovered = false;
        bool isLeftClickReleasedWithoutDragging = false;
        bool isRightClickReleasedWithoutDragging = false;
    };
    ImGuiItemHittestResult HittestLastImguiItem();
    ImGuiItemHittestResult HittestLastImguiItem(float dragThreshold);

    // returns `true` if any scancode in the provided range is currently pressed down
    bool IsAnyKeyDown(std::span<ImGuiKey const>);
    bool IsAnyKeyDown(std::initializer_list<ImGuiKey const>);

    // returns `true` if any scancode in the provided range was pressed down this frame
    bool IsAnyKeyPressed(std::span<ImGuiKey const>);
    bool IsAnyKeyPressed(std::initializer_list<ImGuiKey const>);

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
    bool IsMouseReleasedWithoutDragging(ImGuiMouseButton, float dragThreshold);

    // returns `true` if the user is dragging their mouse with any button pressed
    bool IsDraggingWithAnyMouseButtonDown();

    // (lower-level tooltip methods: prefer using higher-level 'DrawTooltip(txt)' methods)
    void BeginTooltip(std::optional<float> wrapWidth = std::nullopt);
    void EndTooltip(std::optional<float> wrapWidth = std::nullopt);
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
    void DrawHelpMarker(CStringView header, CStringView desc);

    // draw a help text marker `"(?)"` and display a tooltip when the user hovers over it
    void DrawHelpMarker(CStringView);

    // draw a ui::InputText that manipulates a std::string
    bool InputString(
        CStringView label,
        std::string& editedString,
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

    // convert a color to ImU32 (used by ImGui's drawlist)
    ImU32 ToImU32(Color const&);
    Color ToColor(ImU32);

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
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar
    );

    // begin a menu that's attached to the bottom of a viewport, end it with ui::End();
    bool BeginMainViewportBottomBar(CStringView);

    // draws a ui::Button, but centered on the current line
    bool ButtonCentered(CStringView);

    // draw text, but centered on the current window/line
    void TextCentered(CStringView);

    // draw disabled text, but centered on the current window/line
    void TextDisabledAndCentered(CStringView);

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
        std::function<CStringView(size_t)> const& accessor
    );

    bool Combo(
        CStringView label,
        size_t* current,
        std::span<CStringView const> items
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
