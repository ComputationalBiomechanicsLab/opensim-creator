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
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/UID.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <span>
#include <string>

using namespace osc::literals;
using namespace osc;
namespace rgs = std::ranges;

namespace
{
    inline constexpr float c_default_drag_threshold = 5.0f;

    template<
        rgs::random_access_range TCollection,
        rgs::random_access_range UCollection
    >
    requires
        std::convertible_to<typename TCollection::value_type, float> and
        std::convertible_to<typename UCollection::value_type, float>
    float diff(const TCollection& older, const UCollection& newer, size_t n)
    {
        for (size_t i = 0; i < n; ++i) {
            if (static_cast<float>(older[i]) != static_cast<float>(newer[i])) {
                return newer[i];
            }
        }
        return static_cast<float>(older[0]);
    }

    Vec2 centroid_of(const ImRect& r)
    {
        return 0.5f * (Vec2{r.Min} + Vec2{r.Max});
    }

    Vec2 dimensions_of(const ImRect& r)
    {
        return Vec2{r.Max} - Vec2{r.Min};
    }

    float ShortestEdgeLength(const ImRect& r)
    {
        const Vec2 sz = dimensions_of(r);
        return min(sz.x, sz.y);
    }

    ImU32 Brighten(ImU32 color, float factor)
    {
        const Color srgb = ui::to_color(color);
        const Color brightened = factor * srgb;
        const Color clamped = clamp_to_ldr(brightened);
        return ui::ToImU32(clamped);
    }
}

void osc::ui::ApplyDarkTheme()
{
    // see: https://github.com/ocornut/imgui/issues/707
    // this one: https://github.com/ocornut/imgui/issues/707#issuecomment-512669512

    auto& style = ui::GetStyle();

    style.FrameRounding = 0.0f;
    style.GrabRounding = 20.0f;
    style.GrabMinSize = 10.0f;

    auto& colors = style.Colors;
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

bool osc::ui::UpdatePolarCameraFromMouseInputs(
    PolarPerspectiveCamera& camera,
    Vec2 viewport_dimensions)
{
    bool modified = false;

    // handle mousewheel scrolling
    if (ui::GetIO().MouseWheel != 0.0f)
    {
        camera.radius *= 1.0f - 0.1f * ui::GetIO().MouseWheel;
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

    const float aspectRatio = viewport_dimensions.x / viewport_dimensions.y;

    const bool leftDragging = ui::IsMouseDragging(ImGuiMouseButton_Left);
    const bool middleDragging = ui::IsMouseDragging(ImGuiMouseButton_Middle);
    const Vec2 delta = ui::GetIO().MouseDelta;

    if (delta != Vec2{0.0f, 0.0f} && (leftDragging || middleDragging)) {
        if (IsCtrlDown()) {
            camera.pan(aspectRatio, delta/viewport_dimensions);
            modified = true;
        }
        else if (IsCtrlOrSuperDown()) {
            camera.radius *= 1.0f + 4.0f*delta.y/viewport_dimensions.y;
            modified = true;
        }
        else {
            camera.drag(delta/viewport_dimensions);
            modified = true;
        }
    }
    else if (ui::IsMouseDragging(ImGuiMouseButton_Right)) {
        if (IsAltDown()) {
            camera.radius *= 1.0f + 4.0f*delta.y/viewport_dimensions.y;
            modified = true;
        }
        else {
            camera.pan(aspectRatio, delta/viewport_dimensions);
            modified = true;
        }
    }

    if (modified) {
        camera.rescale_znear_and_zfar_based_on_radius();
    }

    return modified;
}

bool osc::ui::UpdatePolarCameraFromKeyboardInputs(
    PolarPerspectiveCamera& camera,
    const Rect& viewport_rect,
    std::optional<AABB> maybe_scene_aabb)
{
    const bool shiftDown = IsShiftDown();
    const bool ctrlOrSuperDown = IsCtrlOrSuperDown();

    if (ui::IsKeyReleased(ImGuiKey_X)) {
        if (ctrlOrSuperDown) {
            focus_along_minus_x(camera);
            return true;
        } else {
            focus_along_x(camera);
            return true;
        }
    }
    else if (ui::IsKeyPressed(ImGuiKey_Y)) {
        // Ctrl+Y already does something?
        if (!ctrlOrSuperDown) {
            focus_along_y(camera);
            return true;
        }
    }
    else if (ui::IsKeyPressed(ImGuiKey_F)) {
        if (ctrlOrSuperDown) {
            if (maybe_scene_aabb) {
                auto_focus(
                    camera,
                    *maybe_scene_aabb,
                    aspect_ratio_of(viewport_rect)
                );
                return true;
            }
        }
        else {
            reset(camera);
            return true;
        }
    }
    else if (ctrlOrSuperDown && (ui::IsKeyPressed(ImGuiKey_8))) {
        if (maybe_scene_aabb) {
            auto_focus(
                camera,
                *maybe_scene_aabb,
                aspect_ratio_of(viewport_rect)
            );
            return true;
        }
    }
    else if (ui::IsKeyDown(ImGuiKey_UpArrow)) {
        if (ctrlOrSuperDown) {
            // pan
            camera.pan(aspect_ratio_of(viewport_rect), {0.0f, -0.1f});
        }
        else if (shiftDown) {
            camera.phi -= 90_deg;  // rotate in 90-deg increments
        }
        else {
            camera.phi -= 10_deg;  // rotate in 10-deg increments
        }
        return true;
    }
    else if (ui::IsKeyDown(ImGuiKey_DownArrow)) {
        if (ctrlOrSuperDown) {
            // pan
            camera.pan(aspect_ratio_of(viewport_rect), {0.0f, +0.1f});
        }
        else if (shiftDown) {
            // rotate in 90-deg increments
            camera.phi += 90_deg;
        }
        else {
            // rotate in 10-deg increments
            camera.phi += 10_deg;
        }
        return true;
    }
    else if (ui::IsKeyDown(ImGuiKey_LeftArrow)) {
        if (ctrlOrSuperDown) {
            // pan
            camera.pan(aspect_ratio_of(viewport_rect), {-0.1f, 0.0f});
        }
        else if (shiftDown) {
            // rotate in 90-deg increments
            camera.theta += 90_deg;
        }
        else {
            // rotate in 10-deg increments
            camera.theta += 10_deg;
        }
        return true;
    }
    else if (ui::IsKeyDown(ImGuiKey_RightArrow)) {
        if (ctrlOrSuperDown) {
            // pan
            camera.pan(aspect_ratio_of(viewport_rect), {+0.1f, 0.0f});
        }
        else if (shiftDown) {
            camera.theta -= 90_deg;  // rotate in 90-deg increments
        }
        else {
            camera.theta -= 10_deg;  // rotate in 10-deg increments
        }
        return true;
    }
    else if (ui::IsKeyDown(ImGuiKey_Minus)) {
        camera.radius *= 1.1f;
        return true;
    }
    else if (ui::IsKeyDown(ImGuiKey_Equal)) {
        camera.radius *= 0.9f;
        return true;
    }
    return false;
}

bool osc::ui::UpdatePolarCameraFromInputs(
    PolarPerspectiveCamera& camera,
    const Rect& viewport_rect,
    std::optional<AABB> maybe_scene_aabb)
{

    ImGuiIO& io = ui::GetIO();

    // we don't check `io.WantCaptureMouse` because clicking/dragging on an ImGui::Image
    // is classed as a mouse interaction
    const bool mouseHandled =
        UpdatePolarCameraFromMouseInputs(camera, dimensions_of(viewport_rect));
    const bool keyboardHandled = !io.WantCaptureKeyboard ?
        UpdatePolarCameraFromKeyboardInputs(camera, viewport_rect, maybe_scene_aabb) :
        false;

    return mouseHandled || keyboardHandled;
}

void osc::ui::UpdateCameraFromInputs(Camera& camera, Eulers& eulers)
{
    const Vec3 front = camera.direction();
    const Vec3 up = camera.upwards_direction();
    const Vec3 right = cross(front, up);
    const Vec2 mouseDelta = ui::GetIO().MouseDelta;

    const float speed = 10.0f;
    const float displacement = speed * ui::GetIO().DeltaTime;
    const Radians sensitivity{0.005f};

    // keyboard: changes camera position
    Vec3 pos = camera.position();
    if (ui::IsKeyDown(ImGuiKey_W))
    {
        pos += displacement * front;
    }
    if (ui::IsKeyDown(ImGuiKey_S))
    {
        pos -= displacement * front;
    }
    if (ui::IsKeyDown(ImGuiKey_A))
    {
        pos -= displacement * right;
    }
    if (ui::IsKeyDown(ImGuiKey_D))
    {
        pos += displacement * right;
    }
    if (ui::IsKeyDown(ImGuiKey_Space))
    {
        pos += displacement * up;
    }
    if (ui::GetIO().KeyCtrl)
    {
        pos -= displacement * up;
    }
    camera.set_position(pos);

    eulers.x += sensitivity * -mouseDelta.y;
    eulers.x = clamp(eulers.x, -90_deg + 0.1_rad, 90_deg - 0.1_rad);
    eulers.y += sensitivity * -mouseDelta.x;
    eulers.y = mod(eulers.y, 360_deg);

    camera.set_rotation(to_worldspace_rotation_quat(eulers));
}

Rect osc::ui::ContentRegionAvailScreenRect()
{
    const Vec2 topLeft = ui::GetCursorScreenPos();
    return Rect{topLeft, topLeft + ui::GetContentRegionAvail()};
}

void osc::ui::Image(const Texture2D& t)
{
    Image(t, t.dimensions());
}

void osc::ui::Image(const Texture2D& t, Vec2 dimensions)
{
    const Vec2 topLeftCoord = {0.0f, 1.0f};
    const Vec2 bottomRightCoord = {1.0f, 0.0f};
    Image(t, dimensions, topLeftCoord, bottomRightCoord);
}

void osc::ui::Image(
    const Texture2D& t,
    Vec2 dimensions,
    Vec2 top_left_texture_coordinate,
    Vec2 bottom_right_texture_coordinate)
{
    const auto handle = ui::graphics_backend::allocate_texture_for_current_frame(t);
    ImGui::Image(handle, dimensions, top_left_texture_coordinate, bottom_right_texture_coordinate);
}

void osc::ui::Image(const RenderTexture& tex)
{
    return Image(tex, tex.dimensions());
}

void osc::ui::Image(const RenderTexture& t, Vec2 dimensions)
{
    const Vec2 uv0 = {0.0f, 1.0f};
    const Vec2 uv1 = {1.0f, 0.0f};
    const auto handle = ui::graphics_backend::allocate_texture_for_current_frame(t);
    ImGui::Image(handle, dimensions, uv0, uv1);
}

Vec2 osc::ui::CalcButtonSize(CStringView content)
{
    return ui::CalcTextSize(content) + 2.0f*ui::GetStyleFramePadding();
}

float osc::ui::CalcButtonWidth(CStringView content)
{
    return CalcButtonSize(content).x;
}

bool osc::ui::ButtonNoBg(CStringView label, Vec2 dimensions)
{
    PushStyleColor(ImGuiCol_Button, Color::clear());
    PushStyleColor(ImGuiCol_ButtonHovered, Color::clear());
    const bool rv = ui::Button(label, dimensions);
    PopStyleColor();
    PopStyleColor();

    return rv;
}

bool osc::ui::ImageButton(
    CStringView label,
    const Texture2D& t,
    Vec2 dimensions,
    const Rect& texture_coordinates)
{
    const auto handle = ui::graphics_backend::allocate_texture_for_current_frame(t);
    return ImGui::ImageButton(label.c_str(), handle, dimensions, texture_coordinates.p1, texture_coordinates.p2);
}

bool osc::ui::ImageButton(CStringView label, const Texture2D& t, Vec2 dimensions)
{
    return ImageButton(label, t, dimensions, Rect{{0.0f, 1.0f}, {1.0f, 0.0f}});
}

Rect osc::ui::GetItemRect()
{
    return {ui::GetItemRectMin(), ui::GetItemRectMax()};
}

ui::HittestResult osc::ui::HittestLastItem()
{
    return HittestLastItem(c_default_drag_threshold);
}

ui::HittestResult osc::ui::HittestLastItem(float drag_threshold)
{
    HittestResult rv;
    rv.item_rect.p1 = ui::GetItemRectMin();
    rv.item_rect.p2 = ui::GetItemRectMax();
    rv.is_hovered = ui::IsItemHovered();
    rv.is_left_click_released_without_dragging = rv.is_hovered && IsMouseReleasedWithoutDragging(ImGuiMouseButton_Left, drag_threshold);
    rv.is_right_click_released_without_dragging = rv.is_hovered && IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right, drag_threshold);
    return rv;
}

bool osc::ui::IsAnyKeyDown(std::span<const ImGuiKey> keys)
{
    return rgs::any_of(keys, ui::IsKeyDown);
}

bool osc::ui::IsAnyKeyDown(std::initializer_list<const ImGuiKey> keys)
{
    return IsAnyKeyDown(std::span<ImGuiKey const>{keys.begin(), keys.end()});
}

bool osc::ui::IsAnyKeyPressed(std::span<const ImGuiKey> keys)
{
    return rgs::any_of(keys, [](ImGuiKey k) { return ui::IsKeyPressed(k); });
}
bool osc::ui::IsAnyKeyPressed(std::initializer_list<const ImGuiKey> keys)
{
    return IsAnyKeyPressed(std::span<const ImGuiKey>{keys.begin(), keys.end()});
}

bool osc::ui::IsCtrlDown()
{
    return ui::GetIO().KeyCtrl;
}

bool osc::ui::IsCtrlOrSuperDown()
{
    return ui::GetIO().KeyCtrl || ui::GetIO().KeySuper;
}

bool osc::ui::IsShiftDown()
{
    return ui::GetIO().KeyShift;
}

bool osc::ui::IsAltDown()
{
    return ui::GetIO().KeyAlt;
}

bool osc::ui::IsMouseReleasedWithoutDragging(ImGuiMouseButton btn)
{
    return IsMouseReleasedWithoutDragging(btn, c_default_drag_threshold);
}

bool osc::ui::IsMouseReleasedWithoutDragging(ImGuiMouseButton btn, float threshold)
{
    if (!ui::IsMouseReleased(btn))
    {
        return false;
    }

    const Vec2 dragDelta = ImGui::GetMouseDragDelta(btn);

    return length(dragDelta) < threshold;
}

bool osc::ui::IsDraggingWithAnyMouseButtonDown()
{
    return
        ui::IsMouseDragging(ImGuiMouseButton_Left) ||
        ui::IsMouseDragging(ImGuiMouseButton_Middle) ||
        ui::IsMouseDragging(ImGuiMouseButton_Right);
}

void osc::ui::BeginTooltip(std::optional<float> wrap_width)
{
    BeginTooltipNoWrap();
    ImGui::PushTextWrapPos(wrap_width.value_or(ImGui::GetFontSize() * 35.0f));
}

void osc::ui::EndTooltip(std::optional<float>)
{
    ImGui::PopTextWrapPos();
    EndTooltipNoWrap();
}

void osc::ui::TooltipHeaderText(CStringView s)
{
    TextUnformatted(s);
}

void osc::ui::TooltipDescriptionSpacer()
{
    ui::Dummy({0.0f, 1.0f});
}

void osc::ui::TooltipDescriptionText(CStringView s)
{
    TextFaded(s);
}

void osc::ui::DrawTooltipBodyOnly(CStringView label)
{
    BeginTooltip();
    TooltipHeaderText(label);
    EndTooltip();
}

void osc::ui::DrawTooltipBodyOnlyIfItemHovered(
    CStringView label,
    ImGuiHoveredFlags flags)
{
    if (ui::IsItemHovered(flags))
    {
        DrawTooltipBodyOnly(label);
    }
}

void osc::ui::DrawTooltip(CStringView header, CStringView description)
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

void osc::ui::DrawTooltipIfItemHovered(
    CStringView header,
    CStringView description,
    ImGuiHoveredFlags flags)
{
    if (ui::IsItemHovered(flags))
    {
        DrawTooltip(header, description);
    }
}

// draw a help text marker `"(?)"` and display a tooltip when the user hovers over it
void osc::ui::DrawHelpMarker(CStringView header, CStringView desc)
{
    ui::TextDisabled("(?)");
    DrawTooltipIfItemHovered(header, desc, ImGuiHoveredFlags_None);
}

// draw a help text marker `"(?)"` and display a tooltip when the user hovers over it
void osc::ui::DrawHelpMarker(CStringView desc)
{
    ui::TextDisabled("(?)");
    DrawTooltipIfItemHovered(desc, {}, ImGuiHoveredFlags_None);
}

bool osc::ui::InputString(CStringView label, std::string& edited_string, ImGuiInputTextFlags flags)
{
    return ImGui::InputText(label.c_str(), &edited_string, flags);  // uses `imgui_stdlib`
}

bool osc::ui::InputMetersFloat(CStringView label, float& v, float step, float step_fast, ImGuiInputTextFlags flags)
{
    return ui::InputFloat(label, &v, step, step_fast, "%.6f", flags);
}

bool osc::ui::InputMetersFloat3(CStringView label, Vec3& vec, ImGuiInputTextFlags flags)
{
    return ui::InputFloat3(label, value_ptr(vec), "%.6f", flags);
}

bool osc::ui::SliderMetersFloat(CStringView label, float& v, float v_min, float v_max, ImGuiSliderFlags flags)
{
    return ui::SliderFloat(label, &v, v_min, v_max, "%.6f", flags);
}

bool osc::ui::InputKilogramFloat(CStringView label, float& v, float step, float step_fast, ImGuiInputTextFlags flags)
{
    return InputMetersFloat(label, v, step, step_fast, flags);
}

bool osc::ui::InputAngle(CStringView label, Radians& v)
{
    float dv = Degrees{v}.count();
    if (ui::InputFloat(label, &dv))
    {
        v = Degrees{dv};
        return true;
    }
    return false;
}

bool osc::ui::InputAngle3(
    CStringView label,
    Vec<3, Radians>& vs,
    CStringView format)
{
    Vec3 dvs = {Degrees{vs.x}.count(), Degrees{vs.y}.count(), Degrees{vs.z}.count()};
    if (ui::InputVec3(label, dvs, format.c_str()))
    {
        vs = Vec<3, Degrees>{dvs};
        return true;
    }
    return false;
}

bool osc::ui::SliderAngle(CStringView label, Radians& v, Radians min, Radians max)
{
    float dv = Degrees{v}.count();
    const Degrees dmin{min};
    const Degrees dmax{max};
    if (ui::SliderFloat(label, &dv, dmin.count(), dmax.count())) {
        v = Degrees{dv};
        return true;
    }
    return false;
}

ImU32 osc::ui::ToImU32(const Color& color)
{
    return ui::ColorConvertFloat4ToU32(Vec4{color});
}

Color osc::ui::to_color(ImU32 u32color)
{
    return Color{Vec4{ImGui::ColorConvertU32ToFloat4(u32color)}};
}

Color osc::ui::to_color(const ImVec4& v)
{
    return {v.x, v.y, v.z, v.w};
}

ImVec4 osc::ui::ToImVec4(const Color& color)
{
    return ImVec4{Vec4{color}};
}

ImGuiWindowFlags osc::ui::GetMinimalWindowFlags()
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

Rect osc::ui::GetMainViewportWorkspaceScreenRect()
{
    const ImGuiViewport& viewport = *ui::GetMainViewport();

    return Rect{
        viewport.WorkPos,
        Vec2{viewport.WorkPos} + Vec2{viewport.WorkSize}
    };
}

bool osc::ui::IsMouseInMainViewportWorkspaceScreenRect()
{
    const Vec2 mousepos = ui::GetMousePos();
    const Rect hitRect = GetMainViewportWorkspaceScreenRect();

    return is_intersecting(hitRect, mousepos);
}

bool osc::ui::BeginMainViewportTopBar(CStringView label, float height, ImGuiWindowFlags flags)
{
    // https://github.com/ocornut/imgui/issues/3518
    auto* const viewport = static_cast<ImGuiViewportP*>(static_cast<void*>(ui::GetMainViewport()));
    return ImGui::BeginViewportSideBar(label.c_str(), viewport, ImGuiDir_Up, height, flags);
}


bool osc::ui::BeginMainViewportBottomBar(CStringView label)
{
    // https://github.com/ocornut/imgui/issues/3518
    auto* const viewport = static_cast<ImGuiViewportP*>(static_cast<void*>(ui::GetMainViewport()));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
    const float height = ui::GetFrameHeight() + ui::GetStyleWindowPadding().y;

    return ImGui::BeginViewportSideBar(label.c_str(), viewport, ImGuiDir_Down, height, flags);
}

bool osc::ui::ButtonCentered(CStringView s)
{
    const float buttonWidth = ui::CalcTextSize(s).x + 2.0f*ui::GetStyleFramePadding().x;
    const float midpoint = ui::GetCursorScreenPos().x + 0.5f*ui::GetContentRegionAvail().x;
    const float buttonStartX = midpoint - 0.5f*buttonWidth;

    ui::SetCursorScreenPos({buttonStartX, ui::GetCursorScreenPos().y});

    return ui::Button(s);
}

void osc::ui::TextCentered(CStringView s)
{
    const float windowWidth = ImGui::GetWindowSize().x;
    const float textWidth   = ui::CalcTextSize(s).x;

    ui::SetCursorPosX(0.5f * (windowWidth - textWidth));
    TextUnformatted(s);
}

void osc::ui::TextWindowCentered(CStringView s)
{
    const auto windowDimensions = ui::GetWindowSize();
    const auto textDimensions = ui::CalcTextSize(s);

    ui::SetCursorPos(0.5f * (windowDimensions - textDimensions));
    TextUnformatted(s);
}

void osc::ui::TextDisabledAndCentered(CStringView s)
{
    ui::BeginDisabled();
    TextCentered(s);
    ui::EndDisabled();
}

void osc::ui::TextDisabledAndWindowCentered(CStringView s)
{
    ui::BeginDisabled();
    TextWindowCentered(s);
    ui::EndDisabled();
}

void osc::ui::TextColumnCentered(CStringView s)
{
    const float columnWidth = ui::GetColumnWidth();
    const float columnOffset = ui::GetCursorPos().x;
    const float textWidth = ui::CalcTextSize(s).x;

    ui::SetCursorPosX(columnOffset + 0.5f*(columnWidth-textWidth));
    TextUnformatted(s);
}

void osc::ui::TextFaded(CStringView s)
{
    ui::PushStyleColor(ImGuiCol_Text, Color{0.7f, 0.7f, 0.7f});
    TextUnformatted(s);
    ui::PopStyleColor();
}

void osc::ui::TextWarning(CStringView s)
{
    PushStyleColor(ImGuiCol_Text, Color::yellow());
    TextUnformatted(s);
    PopStyleColor();
}

bool osc::ui::ItemValueShouldBeSaved()
{
    if (ui::IsItemDeactivatedAfterEdit())
    {
        return true;  // ImGui detected that the item was deactivated after an edit
    }

    if (ImGui::IsItemEdited() && IsAnyKeyPressed({ImGuiKey_Enter, ImGuiKey_Tab}))
    {
        return true;  // user pressed enter/tab after editing
    }

    return false;
}

void osc::ui::PopItemFlags(int n)
{
    for (int i = 0; i < n; ++i)
    {
        ImGui::PopItemFlag();
    }
}

bool osc::ui::Combo(
    CStringView label,
    size_t* current,
    size_t size,
    const std::function<CStringView(size_t)>& accessor)
{
    const CStringView preview = current != nullptr ?
        accessor(*current) :
        CStringView{};

    if (!ui::BeginCombo(label, preview, ImGuiComboFlags_None))
    {
        return false;
    }

    bool changed = false;
    for (size_t i = 0; i < size; ++i)
    {
        ui::PushID(static_cast<int>(i));
        const bool isSelected = current != nullptr && *current == i;
        if (ui::Selectable(accessor(i), isSelected))
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
        ui::PopID();
    }

    ui::EndCombo();

    if (changed)
    {
        ImGui::MarkItemEdited(ImGui::GetCurrentContext()->LastItemData.ID);
    }

    return changed;
}

bool osc::ui::Combo(
    CStringView label,
    size_t* current,
    std::span<const CStringView> items)
{
    return Combo(
        label,
        current,
        items.size(),
        [items](size_t i) { return items[i]; }
    );
}

void osc::ui::VerticalSeperator()
{
    ui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
}

void osc::ui::SameLineWithVerticalSeperator()
{
    ui::SameLine();
    VerticalSeperator();
    ui::SameLine();
}

bool osc::ui::CircularSliderFloat(
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
    const Vec2 labelSize = ui::CalcTextSize(label, true);
    const Vec2 frameDims = {ImGui::CalcItemWidth(), labelSize.y + 2.0f*style.FramePadding.y};
    const Vec2 cursorScreenPos = ui::GetCursorScreenPos();
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
            shouldClampTextualInput ? static_cast<const void*>(&min) : nullptr,
            shouldClampTextualInput ? static_cast<const void*>(&max) : nullptr
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
    const bool useCustomRendering = true;
    if (useCustomRendering)
    {
        const Vec2 sliderNobCenter = ::centroid_of(grabBoundingBox);
        const float sliderNobRadius = 0.75f * ShortestEdgeLength(grabBoundingBox);
        const float sliderRailThickness = 0.5f * sliderNobRadius;
        const float sliderRailTopY = sliderNobCenter.y - 0.5f*sliderRailThickness;
        const float sliderRailBottomY = sliderNobCenter.y + 0.5f*sliderRailThickness;

        const bool isActive = g.ActiveId == id;
        const ImU32 railColor = ui::GetColorU32(isHovered ? ImGuiCol_FrameBgHovered : isActive ? ImGuiCol_FrameBgActive : ImGuiCol_FrameBg);
        const ImU32 grabColor = ui::GetColorU32(isActive ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab);

        // render left-hand rail (brighter)
        {
            const Vec2 lhsRailTopLeft = {frameBB.Min.x, sliderRailTopY};
            const Vec2 lhsRailBottomright = {sliderNobCenter.x, sliderRailBottomY};
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
            const Vec2 rhsRailTopLeft = {sliderNobCenter.x, sliderRailTopY};
            const Vec2 rhsRailBottomRight = {frameBB.Max.x, sliderRailBottomY};

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
            const ImU32 frameColor = ui::GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : isHovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
            ImGui::RenderNavHighlight(frameBB, id);
            ImGui::RenderFrame(frameBB.Min, frameBB.Max, frameColor, true, g.Style.FrameRounding);
        }

        // render slider grab handle
        if (grabBoundingBox.Max.x > grabBoundingBox.Min.x)
        {
            window->DrawList->AddRectFilled(grabBoundingBox.Min, grabBoundingBox.Max, ui::GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);
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
