#include "ImGuiHelpers.hpp"

#include "oscar/Graphics/Camera.hpp"
#include "oscar/Graphics/Color.hpp"
#include "oscar/Graphics/RenderTexture.hpp"
#include "oscar/Graphics/Texture2D.hpp"
#include "oscar/Maths/CollisionTests.hpp"
#include "oscar/Maths/Constants.hpp"
#include "oscar/Maths/MathHelpers.hpp"
#include "oscar/Maths/Rect.hpp"
#include "oscar/Maths/PolarPerspectiveCamera.hpp"
#include "oscar/Utils/ArrayHelpers.hpp"
#include "oscar/Utils/SynchronizedValue.hpp"
#include "oscar/Utils/UID.hpp"
#include "OscarConfiguration.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <nonstd/span.hpp>
#include <SDL_events.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

static inline float constexpr c_DefaultDragThreshold = 5.0f;

namespace
{
    template<typename TCollection, typename UCollection>
    float diff(TCollection const& older, UCollection const& newer, size_t n)
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
}

void osc::ImGuiApplyDarkTheme()
{
    // see: https://github.com/ocornut/imgui/issues/707
    // this one: https://github.com/ocornut/imgui/issues/707#issuecomment-512669512

    ImGui::GetStyle().FrameRounding = 0.0f;
    ImGui::GetStyle().GrabRounding = 20.0f;
    ImGui::GetStyle().GrabMinSize = 10.0f;

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
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
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
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
    osc::PolarPerspectiveCamera& camera,
    glm::vec2 viewportDims)
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
    glm::vec2 const delta = ImGui::GetIO().MouseDelta;

    if (delta != glm::vec2{0.0f, 0.0f} && (leftDragging || middleDragging))
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
    std::optional<osc::AABB> maybeSceneAABB)
{
    bool const shiftDown = osc::IsShiftDown();
    bool const ctrlOrSuperDown = osc::IsCtrlOrSuperDown();

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
                osc::AutoFocus(
                    camera,
                    *maybeSceneAABB,
                    osc::AspectRatio(viewportRect)
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
            osc::AutoFocus(
                camera,
                *maybeSceneAABB,
                osc::AspectRatio(viewportRect)
            );
            return true;
        }
    }
    else if (ImGui::IsKeyDown(ImGuiKey_UpArrow))
    {
        if (ctrlOrSuperDown)
        {
            // pan
            camera.pan(osc::AspectRatio(viewportRect), {0.0f, -0.1f});
        }
        else if (shiftDown)
        {
            // rotate in 90-deg increments
            camera.phi -= glm::radians(90.0f);
        }
        else
        {
            // rotate in 10-deg increments
            camera.phi -= glm::radians(10.0f);
        }
        return true;
    }
    else if (ImGui::IsKeyDown(ImGuiKey_DownArrow))
    {
        if (ctrlOrSuperDown)
        {
            // pan
            camera.pan(osc::AspectRatio(viewportRect), {0.0f, +0.1f});
        }
        else if (shiftDown)
        {
            // rotate in 90-deg increments
            camera.phi += glm::radians(90.0f);
        }
        else
        {
            // rotate in 10-deg increments
            camera.phi += glm::radians(10.0f);
        }
        return true;
    }
    else if (ImGui::IsKeyDown(ImGuiKey_LeftArrow))
    {
        if (ctrlOrSuperDown)
        {
            // pan
            camera.pan(osc::AspectRatio(viewportRect), {-0.1f, 0.0f});
        }
        else if (shiftDown)
        {
            // rotate in 90-deg increments
            camera.theta += glm::radians(90.0f);
        }
        else
        {
            // rotate in 10-deg increments
            camera.theta += glm::radians(10.0f);
        }
        return true;
    }
    else if (ImGui::IsKeyDown(ImGuiKey_RightArrow))
    {
        if (ctrlOrSuperDown)
        {
            // pan
            camera.pan(osc::AspectRatio(viewportRect), {+0.1f, 0.0f});
        }
        else if (shiftDown)
        {
            // rotate in 90-deg increments
            camera.theta -= glm::radians(90.0f);
        }
        else
        {
            // rotate in 10-deg increments
            camera.theta -= glm::radians(10.0f);
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
    std::optional<osc::AABB> maybeSceneAABB)
{

    ImGuiIO& io = ImGui::GetIO();

    // we don't check `io.WantCaptureMouse` because clicking/dragging on an ImGui::Image
    // is classed as a mouse interaction
    bool const mouseHandled =
        UpdatePolarCameraFromImGuiMouseInputs(camera, osc::Dimensions(viewportRect));
    bool const keyboardHandled = !io.WantCaptureKeyboard ?
        UpdatePolarCameraFromImGuiKeyboardInputs(camera, viewportRect, maybeSceneAABB) :
        false;

    return mouseHandled || keyboardHandled;
}

void osc::UpdateEulerCameraFromImGuiUserInput(Camera& camera, glm::vec3& eulers)
{
    glm::vec3 const front = camera.getDirection();
    glm::vec3 const up = camera.getUpwardsDirection();
    glm::vec3 const right = glm::cross(front, up);
    glm::vec2 const mouseDelta = ImGui::GetIO().MouseDelta;

    float const speed = 10.0f;
    float const displacement = speed * ImGui::GetIO().DeltaTime;
    float const sensitivity = 0.005f;

    // keyboard: changes camera position
    glm::vec3 pos = camera.getPosition();
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
    eulers.x = glm::clamp(eulers.x, -osc::fpi2 + 0.1f, osc::fpi2 - 0.1f);
    eulers.y += sensitivity * -mouseDelta.x;
    eulers.y = std::fmod(eulers.y, 2.0f * osc::fpi);

    camera.setRotation(glm::normalize(glm::quat{eulers}));
}

osc::Rect osc::ContentRegionAvailScreenRect()
{
    glm::vec2 const topLeft = ImGui::GetCursorScreenPos();
    glm::vec2 const dims = ImGui::GetContentRegionAvail();
    glm::vec2 const bottomRight = topLeft + dims;

    return Rect{topLeft, bottomRight};
}

void osc::DrawTextureAsImGuiImage(Texture2D const& t)
{
    DrawTextureAsImGuiImage(t, t.getDimensions());
}

void osc::DrawTextureAsImGuiImage(Texture2D const& t, glm::vec2 dims)
{
    glm::vec2 const topLeftCoord = {0.0f, 1.0f};
    glm::vec2 const bottomRightCoord = {1.0f, 0.0f};
    DrawTextureAsImGuiImage(t, dims, topLeftCoord, bottomRightCoord);
}

void osc::DrawTextureAsImGuiImage(
    Texture2D const& t,
    glm::vec2 dims,
    glm::vec2 topLeftCoord,
    glm::vec2 bottomRightCoord)
{
    ImGui::Image(t.getTextureHandleHACK(), dims, topLeftCoord, bottomRightCoord);
}

void osc::DrawTextureAsImGuiImage(RenderTexture const& tex)
{
    return DrawTextureAsImGuiImage(tex, tex.getDimensions());
}

void osc::DrawTextureAsImGuiImage(RenderTexture const& t, glm::vec2 dims)
{
    glm::vec2 const uv0 = {0.0f, 1.0f};
    glm::vec2 const uv1 = {1.0f, 0.0f};

    ImGui::Image(t.getTextureHandleHACK(), dims, uv0, uv1);
}

bool osc::ImageButton(
    CStringView label,
    Texture2D const& t,
    glm::vec2 dims,
    Rect const& textureCoords)
{
    return ImGui::ImageButton(label.c_str(), t.getTextureHandleHACK(), dims, textureCoords.p1, textureCoords.p2);
}

bool osc::ImageButton(CStringView label, Texture2D const& t, glm::vec2 dims)
{
    return ImageButton(label, t, dims, Rect{{0.0f, 1.0f}, {1.0f, 0.0f}});
}

osc::Rect osc::GetItemRect()
{
    return {ImGui::GetItemRectMin(), ImGui::GetItemRectMax()};
}

osc::ImGuiItemHittestResult osc::HittestLastImguiItem()
{
    return osc::HittestLastImguiItem(c_DefaultDragThreshold);
}

osc::ImGuiItemHittestResult osc::HittestLastImguiItem(float dragThreshold)
{
    ImGuiItemHittestResult rv;
    rv.rect.p1 = ImGui::GetItemRectMin();
    rv.rect.p2 = ImGui::GetItemRectMax();
    rv.isHovered = ImGui::IsItemHovered();
    rv.isLeftClickReleasedWithoutDragging = rv.isHovered && osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Left, dragThreshold);
    rv.isRightClickReleasedWithoutDragging = rv.isHovered && osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right, dragThreshold);
    return rv;
}

bool osc::IsAnyKeyDown(nonstd::span<ImGuiKey const> keys)
{
    for (ImGuiKey const key : keys)
    {
        if (ImGui::IsKeyDown(key))
        {
            return true;
        }
    }
    return false;
}

bool osc::IsAnyKeyDown(std::initializer_list<ImGuiKey const> keys)
{
    return IsAnyKeyDown(nonstd::span<ImGuiKey const>{keys.begin(), keys.end()});
}

bool osc::IsAnyKeyPressed(nonstd::span<ImGuiKey const> keys)
{
    for (ImGuiKey const key : keys)
    {
        if (ImGui::IsKeyPressed(key))
        {
            return true;
        }
    }
    return false;
}
bool osc::IsAnyKeyPressed(std::initializer_list<ImGuiKey const> keys)
{
    return IsAnyKeyPressed(nonstd::span<ImGuiKey const>{keys.begin(), keys.end()});
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
    return osc::IsMouseReleasedWithoutDragging(btn, c_DefaultDragThreshold);
}

bool osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton btn, float threshold)
{
    if (!ImGui::IsMouseReleased(btn))
    {
        return false;
    }

    glm::vec2 const dragDelta = ImGui::GetMouseDragDelta(btn);

    return glm::length(dragDelta) < threshold;
}

bool osc::IsDraggingWithAnyMouseButtonDown()
{
    return
        ImGui::IsMouseDragging(ImGuiMouseButton_Left) ||
        ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
        ImGui::IsMouseDragging(ImGuiMouseButton_Right);
}

void osc::DrawTooltipBodyOnly(CStringView label)
{
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(label.c_str());
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

void osc::DrawTooltipBodyOnlyIfItemHovered(CStringView label)
{
    if (ImGui::IsItemHovered())
    {
        DrawTooltipBodyOnly(label);
    }
}

void osc::DrawTooltip(CStringView header, CStringView description)
{
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(header.c_str());

    if (!description.empty())
    {
        ImGui::Dummy({0.0f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_Text, {0.7f, 0.7f, 0.7f, 1.0f});
        ImGui::TextUnformatted(description.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

void osc::DrawTooltipIfItemHovered(CStringView header, CStringView description)
{
    if (ImGui::IsItemHovered())
    {
        DrawTooltip(header, description);
    }
}

glm::vec2 osc::CalcAlignmentAxesDimensions()
{
    float const fontSize = ImGui::GetFontSize();
    float const linelen = 2.0f * fontSize;
    float const circleRadius = 0.6f * fontSize;
    float const edgeLen = 2.0f * (linelen + circleRadius);
    return {edgeLen, edgeLen};
}

osc::Rect osc::DrawAlignmentAxes(glm::mat4 const& viewMtx)
{
    float const fontSize = ImGui::GetFontSize();
    float const linelen = 2.0f * fontSize;
    float const circleRadius = 0.6f * fontSize;
    ImU32 const whiteColorU32 = ImGui::ColorConvertFloat4ToU32({1.0f, 1.0f, 1.0f, 1.0f});

    glm::vec2 const topLeft = ImGui::GetCursorScreenPos();
    glm::vec2 const bottomRight = topLeft + 2.0f*(linelen + circleRadius);
    Rect const bounds = {topLeft, bottomRight};
    glm::vec2 const origin = Midpoint(bounds);

    auto const labels = osc::MakeSizedArray<osc::CStringView, 3>("X", "Y", "Z");

    ImDrawList& drawlist = *ImGui::GetWindowDrawList();
    for (size_t i = 0; i < std::size(labels); ++i)
    {
        glm::vec4 world = {0.0f, 0.0f, 0.0f, 0.0f};
        world[static_cast<glm::vec4::length_type>(i)] = 1.0f;

        glm::vec2 view = glm::vec2{viewMtx * world};
        view.y = -view.y;  // y goes down in screen-space

        glm::vec2 const p1 = origin;
        glm::vec2 const p2 = origin + linelen*view;

        Color color = {0.15f, 0.15f, 0.15f, 1.0f};
        color[i] = 0.7f;
        ImU32 const colorU32 = ImGui::ColorConvertFloat4ToU32(glm::vec4{color});

        glm::vec2 const ts = ImGui::CalcTextSize(labels[i].c_str());

        drawlist.AddLine(p1, p2, colorU32, 3.0f);
        drawlist.AddCircleFilled(p2, circleRadius, colorU32);
        drawlist.AddText(p2 - 0.5f*ts, whiteColorU32, labels[i].c_str());

        // also, add a faded line for symmetry
        {
            color.a *= 0.15f;
            ImU32 const colorFadedU32 = ImGui::ColorConvertFloat4ToU32(glm::vec4{color});
            glm::vec2 const p2rev = origin - linelen*view;
            drawlist.AddLine(p1, p2rev, colorFadedU32, 3.0f);

        }
    }

    return bounds;
}

// draw a help text marker `"(?)"` and display a tooltip when the user hovers over it
void osc::DrawHelpMarker(CStringView header, CStringView desc)
{
    ImGui::TextDisabled("(?)");
    DrawTooltipIfItemHovered(header, desc);
}

// draw a help text marker `"(?)"` and display a tooltip when the user hovers over it
void osc::DrawHelpMarker(CStringView desc)
{
    ImGui::TextDisabled("(?)");
    DrawTooltipIfItemHovered(desc);
}

bool osc::InputString(CStringView label, std::string& s, size_t maxLen, ImGuiInputTextFlags flags)
{
    static SynchronizedValue<std::string> s_Buf;

    auto bufGuard = s_Buf.lock();

    *bufGuard = s;
    bufGuard->resize(std::max(maxLen, std::size(s)));
    (*bufGuard)[std::size(s)] = '\0';

    if (ImGui::InputText(label.c_str(), bufGuard->data(), maxLen, flags))
    {
        s = bufGuard->data();
        return true;
    }
    else
    {
        return false;
    }
}

bool osc::InputMetersFloat(CStringView label, float& v, float step, float step_fast, ImGuiInputTextFlags flags)
{
    return ImGui::InputFloat(label.c_str(), &v, step, step_fast, OSC_DEFAULT_FLOAT_INPUT_FORMAT, flags);
}

bool osc::InputMetersFloat3(CStringView label, glm::vec3& vec, ImGuiInputTextFlags flags)
{
    return ImGui::InputFloat3(label.c_str(), glm::value_ptr(vec), OSC_DEFAULT_FLOAT_INPUT_FORMAT, flags);
}

bool osc::SliderMetersFloat(CStringView label, float& v, float v_min, float v_max, ImGuiSliderFlags flags)
{
    return ImGui::SliderFloat(label.c_str(), &v, v_min, v_max, OSC_DEFAULT_FLOAT_INPUT_FORMAT, flags);
}

bool osc::InputKilogramFloat(CStringView label, float& v, float step, float step_fast, ImGuiInputTextFlags flags)
{
    return InputMetersFloat(label, v, step, step_fast, flags);
}

void osc::PushID(UID const& id)
{
    ImGui::PushID(static_cast<int>(id.get()));
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

osc::Rect osc::GetMainViewportWorkspaceScreenRect()
{
    ImGuiViewport const& viewport = *ImGui::GetMainViewport();

    return Rect
    {
        viewport.WorkPos,
        glm::vec2{viewport.WorkPos} + glm::vec2{viewport.WorkSize}
    };
}

bool osc::IsMouseInMainViewportWorkspaceScreenRect()
{
    glm::vec2 const mousepos = ImGui::GetIO().MousePos;
    osc::Rect const hitRect = osc::GetMainViewportWorkspaceScreenRect();

    return osc::IsPointInRect(hitRect, mousepos);
}

bool osc::BeginMainViewportTopBar(CStringView label, float height, ImGuiWindowFlags flags)
{
    // https://github.com/ocornut/imgui/issues/3518
    ImGuiViewportP* const viewport = static_cast<ImGuiViewportP*>(static_cast<void*>(ImGui::GetMainViewport()));
    return ImGui::BeginViewportSideBar(label.c_str(), viewport, ImGuiDir_Up, height, flags);
}


bool osc::BeginMainViewportBottomBar(CStringView label)
{
    // https://github.com/ocornut/imgui/issues/3518
    ImGuiViewportP* const viewport = static_cast<ImGuiViewportP*>(static_cast<void*>(ImGui::GetMainViewport()));
    ImGuiWindowFlags const flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
    float const height = ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y;

    return ImGui::BeginViewportSideBar(label.c_str(), viewport, ImGuiDir_Down, height, flags);
}

void osc::TextCentered(CStringView s)
{
    float const windowWidth = ImGui::GetWindowSize().x;
    float const textWidth   = ImGui::CalcTextSize(s.c_str()).x;

    ImGui::SetCursorPosX(0.5f * (windowWidth - textWidth));
    ImGui::TextUnformatted(s.c_str());
}

bool osc::ItemValueShouldBeSaved()
{
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        return true;  // ImGui detected that the item was deactivated after an edit
    }

    if (ImGui::IsItemEdited() && osc::IsAnyKeyPressed({ImGuiKey_Enter, ImGuiKey_Tab}))
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
    nonstd::span<CStringView const> items)
{
    char const* preview = *current < items.size() ?
        items[*current].c_str() :
        nullptr;

    if (!ImGui::BeginCombo(label.c_str(), preview, ImGuiComboFlags_None))
    {
        return false;
    }

    bool changed = false;
    for (size_t i = 0; i < items.size(); ++i)
    {
        ImGui::PushID(static_cast<int>(i));
        bool const isSelected = i == *current;
        if (ImGui::Selectable(items[i].c_str(), isSelected))
        {
            changed = true;
            *current = i;
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

// create a lookup table that maps sRGB color bytes to linear-space color bytes
static std::array<uint8_t, 256> CreateSRGBToLinearLUT()
{
    std::array<uint8_t, 256> rv;
    for (size_t i = 0; i < 256; ++i)
    {
        rv[i] = static_cast<uint8_t>(255.0f*osc::ToLinear(static_cast<float>(i)/255.0f));
    }
    return rv;
}

static std::array<uint8_t, 256> const& GetSRGBToLinearLUT()
{
    static std::array<uint8_t, 256> s_LUT = CreateSRGBToLinearLUT();
    return s_LUT;
}

void osc::ConvertDrawDataFromSRGBToLinear(ImDrawData& dd)
{
    std::array<uint8_t, 256> const& lut = GetSRGBToLinearLUT();

    for (int i = 0; i < dd.CmdListsCount; ++i)
    {
        for (ImDrawVert& v : dd.CmdLists[i]->VtxBuffer)
        {
            uint8_t const rSRGB = static_cast<uint8_t>((v.col >> IM_COL32_R_SHIFT) & 0xFF);
            uint8_t const gSRGB = static_cast<uint8_t>((v.col >> IM_COL32_G_SHIFT) & 0xFF);
            uint8_t const bSRGB = static_cast<uint8_t>((v.col >> IM_COL32_B_SHIFT) & 0xFF);
            uint8_t const aSRGB = static_cast<uint8_t>((v.col >> IM_COL32_A_SHIFT) & 0xFF);

            uint8_t const rLinear = lut[rSRGB];
            uint8_t const gLinear = lut[gSRGB];
            uint8_t const bLinear = lut[bSRGB];

            v.col =
                static_cast<ImU32>(rLinear) << IM_COL32_R_SHIFT |
                static_cast<ImU32>(gLinear) << IM_COL32_G_SHIFT |
                static_cast<ImU32>(bLinear) << IM_COL32_B_SHIFT |
                static_cast<ImU32>(aSRGB) << IM_COL32_A_SHIFT;
        }
    }
}
