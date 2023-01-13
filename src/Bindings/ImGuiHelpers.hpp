#pragma once

#include "src/Maths/Rect.hpp"
#include "src/Utils/CStringView.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <nonstd/span.hpp>
#include <imgui.h>

#include <cstddef>
#include <initializer_list>
#include <string>

namespace osc { class Camera; }
namespace osc { class PolarPerspectiveCamera; }
namespace osc { class RenderTexture; }
namespace osc { class Texture2D; }
namespace osc { class UID; }

namespace osc
{
    // applies "dark" theme to current ImGui context
    void ImGuiApplyDarkTheme();

    // updates a polar comera's rotation, position, etc. based on ImGui input
    bool UpdatePolarCameraFromImGuiUserInput(glm::vec2 viewportDims, PolarPerspectiveCamera&);
    void UpdateEulerCameraFromImGuiUserInput(Camera&, glm::vec3& eulers);

    // returns the ImGui content region available in screenspace as a `Rect`
    Rect ContentRegionAvailScreenRect();

    // draws a texutre as an ImGui::Image
    void DrawTextureAsImGuiImage(Texture2D const&);
    void DrawTextureAsImGuiImage(Texture2D const&, glm::vec2 dims);  // assumes coords == [(0.0, 1.0), (1.0, 0.0)]
    void DrawTextureAsImGuiImage(Texture2D const&, glm::vec2 dims, glm::vec2 topLeftCoord, glm::vec2 bottomRightCoord);
    void DrawTextureAsImGuiImage(RenderTexture const&, glm::vec2 dims);
    void DrawTextureAsImGuiImage(RenderTexture const&);

    // draws a texture using ImGui::ImageButton
    bool ImageButton(CStringView, Texture2D const&, glm::vec2 dims);

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
    bool IsAnyKeyDown(nonstd::span<ImGuiKey const>);
    bool IsAnyKeyDown(std::initializer_list<ImGuiKey const>);

    // returns `true` if any scancode in the provided range was pressed down this frame
    bool IsAnyKeyPressed(nonstd::span<ImGuiKey const>);
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

    // returns `true` if the specified moouse button was released without the user dragging
    bool IsMouseReleasedWithoutDragging(ImGuiMouseButton);
    bool IsMouseReleasedWithoutDragging(ImGuiMouseButton, float dragThreshold);

    // draws an overlay tooltip (content only)
    void DrawTooltipBodyOnly(CStringView);

    // draws an overlay tooltip (content only) if the last item is hovered
    void DrawTooltipBodyOnlyIfItemHovered(CStringView);

    // draws an overlay tooltip with a header and description
    void DrawTooltip(CStringView header, CStringView description = {});

    // equivalent to `if (ImGui::IsItemHovered()) DrawTooltip(header, description);`
    void DrawTooltipIfItemHovered(CStringView header, CStringView description = {});

    // draw overlay axes at the cursor position and return the bounding box of those axes
    glm::vec2 CalcAlignmentAxesDimensions();
    Rect DrawAlignmentAxes(glm::mat4 const& viewMtx);

    // draw a help text marker `"(?)"` and display a tooltip when the user hovers over it
    void DrawHelpMarker(CStringView header, CStringView desc);

    // draw a help text marker `"(?)"` and display a tooltip when the user hovers over it
    void DrawHelpMarker(CStringView);

    // draw an ImGui::InputText that manipulates a std::string
    bool InputString(
        CStringView label,
        std::string& editedString,
        size_t maxLen,
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
        glm::vec3&,
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

    // push an osc::UID as if it were an ImGui ID (via ImGui::PushID)
    void PushID(UID const&);

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

    // draw text, but centered on the current window/line
    void TextCentered(CStringView);

    // returns `true` if a given item (usually, input) should be saved based on heuristics
    //
    // - if the item was deactivated (e.g. due to focusing something else), it should be saved
    // - if there's an active edit and the user presses enter, it should be saved
    // - if there's an active edit and the user presses tab, it should be saved
    bool ItemValueShouldBeSaved();
}
