#include "ImGuiHelpers.h"

#include <oscar/Graphics/Camera.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/RenderTexture.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Maths/CollisionTests.h>
#include <oscar/Maths/Eulers.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/VecFunctions.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/oscimgui_internal.h>
#include <oscar/UI/ui_graphics_backend.h>
#include <oscar/Utils/UID.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <span>
#include <string>

using namespace osc::literals;
using namespace osc;

namespace
{
    inline constexpr float c_DefaultDragThreshold = 5.0f;

    template<
        std::ranges::random_access_range TCollection,
        std::ranges::random_access_range UCollection
    >
    float diff(TCollection const& older, UCollection const& newer, size_t n)
        requires
            std::convertible_to<typename TCollection::value_type, float> &&
            std::convertible_to<typename UCollection::value_type, float>
    {
        for (size_t i = 0; i < n; ++i)
        {
            if (static_cast<float>(older[i]) != static_cast<float>(newer[i]))
            {
                return newer[i];
            }
        }
        return static_cast<float>(older[0]);
    }

    Vec2 RectMidpoint(ImRect const& r)
    {
        return 0.5f * (Vec2{r.Min} + Vec2{r.Max});
    }

    Vec2 Size(ImRect const& r)
    {
        return Vec2{r.Max} - Vec2{r.Min};
    }

    float ShortestEdgeLength(ImRect const& r)
    {
        const Vec2 sz = Size(r);
        return std::min(sz.x, sz.y);
    }

    ImU32 Brighten(ImU32 color, float factor)
    {
        const Color srgb = ToColor(color);
        const Color brightened = factor * srgb;
        const Color clamped = ClampToLDR(brightened);
        return ToImU32(clamped);
    }
}

void osc::ImGuiApplyDarkTheme()
{
    // see: https://github.com/ocornut/imgui/issues/707
    // this one: https://github.com/ocornut/imgui/issues/707#issuecomment-512669512

    ImGui::GetStyle().FrameRounding = 0.0f;
    ImGui::GetStyle().GrabRounding = 20.0f;
    ImGui::GetStyle().GrabMinSize = 10.0f;

    auto& colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.2f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.24f, 0.32f, 0.35f, 0.70f);  // contrasts against other Header* elements (#677)
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.6f);
}

bool osc::UpdatePolarCameraFromImGuiMouseInputs(
    PolarPerspectiveCamera& camera,
    Vec2 viewportDims)
{
    bool modified = false;

    // handle mousewheel scrolling
    if (ImGui::GetIO().MouseWheel != 0.0f)
    {
        camera.radius *= 1.0f - 0.1f * ImGui::GetIO().MouseWheel;
        modified = true;
    }

    // these camera controls try to be the union of other GUIs (e.g. Blender)
    //
    // left drag: drags/orbits camera
    // left drag + L/R SHIFT: pans camera (CUSTOM behavior: can be handy on laptops where right-click + drag sucks)
    // left drag + L/R CTRL: zoom camera (CUSTOM behavior: can be handy on laptops where right-click + drag sucks)
    // middle drag: drags/orbits camera (Blender behavior)
    // middle drag + L/R SHIFT: pans camera (Blender behavior)
    // middle drag + L/R CTRL: zooms camera (Blender behavior)
    // right drag: pans camera
    //
    // the reason it's like this is to please legacy users from a variety of
    // other GUIs and users who use modelling software like Blender (which is
    // more popular among newer users looking to make new models)

    float const aspectRatio = viewportDims.x / viewportDims.y;

    bool const leftDragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
    bool const middleDragging = ImGui::IsMouseDragging(ImGuiMouseButton_Middle);
    Vec2 const delta = ImGui::GetIO().MouseDelta;

    if (delta != Vec2{0.0f, 0.0f} && (leftDragging || middleDragging))
    {
        if (IsCtrlDown())
        {
            camera.pan(aspectRatio, delta/viewportDims);
            modified = true;
        }
        else if (IsCtrlOrSuperDown())
        {
            camera.radius *= 1.0f + 4.0f*delta.y/viewportDims.y;
            modified = true;
        }
        else
        {
            camera.drag(delta/viewportDims);
            modified = true;
        }

    }
    else if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    {
        if (IsAltDown())
        {
            camera.radius *= 1.0f + 4.0f*delta.y/viewportDims.y;
            modified = true;
        }
        else
        {
            camera.pan(aspectRatio, delta/viewportDims);
            modified = true;
        }
    }

    if (modified)
    {
        camera.rescaleZNearAndZFarBasedOnRadius();
    }

    return modified;
}

bool osc::UpdatePolarCameraFromImGuiKeyboardInputs(
    PolarPerspectiveCamera& camera,
    Rect const& viewportRect,
    std::optional<AABB> maybeSceneAABB)
{
    bool const shiftDown = IsShiftDown();
    bool const ctrlOrSuperDown = IsCtrlOrSuperDown();

    if (ImGui::IsKeyReleased(ImGuiKey_X))
    {
        if (ctrlOrSuperDown)
        {
            FocusAlongMinusX(camera);
            return true;
        } else
        {
            FocusAlongX(camera);
            return true;
        }
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_Y))
    {
        // Ctrl+Y already does something?
        if (!ctrlOrSuperDown)
        {
            FocusAlongY(camera);
            return true;
        }
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_F))
    {
        if (ctrlOrSuperDown)
        {
            if (maybeSceneAABB)
            {
                AutoFocus(
                    camera,
                    *maybeSceneAABB,
                    AspectRatio(viewportRect)
                );
                return true;
            }
        }
        else
        {
            Reset(camera);
            return true;
        }
    }
    else if (ctrlOrSuperDown && (ImGui::IsKeyPressed(ImGuiKey_8)))
    {
        if (maybeSceneAABB)
        {
            AutoFocus(
                camera,
                *maybeSceneAABB,
                AspectRatio(viewportRect)
            );
            return true;
        }
    }
    else if (ImGui::IsKeyDown(ImGuiKey_UpArrow))
    {
        if (ctrlOrSuperDown)
        {
            // pan
            camera.pan(AspectRatio(viewportRect), {0.0f, -0.1f});
        }
        else if (shiftDown)
        {
            camera.phi -= 90_deg;  // rotate in 90-deg increments
        }
        else
        {
            camera.phi -= 10_deg;  // rotate in 10-deg increments
        }
        return true;
    }
    else if (ImGui::IsKeyDown(ImGuiKey_DownArrow))
    {
        if (ctrlOrSuperDown)
        {
            // pan
            camera.pan(AspectRatio(viewportRect), {0.0f, +0.1f});
        }
        else if (shiftDown)
        {
            // rotate in 90-deg increments
            camera.phi += 90_deg;
        }
        else
        {
            // rotate in 10-deg increments
            camera.phi += 10_deg;
        }
        return true;
    }
    else if (ImGui::IsKeyDown(ImGuiKey_LeftArrow))
    {
        if (ctrlOrSuperDown)
        {
            // pan
            camera.pan(AspectRatio(viewportRect), {-0.1f, 0.0f});
        }
        else if (shiftDown)
        {
            // rotate in 90-deg increments
            camera.theta += 90_deg;
        }
        else
        {
            // rotate in 10-deg increments
            camera.theta += 10_deg;
        }
        return true;
    }
    else if (ImGui::IsKeyDown(ImGuiKey_RightArrow))
    {
        if (ctrlOrSuperDown)
        {
            // pan
            camera.pan(AspectRatio(viewportRect), {+0.1f, 0.0f});
        }
        else if (shiftDown)
        {
            camera.theta -= 90_deg;  // rotate in 90-deg increments
        }
        else
        {
            camera.theta -= 10_deg;  // rotate in 10-deg increments
        }
        return true;
    }
    else if (ImGui::IsKeyDown(ImGuiKey_Minus))
    {
        camera.radius *= 1.1f;
        return true;
    }
    else if (ImGui::IsKeyDown(ImGuiKey_Equal))
    {
        camera.radius *= 0.9f;
        return true;
    }
    return false;
}

bool osc::UpdatePolarCameraFromImGuiInputs(
    PolarPerspectiveCamera& camera,
    Rect const& viewportRect,
    std::optional<AABB> maybeSceneAABB)
{

    ImGuiIO& io = ImGui::GetIO();

    // we don't check `io.WantCaptureMouse` because clicking/dragging on an ImGui::Image
    // is classed as a mouse interaction
    bool const mouseHandled =
        UpdatePolarCameraFromImGuiMouseInputs(camera, dimensions(viewportRect));
    bool const keyboardHandled = !io.WantCaptureKeyboard ?
        UpdatePolarCameraFromImGuiKeyboardInputs(camera, viewportRect, maybeSceneAABB) :
        false;

    return mouseHandled || keyboardHandled;
}

void osc::UpdateEulerCameraFromImGuiUserInput(Camera& camera, Eulers& eulers)
{
    Vec3 const front = camera.getDirection();
    Vec3 const up = camera.getUpwardsDirection();
    Vec3 const right = cross(front, up);
    Vec2 const mouseDelta = ImGui::GetIO().MouseDelta;

    float const speed = 10.0f;
    float const displacement = speed * ImGui::GetIO().DeltaTime;
    Radians const sensitivity{0.005f};

    // keyboard: changes camera position
    Vec3 pos = camera.getPosition();
    if (ImGui::IsKeyDown(ImGuiKey_W))
    {
        pos += displacement * front;
    }
    if (ImGui::IsKeyDown(ImGuiKey_S))
    {
        pos -= displacement * front;
    }
    if (ImGui::IsKeyDown(ImGuiKey_A))
    {
        pos -= displacement * right;
    }
    if (ImGui::IsKeyDown(ImGuiKey_D))
    {
        pos += displacement * right;
    }
    if (ImGui::IsKeyDown(ImGuiKey_Space))
    {
        pos += displacement * up;
    }
    if (ImGui::GetIO().KeyCtrl)
    {
        pos -= displacement * up;
    }
    camera.setPosition(pos);

    eulers.x += sensitivity * -mouseDelta.y;
    eulers.x = clamp(eulers.x, -90_deg + 0.1_rad, 90_deg - 0.1_rad);
    eulers.y += sensitivity * -mouseDelta.x;
    eulers.y = mod(eulers.y, 360_deg);

    camera.setRotation(WorldspaceRotation(eulers));
}

Rect osc::ContentRegionAvailScreenRect()
{
    Vec2 const topLeft = ImGui::GetCursorScreenPos();
    Vec2 const dims = ImGui::GetContentRegionAvail();
    Vec2 const bottomRight = topLeft + dims;

    return Rect{topLeft, bottomRight};
}

void osc::DrawTextureAsImGuiImage(Texture2D const& t)
{
    DrawTextureAsImGuiImage(t, t.getDimensions());
}

void osc::DrawTextureAsImGuiImage(Texture2D const& t, Vec2 dims)
{
    Vec2 const topLeftCoord = {0.0f, 1.0f};
    Vec2 const bottomRightCoord = {1.0f, 0.0f};
    DrawTextureAsImGuiImage(t, dims, topLeftCoord, bottomRightCoord);
}

void osc::DrawTextureAsImGuiImage(
    Texture2D const& t,
    Vec2 dims,
    Vec2 topLeftCoord,
    Vec2 bottomRightCoord)
{
    auto const handle = ui::graphics_backend::AllocateTextureID(t);
    ImGui::Image(handle, dims, topLeftCoord, bottomRightCoord);
}

void osc::DrawTextureAsImGuiImage(RenderTexture const& tex)
{
    return DrawTextureAsImGuiImage(tex, tex.getDimensions());
}

void osc::DrawTextureAsImGuiImage(RenderTexture const& t, Vec2 dims)
{
    Vec2 const uv0 = {0.0f, 1.0f};
    Vec2 const uv1 = {1.0f, 0.0f};
    auto const handle = ui::graphics_backend::AllocateTextureID(t);
    ImGui::Image(handle, dims, uv0, uv1);
}

Vec2 osc::CalcButtonSize(CStringView content)
{
    Vec2 const padding = ImGui::GetStyle().FramePadding;
    Vec2 const contentDims = ImGui::CalcTextSize(content.c_str());
    return contentDims + 2.0f*padding;
}

float osc::CalcButtonWidth(CStringView content)
{
    return CalcButtonSize(content).x;
}

bool osc::ButtonNoBg(CStringView label, Vec2 size)
{
    PushStyleColor(ImGuiCol_Button, Color::clear());
    PushStyleColor(ImGuiCol_ButtonHovered, Color::clear());
    bool const rv = ui::Button(label.c_str(), size);
    PopStyleColor();
    PopStyleColor();

    return rv;
}

bool osc::ImageButton(
    CStringView label,
    Texture2D const& t,
    Vec2 dims,
    Rect const& textureCoords)
{
    auto const handle = ui::graphics_backend::AllocateTextureID(t);
    return ImGui::ImageButton(label.c_str(), handle, dims, textureCoords.p1, textureCoords.p2);
}

bool osc::ImageButton(CStringView label, Texture2D const& t, Vec2 dims)
{
    return ImageButton(label, t, dims, Rect{{0.0f, 1.0f}, {1.0f, 0.0f}});
}

Rect osc::GetItemRect()
{
    return {ImGui::GetItemRectMin(), ImGui::GetItemRectMax()};
}

ImGuiItemHittestResult osc::HittestLastImguiItem()
{
    return HittestLastImguiItem(c_DefaultDragThreshold);
}

ImGuiItemHittestResult osc::HittestLastImguiItem(float dragThreshold)
{
    ImGuiItemHittestResult rv;
    rv.rect.p1 = ImGui::GetItemRectMin();
    rv.rect.p2 = ImGui::GetItemRectMax();
    rv.isHovered = ImGui::IsItemHovered();
    rv.isLeftClickReleasedWithoutDragging = rv.isHovered && IsMouseReleasedWithoutDragging(ImGuiMouseButton_Left, dragThreshold);
    rv.isRightClickReleasedWithoutDragging = rv.isHovered && IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right, dragThreshold);
    return rv;
}

bool osc::IsAnyKeyDown(std::span<ImGuiKey const> keys)
{
    return std::any_of(keys.begin(), keys.end(), [](ImGuiKey k) { return ImGui::IsKeyDown(k); });
}

bool osc::IsAnyKeyDown(std::initializer_list<ImGuiKey const> keys)
{
    return IsAnyKeyDown(std::span<ImGuiKey const>{keys.begin(), keys.end()});
}

bool osc::IsAnyKeyPressed(std::span<ImGuiKey const> keys)
{
    return std::any_of(keys.begin(), keys.end(), [](ImGuiKey k) { return ImGui::IsKeyPressed(k); });
}
bool osc::IsAnyKeyPressed(std::initializer_list<ImGuiKey const> keys)
{
    return IsAnyKeyPressed(std::span<ImGuiKey const>{keys.begin(), keys.end()});
}

bool osc::IsCtrlDown()
{
    return ImGui::GetIO().KeyCtrl;
}

bool osc::IsCtrlOrSuperDown()
{
    return ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
}

bool osc::IsShiftDown()
{
    return ImGui::GetIO().KeyShift;
}

bool osc::IsAltDown()
{
    return ImGui::GetIO().KeyAlt;
}

bool osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton btn)
{
    return IsMouseReleasedWithoutDragging(btn, c_DefaultDragThreshold);
}

bool osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton btn, float threshold)
{
    if (!ImGui::IsMouseReleased(btn))
    {
        return false;
    }

    Vec2 const dragDelta = ImGui::GetMouseDragDelta(btn);

    return length(dragDelta) < threshold;
}

bool osc::IsDraggingWithAnyMouseButtonDown()
{
    return
        ImGui::IsMouseDragging(ImGuiMouseButton_Left) ||
        ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
        ImGui::IsMouseDragging(ImGuiMouseButton_Right);
}

void osc::BeginTooltip(std::optional<float> wrapWidth)
{
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(wrapWidth.value_or(ImGui::GetFontSize() * 35.0f));
}

void osc::EndTooltip()
{
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

void osc::TooltipHeaderText(CStringView s)
{
    TextUnformatted(s);
}

void osc::TooltipDescriptionSpacer()
{
    ui::Dummy({0.0f, 1.0f});
}

void osc::TooltipDescriptionText(CStringView s)
{
    TextFaded(s);
}

void osc::DrawTooltipBodyOnly(CStringView label)
{
    BeginTooltip();
    TooltipHeaderText(label);
    EndTooltip();
}

void osc::DrawTooltipBodyOnlyIfItemHovered(
    CStringView label,
    ImGuiHoveredFlags flags)
{
    if (ImGui::IsItemHovered(flags))
    {
        DrawTooltipBodyOnly(label);
    }
}

void osc::DrawTooltip(CStringView header, CStringView description)
{
    BeginTooltip();
    TooltipHeaderText(header);
    if (!description.empty())
    {
        TooltipDescriptionSpacer();
        TooltipDescriptionText(description);
    }
    EndTooltip();
}

void osc::DrawTooltipIfItemHovered(
    CStringView header,
    CStringView description,
    ImGuiHoveredFlags flags)
{
    if (ImGui::IsItemHovered(flags))
    {
        DrawTooltip(header, description);
    }
}

// draw a help text marker `"(?)"` and display a tooltip when the user hovers over it
void osc::DrawHelpMarker(CStringView header, CStringView desc)
{
    ui::TextDisabled("(?)");
    DrawTooltipIfItemHovered(header, desc, ImGuiHoveredFlags_None);
}

// draw a help text marker `"(?)"` and display a tooltip when the user hovers over it
void osc::DrawHelpMarker(CStringView desc)
{
    ui::TextDisabled("(?)");
    DrawTooltipIfItemHovered(desc, {}, ImGuiHoveredFlags_None);
}

void osc::TextUnformatted(CStringView label)
{
    ui::TextUnformatted(label);
}

bool osc::InputString(CStringView label, std::string& editedString, ImGuiInputTextFlags flags)
{
    return ImGui::InputText(label.c_str(), &editedString, flags);  // uses `imgui_stdlib`
}

bool osc::InputMetersFloat(CStringView label, float& v, float step, float step_fast, ImGuiInputTextFlags flags)
{
    return ImGui::InputFloat(label.c_str(), &v, step, step_fast, "%.6f", flags);
}

bool osc::InputMetersFloat3(CStringView label, Vec3& vec, ImGuiInputTextFlags flags)
{
    return ImGui::InputFloat3(label.c_str(), value_ptr(vec), "%.6f", flags);
}

bool osc::SliderMetersFloat(CStringView label, float& v, float v_min, float v_max, ImGuiSliderFlags flags)
{
    return ImGui::SliderFloat(label.c_str(), &v, v_min, v_max, "%.6f", flags);
}

bool osc::InputKilogramFloat(CStringView label, float& v, float step, float step_fast, ImGuiInputTextFlags flags)
{
    return InputMetersFloat(label, v, step, step_fast, flags);
}

bool osc::InputAngle(CStringView label, Radians& v)
{
    float dv = Degrees{v}.count();
    if (ImGui::InputFloat(label.c_str(), &dv))
    {
        v = Degrees{dv};
        return true;
    }
    return false;
}

bool osc::InputAngle3(
    CStringView label,
    Vec<3, Radians>& vs,
    CStringView format)
{
    Vec3 dvs = {Degrees{vs.x}.count(), Degrees{vs.y}.count(), Degrees{vs.z}.count()};
    if (ImGui::InputFloat3(label.c_str(), value_ptr(dvs), format.c_str()))
    {
        vs = Vec<3, Degrees>{dvs};
        return true;
    }
    return false;
}

bool osc::SliderAngle(CStringView label, Radians& v, Radians min, Radians max)
{
    float dv = Degrees{v}.count();
    Degrees const dmin{min};
    Degrees const dmax{max};
    if (ImGui::SliderFloat(label.c_str(), &dv, dmin.count(), dmax.count()))
    {
        v = Degrees{dv};
        return true;
    }
    return false;
}

void osc::PushID(UID id)
{
    ImGui::PushID(static_cast<int>(id.get()));
}

void osc::PushID(ptrdiff_t p)
{
    ImGui::PushID(static_cast<int>(p));
}

void osc::PopID()
{
    ImGui::PopID();
}

ImU32 osc::ToImU32(Color const& color)
{
    return ImGui::ColorConvertFloat4ToU32(Vec4{color});
}

Color osc::ToColor(ImU32 u32color)
{
    return Color{Vec4{ImGui::ColorConvertU32ToFloat4(u32color)}};
}

void osc::PushStyleColor(ImGuiCol index, Color const& c)
{
    ImGui::PushStyleColor(index, {c.r, c.g, c.b, c.a});
}

void osc::PopStyleColor(int count)
{
    ImGui::PopStyleColor(count);
}

ImGuiWindowFlags osc::GetMinimalWindowFlags()
{
    return
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoTitleBar;
}

Rect osc::GetMainViewportWorkspaceScreenRect()
{
    ImGuiViewport const& viewport = *ImGui::GetMainViewport();

    return Rect
    {
        viewport.WorkPos,
        Vec2{viewport.WorkPos} + Vec2{viewport.WorkSize}
    };
}

bool osc::IsMouseInMainViewportWorkspaceScreenRect()
{
    Vec2 const mousepos = ImGui::GetMousePos();
    Rect const hitRect = GetMainViewportWorkspaceScreenRect();

    return is_point_in_rect(hitRect, mousepos);
}

bool osc::BeginMainViewportTopBar(CStringView label, float height, ImGuiWindowFlags flags)
{
    // https://github.com/ocornut/imgui/issues/3518
    auto* const viewport = static_cast<ImGuiViewportP*>(static_cast<void*>(ImGui::GetMainViewport()));
    return ImGui::BeginViewportSideBar(label.c_str(), viewport, ImGuiDir_Up, height, flags);
}


bool osc::BeginMainViewportBottomBar(CStringView label)
{
    // https://github.com/ocornut/imgui/issues/3518
    auto* const viewport = static_cast<ImGuiViewportP*>(static_cast<void*>(ImGui::GetMainViewport()));
    ImGuiWindowFlags const flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
    float const height = ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y;

    return ImGui::BeginViewportSideBar(label.c_str(), viewport, ImGuiDir_Down, height, flags);
}

bool osc::ButtonCentered(CStringView s)
{
    float const buttonWidth = ImGui::CalcTextSize(s.c_str()).x + 2.0f*ImGui::GetStyle().FramePadding.x;
    float const midpoint = ImGui::GetCursorScreenPos().x + 0.5f*ImGui::GetContentRegionAvail().x;
    float const buttonStartX = midpoint - 0.5f*buttonWidth;

    ImGui::SetCursorScreenPos({buttonStartX, ImGui::GetCursorScreenPos().y});

    return ui::Button(s.c_str());
}

void osc::TextCentered(CStringView s)
{
    float const windowWidth = ImGui::GetWindowSize().x;
    float const textWidth   = ImGui::CalcTextSize(s.c_str()).x;

    ImGui::SetCursorPosX(0.5f * (windowWidth - textWidth));
    TextUnformatted(s);
}

void osc::TextDisabledAndCentered(CStringView s)
{
    ui::BeginDisabled();
    TextCentered(s);
    ui::EndDisabled();
}

void osc::TextColumnCentered(CStringView s)
{
    float const columnWidth = ImGui::GetColumnWidth();
    float const columnOffset = ImGui::GetCursorPos().x;
    float const textWidth = ImGui::CalcTextSize(s.c_str()).x;

    ImGui::SetCursorPosX(columnOffset + 0.5f*(columnWidth-textWidth));
    TextUnformatted(s);
}

void osc::TextFaded(CStringView s)
{
    ImGui::PushStyleColor(ImGuiCol_Text, {0.7f, 0.7f, 0.7f, 1.0f});
    TextUnformatted(s);
    ImGui::PopStyleColor();
}

void osc::TextWarning(CStringView s)
{
    PushStyleColor(ImGuiCol_Text, Color::yellow());
    TextUnformatted(s);
    PopStyleColor();
}

bool osc::ItemValueShouldBeSaved()
{
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        return true;  // ImGui detected that the item was deactivated after an edit
    }

    if (ImGui::IsItemEdited() && IsAnyKeyPressed({ImGuiKey_Enter, ImGuiKey_Tab}))
    {
        return true;  // user pressed enter/tab after editing
    }

    return false;
}

void osc::PopItemFlags(int n)
{
    for (int i = 0; i < n; ++i)
    {
        ImGui::PopItemFlag();
    }
}

bool osc::Combo(
    CStringView label,
    size_t* current,
    size_t size,
    std::function<CStringView(size_t)> const& accessor)
{
    CStringView const preview = current != nullptr ?
        accessor(*current) :
        CStringView{};

    if (!ImGui::BeginCombo(label.c_str(), preview.c_str(), ImGuiComboFlags_None))
    {
        return false;
    }

    bool changed = false;
    for (size_t i = 0; i < size; ++i)
    {
        ImGui::PushID(static_cast<int>(i));
        bool const isSelected = current != nullptr && *current == i;
        if (ImGui::Selectable(accessor(i).c_str(), isSelected))
        {
            changed = true;
            if (current != nullptr)
            {
                *current = i;
            }
        }
        if (isSelected)
        {
            ImGui::SetItemDefaultFocus();
        }
        ImGui::PopID();
    }

    ImGui::EndCombo();

    if (changed)
    {
        ImGui::MarkItemEdited(ImGui::GetCurrentContext()->LastItemData.ID);
    }

    return changed;
}

bool osc::Combo(
    CStringView label,
    size_t* current,
    std::span<CStringView const> items)
{
    return Combo(
        label,
        current,
        items.size(),
        [items](size_t i) { return items[i]; }
    );
}

void osc::VerticalSeperator()
{
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
}

void osc::SameLineWithVerticalSeperator()
{
    ui::SameLine();
    VerticalSeperator();
    ui::SameLine();
}

bool osc::CircularSliderFloat(
    CStringView label,
    float* v,
    float min,
    float max,
    CStringView format,
    ImGuiSliderFlags flags)
{
    // this implementation was initially copied from `ImGui::SliderFloat` and written in a
    // similar style to code in `imgui_widgets.cpp` (see https://github.com/ocornut/imgui),
    // but has since mutated
    //
    // the display style etc. uses ideas from XEMU (https://github.com/xemu-project/xemu), which
    // contains custom widgets like sliders etc. - I liked XEMU's style but I needed some
    // features that `ImGui::SliderFloat` has, so I ended up with this mash-up

    // prefetch top-level state
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        // skip drawing: the window is not visible or it is clipped
        return false;
    }
    ImGuiContext& g = *ImGui::GetCurrentContext();
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.c_str());

    // calculate top-level item info for early-cull checks etc.
    const Vec2 labelSize = ImGui::CalcTextSize(label.c_str(), nullptr, true);
    const Vec2 frameDims = {ImGui::CalcItemWidth(), labelSize.y + 2.0f*style.FramePadding.y};
    const Vec2 cursorScreenPos = ImGui::GetCursorScreenPos();
    const ImRect frameBB = {cursorScreenPos, cursorScreenPos + frameDims};
    const float labelWidthWithSpacing = labelSize.x > 0.0f ? labelSize.x + style.ItemInnerSpacing.x : 0.0f;
    const ImRect totalBB = {frameBB.Min, Vec2{frameBB.Max} + Vec2{labelWidthWithSpacing, 0.0f}};

    const bool temporaryTextInputAllowed = (flags & ImGuiSliderFlags_NoInput) == 0;
    ImGui::ItemSize(totalBB, style.FramePadding.y);
    if (!ImGui::ItemAdd(totalBB, id, &frameBB, temporaryTextInputAllowed ? ImGuiItemFlags_Inputable : 0))
    {
        // skip drawing: the slider item is off-screen or not interactable
        return false;
    }
    const bool isHovered = ImGui::ItemHoverable(frameBB, id, g.LastItemData.InFlags);  // hovertest the item

    // figure out whether the user is (temporarily) editing the slider as an input text box
    bool temporaryTextInputActive = temporaryTextInputAllowed && ImGui::TempInputIsActive(id);
    if (!temporaryTextInputActive)
    {
        // tabbing or double clicking the slider temporarily transforms it into an input box
        const bool clicked = isHovered && ui::IsMouseClicked(ImGuiMouseButton_Left, id);
        const bool doubleClicked = (isHovered && g.IO.MouseClickedCount[0] == 2 && ImGui::TestKeyOwner(ImGuiKey_MouseLeft, id));
        const bool makeActive = (clicked || doubleClicked || g.NavActivateId == id);

        if (makeActive && (clicked || doubleClicked))
        {
            ImGui::SetKeyOwner(ImGuiKey_MouseLeft, id);  // tell ImGui that left-click is locked from further interaction etc. this frame
        }
        if (makeActive && temporaryTextInputAllowed)
        {
            if ((clicked && g.IO.KeyCtrl) || doubleClicked || (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput)))
            {
                temporaryTextInputActive = true;
            }
        }

        // if it's decided that the text input should be made active, then make it active
        // by focusing on it (e.g. give it keyboard focus)
        if (makeActive && !temporaryTextInputActive)
        {
            ImGui::SetActiveID(id, window);
            ImGui::SetFocusID(id, window);
            ImGui::FocusWindow(window);
            g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
        }
    }

    // if the user is editing the slider as an input text box then draw that instead of the slider
    if (temporaryTextInputActive)
    {
        const bool shouldClampTextualInput = (flags & ImGuiSliderFlags_AlwaysClamp) != 0;

        return ImGui::TempInputScalar(
            frameBB,
            id,
            label.c_str(),
            ImGuiDataType_Float,
            static_cast<void*>(v),
            format.c_str(),
            shouldClampTextualInput ? static_cast<void const*>(&min) : nullptr,
            shouldClampTextualInput ? static_cast<void const*>(&max) : nullptr
        );
    }
    // else: draw the slider (remainder of this func)

    // calculate slider behavior (interaction, etc.)
    //
    // note: I haven't studied `ImGui::SliderBehaviorT` in-depth. I'm just going to
    // go ahead and assume that it's doing the interaction/hittest/mutation logic
    // and leaves rendering to us.
    ImRect grabBoundingBox{};
    bool valueChanged = ImGui::SliderBehaviorT<float, float, float>(
        frameBB,
        id,
        ImGuiDataType_Float,
        v,
        min,
        max,
        format.c_str(),
        flags,
        &grabBoundingBox
    );
    if (valueChanged)
    {
        ImGui::MarkItemEdited(id);
    }

    // render
    bool const useCustomRendering = true;
    if (useCustomRendering)
    {
        const Vec2 sliderNobCenter = RectMidpoint(grabBoundingBox);
        const float sliderNobRadius = 0.75f * ShortestEdgeLength(grabBoundingBox);
        const float sliderRailThickness = 0.5f * sliderNobRadius;
        const float sliderRailTopY = sliderNobCenter.y - 0.5f*sliderRailThickness;
        const float sliderRailBottomY = sliderNobCenter.y + 0.5f*sliderRailThickness;

        const bool isActive = g.ActiveId == id;
        const ImU32 railColor = ImGui::GetColorU32(isHovered ? ImGuiCol_FrameBgHovered : isActive ? ImGuiCol_FrameBgActive : ImGuiCol_FrameBg);
        const ImU32 grabColor = ImGui::GetColorU32(isActive ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab);

        // render left-hand rail (brighter)
        {
            Vec2 const lhsRailTopLeft = {frameBB.Min.x, sliderRailTopY};
            Vec2 const lhsRailBottomright = {sliderNobCenter.x, sliderRailBottomY};
            const ImU32 brightenedRailColor = Brighten(railColor, 2.0f);

            window->DrawList->AddRectFilled(
                lhsRailTopLeft,
                lhsRailBottomright,
                brightenedRailColor,
                g.Style.FrameRounding
            );
        }

        // render right-hand rail
        {
            Vec2 const rhsRailTopLeft = {sliderNobCenter.x, sliderRailTopY};
            Vec2 const rhsRailBottomRight = {frameBB.Max.x, sliderRailBottomY};

            window->DrawList->AddRectFilled(
                rhsRailTopLeft,
                rhsRailBottomRight,
                railColor,
                g.Style.FrameRounding
            );
        }

        // render slider grab on top of rail
        window->DrawList->AddCircleFilled(
            sliderNobCenter,
            sliderNobRadius,  // visible nob is slightly smaller than virtual nob
            grabColor
        );

        // render current slider value using user-provided display format
        {
            std::array<char, 64> buf{};
            const char* const bufEnd = buf.data() + ImGui::DataTypeFormatString(buf.data(), static_cast<int>(buf.size()), ImGuiDataType_Float, v, format.c_str());
            if (g.LogEnabled)
            {
                ImGui::LogSetNextTextDecoration("{", "}");
            }
            ImGui::RenderTextClipped(frameBB.Min, frameBB.Max, buf.data(), bufEnd, nullptr, ImVec2(0.5f, 0.5f));
        }

        // render input label in remaining space
        if (labelSize.x > 0.0f)
        {
            ImGui::RenderText(ImVec2(frameBB.Max.x + style.ItemInnerSpacing.x, frameBB.Min.y + style.FramePadding.y), label.c_str());
        }
    }
    else
    {
        // render slider background frame
        {
            const ImU32 frameColor = ImGui::GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : isHovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
            ImGui::RenderNavHighlight(frameBB, id);
            ImGui::RenderFrame(frameBB.Min, frameBB.Max, frameColor, true, g.Style.FrameRounding);
        }

        // render slider grab handle
        if (grabBoundingBox.Max.x > grabBoundingBox.Min.x)
        {
            window->DrawList->AddRectFilled(grabBoundingBox.Min, grabBoundingBox.Max, ImGui::GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);
        }

        // render current slider value using user-provided display format
        {
            std::array<char, 64> buf{};
            const char* const bufEnd = buf.data() + ImGui::DataTypeFormatString(buf.data(), static_cast<int>(buf.size()), ImGuiDataType_Float, v, format.c_str());
            if (g.LogEnabled)
            {
                ImGui::LogSetNextTextDecoration("{", "}");
            }
            ImGui::RenderTextClipped(frameBB.Min, frameBB.Max, buf.data(), bufEnd, nullptr, ImVec2(0.5f, 0.5f));
        }

        // render input label in remaining space
        if (labelSize.x > 0.0f)
        {
            ImGui::RenderText(ImVec2(frameBB.Max.x + style.ItemInnerSpacing.x, frameBB.Min.y + style.FramePadding.y), label.c_str());
        }
    }

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));

    return valueChanged;
}

void osc::BeginDisabled()
{
    ui::BeginDisabled();
}

void osc::EndDisabled()
{
    ui::EndDisabled();
}
