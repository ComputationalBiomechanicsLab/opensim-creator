#pragma once

#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <imgui.h>

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

namespace osc
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
        Vec3& eulers
    );

    // returns the ImGui content region available in screenspace as a `Rect`
    Rect ContentRegionAvailScreenRect();

    // draws a texutre as an ImGui::Image
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

    // draws a texture using ImGui::ImageButton
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
    void BeginTooltip();
    void EndTooltip();
    void TooltipHeaderText(CStringView);
    void TooltipDescriptionSpacer();
    void TooltipDescriptionText(CStringView);

    // draws an overlay tooltip (content only)
    void DrawTooltipBodyOnly(CStringView);

    // draws an overlay tooltip (content only) if the last item is hovered
    void DrawTooltipBodyOnlyIfItemHovered(CStringView);

    // draws an overlay tooltip with a header and description
    void DrawTooltip(CStringView header, CStringView description = {});

    // equivalent to `if (ImGui::IsItemHovered()) DrawTooltip(header, description);`
    void DrawTooltipIfItemHovered(CStringView header, CStringView description = {});

    // draw overlay axes at the cursor position and return the bounding box of those axes
    Vec2 CalcAlignmentAxesDimensions();
    Rect DrawAlignmentAxes(Mat4 const& viewMtx);

    // draw a help text marker `"(?)"` and display a tooltip when the user hovers over it
    void DrawHelpMarker(CStringView header, CStringView desc);

    // draw a help text marker `"(?)"` and display a tooltip when the user hovers over it
    void DrawHelpMarker(CStringView);

    // draw an ImGui::InputText that manipulates a std::string
    bool InputString(
        CStringView label,
        std::string& editedString,
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_None
    );

    // draw an ImGui::InputFloat that manipulates in the scene scale (note: some users work with very very small sizes)
    bool InputMetersFloat(
        CStringView label,
        float& v,
        float step = 0.0f,
        float step_fast = 0.0f,
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_None
    );

    // draw an ImGui::InputFloat3 that manipulates in the scene scale (note: some users work with very very small sizes)
    bool InputMetersFloat3(
        CStringView label,
        Vec3&,
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_None
    );

    // draw an ImGui::SliderFloat that manipulates in the scene scale (note: some users work with very very small sizes)
    bool SliderMetersFloat(
        CStringView label,
        float& v,
        float v_min,
        float v_max,
        ImGuiSliderFlags flags = ImGuiInputTextFlags_None
    );

    // draw an ImGui::InputFloat for masses (note: some users work with very very small masses)
    bool InputKilogramFloat(
        CStringView label,
        float& v,
        float step = 0.0f,
        float step_fast = 0.0f,
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_None
    );

    // push things as-if by calling `ImGui::PushID(int);`
    void PushID(UID);
    void PushID(ptrdiff_t);

    // symmetric equivalent to `ImGui::PopID();`
    void PopID();

    // push an Color as an ImGui style color var (via ImGui::PushStyleColor())
    void PushStyleColor(ImGuiCol, Color const&);
    void PopStyleColor(int count = 1);

    // returns "minimal" window flags (i.e. no title bar, can't move the window - ideal for images etc.)
    ImGuiWindowFlags GetMinimalWindowFlags();

    // returns a `Rect` that indicates where the current workspace area is in the main viewport
    //
    // handy if (e.g.) you want to know the rect of a tab area
    Rect GetMainViewportWorkspaceScreenRect();

    // returns `true` if the user's mouse is within the current workspace area of the main viewport
    bool IsMouseInMainViewportWorkspaceScreenRect();

    // begin a menu that's attached to the top of a viewport, end it with ImGui::End();
    bool BeginMainViewportTopBar(
        CStringView label,
        float height = ImGui::GetFrameHeight(),
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar
    );

    // begin a menu that's attached to the bottom of a viewport, end it with ImGui::End();
    bool BeginMainViewportBottomBar(CStringView);

    //  draw ImGui::Button, but centered on the current line
    bool ButtonCentered(CStringView);

    // draw text, but centered on the current window/line
    void TextCentered(CStringView);

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

    // converts all color values in all draw commands' vertex buffers from sRGB to linear
    // color space
    void ConvertDrawDataFromSRGBToLinear(ImDrawData&);

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

    void BeginDisabled();
    void EndDisabled();
}
