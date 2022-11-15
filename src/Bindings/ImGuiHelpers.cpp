#include "ImGuiHelpers.hpp"

#include "src/Graphics/Camera.hpp"
#include "src/Graphics/RenderTexture.hpp"
#include "src/Graphics/Texture2D.hpp"
#include "src/Maths/CollisionTests.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/Utils/SynchronizedValue.hpp"
#include "src/Utils/UID.hpp"
#include "osc_config.hpp"

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
#include <cstdint>
#include <cstddef>
#include <string>

static inline constexpr float c_DefaultDragThreshold = 5.0f;

namespace
{
    template<typename Coll1, typename Coll2>
    float diff(Coll1 const& older, Coll2 const& newer, std::size_t n)
    {
        for (int i = 0; i < static_cast<int>(n); ++i)
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

    ImGui::GetStyle().FrameRounding = 2.0f;
    ImGui::GetStyle().GrabRounding = 20.0f;
    ImGui::GetStyle().GrabMinSize = 10.0f;

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
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
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

bool osc::UpdatePolarCameraFromImGuiUserInput(glm::vec2 viewportDims, osc::PolarPerspectiveCamera& camera)
{
    bool modified = false;

    // handle mousewheel scrolling
    if (ImGui::GetIO().MouseWheel != 0.0f)
    {
        camera.radius *= 1.0f - 0.1f * ImGui::GetIO().MouseWheel;
        modified = true;
    }

    // these camera controls try to be the union of OpenSim GUI and Blender
    //
    // left drag: drags/orbits camera (OpenSim GUI behavior)
    // left drag + L/R SHIFT: pans camera (CUSTOM behavior: can be handy on laptops where right-click + drag sucks)
    // left drag + L/R CTRL: zoom camera (CUSTOM behavior: can be handy on laptops where right-click + drag sucks)
    // middle drag: drags/orbits camera (Blender behavior)
    // middle drag + L/R SHIFT: pans camera (Blender behavior)
    // middle drag + L/R CTRL: zooms camera (Blender behavior)
    // right drag: pans camera (OpenSim GUI behavior)
    //
    // the reason it's like this is to please legacy OpenSim users *and*
    // users who use modelling software like Blender (which is more popular
    // among newer users looking to make new models)

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

void osc::DrawTextureAsImGuiImage(Texture2D& t, glm::vec2 dims)
{
    glm::vec2 const uv0 = {0.0f, 1.0f};
    glm::vec2 const uv1 = {1.0f, 0.0f};

    ImGui::Image(t.updTextureHandleHACK(), dims, uv0, uv1);
}

void osc::DrawTextureAsImGuiImage(RenderTexture& t, glm::vec2 dims)
{
    glm::vec2 const uv0 = {0.0f, 1.0f};
    glm::vec2 const uv1 = {1.0f, 0.0f};

    ImGui::Image(t.updTextureHandleHACK(), dims, uv0, uv1);
}

void osc::DrawTextureAsImGuiImage(RenderTexture& tex)
{
    return DrawTextureAsImGuiImage(tex, tex.getDimensions());
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

void osc::DrawTooltipBodyOnly(char const* text)
{
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(text);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

void osc::DrawTooltipBodyOnlyIfItemHovered(char const* text)
{
    if (ImGui::IsItemHovered())
    {
        DrawTooltipBodyOnly(text);
    }
}

void osc::DrawTooltip(char const* header, char const* description)
{
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(header);

    if (description)
    {
        ImGui::Dummy({0.0f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.7f, 0.7f, 0.7f, 1.0f});
        ImGui::TextUnformatted(description);
        ImGui::PopStyleColor();
    }

    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

void osc::DrawTooltipIfItemHovered(char const* header, char const* description)
{
    if (ImGui::IsItemHovered())
    {
        DrawTooltip(header, description);
    }
}

void osc::DrawAlignmentAxesOverlayInBottomRightOf(glm::mat4 const& viewMtx, Rect const& renderRect)
{
    float constexpr linelen = 35.0f;
    float const fontSize = ImGui::GetFontSize();
    float const circleRadius = fontSize/1.5f;
    float const padding = circleRadius + 3.0f;
    ImU32 const whiteColorU32 = ImGui::ColorConvertFloat4ToU32({1.0f, 1.0f, 1.0f, 1.0f});

    glm::vec2 const origin =
    {
        renderRect.p1.x + (linelen + padding),
        renderRect.p2.y - (linelen + padding),
    };

    std::array<char const* const, 3> labels = {"X", "Y", "Z"};

    ImDrawList& dd = *ImGui::GetWindowDrawList();
    for (size_t i = 0; i < labels.size(); ++i)
    {
        glm::vec4 world = {0.0f, 0.0f, 0.0f, 0.0f};
        world[static_cast<glm::vec4::length_type>(i)] = 1.0f;

        glm::vec2 view = glm::vec2{viewMtx * world};
        view.y = -view.y;  // y goes down in screen-space

        glm::vec2 const p1 = origin;
        glm::vec2 const p2 = origin + linelen*view;

        glm::vec4 color = {0.2f, 0.2f, 0.2f, 1.0f};
        color[static_cast<glm::vec4::length_type>(i)] = 0.7f;
        ImU32 const colorU32 = ImGui::ColorConvertFloat4ToU32(color);

        glm::vec2 const ts = ImGui::CalcTextSize(labels[i]);

        dd.AddLine(p1, p2, colorU32, 3.0f);
        dd.AddCircleFilled(p2, circleRadius, colorU32);
        dd.AddText(p2 - ts/2.0f, whiteColorU32, labels[i]);
    }
}

// draw a help text marker `"(?)"` and display a tooltip when the user hovers over it
void osc::DrawHelpMarker(char const* header, char const* desc)
{
    ImGui::TextDisabled("(?)");
    DrawTooltipIfItemHovered(header, desc);
}

// draw a help text marker `"(?)"` and display a tooltip when the user hovers over it
void osc::DrawHelpMarker(char const* desc)
{
    ImGui::TextDisabled("(?)");
    DrawTooltipIfItemHovered(desc);
}

bool osc::InputString(const char* label, std::string& s, std::size_t maxLen, ImGuiInputTextFlags flags)
{
    static SynchronizedValue<std::string> s_Buf;

    auto bufGuard = s_Buf.lock();

    *bufGuard = s;
    bufGuard->resize(std::max(maxLen, s.size()));
    (*bufGuard)[s.size()] = '\0';

    if (ImGui::InputText(label, bufGuard->data(), maxLen, flags))
    {
        s = bufGuard->data();
        return true;
    }
    else
    {
        return false;
    }
}

bool osc::DrawF3Editor(char const* lockID, char const* editorID, float* v, bool* isLocked)
{
    bool changed = false;

    ImGui::PushID(*lockID);
    if (ImGui::Button(*isLocked ? ICON_FA_LOCK : ICON_FA_UNLOCK))
    {
        *isLocked = !*isLocked;
        changed = true;
    }
    ImGui::PopID();

    ImGui::SameLine();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    float copy[3] = {v[0], v[1], v[2]};

    if (ImGui::InputFloat3(editorID, copy, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (*isLocked)
        {
            float val = diff(v, copy, 3);
            v[0] = val;
            v[1] = val;
            v[2] = val;
        }
        else
        {
            v[0] = copy[0];
            v[1] = copy[1];
            v[2] = copy[2];
        }
        changed = true;
    }

    return changed;
}

bool osc::InputMetersFloat(const char* label, float* v, float step, float step_fast, ImGuiInputTextFlags flags)
{
    return ImGui::InputFloat(label, v, step, step_fast, OSC_DEFAULT_FLOAT_INPUT_FORMAT, flags);
}

bool osc::InputMetersFloat3(const char* label, float v[3], ImGuiInputTextFlags flags)
{
    return ImGui::InputFloat3(label, v, OSC_DEFAULT_FLOAT_INPUT_FORMAT, flags);
}

bool osc::SliderMetersFloat(const char* label, float* v, float v_min, float v_max, ImGuiSliderFlags flags)
{
    return ImGui::SliderFloat(label, v, v_min, v_max, OSC_DEFAULT_FLOAT_INPUT_FORMAT, flags);
}

bool osc::InputKilogramFloat(const char* label, float* v, float step, float step_fast, ImGuiInputTextFlags flags)
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

bool osc::BeginMainViewportTopBar(char const* label, float height, ImGuiWindowFlags flags)
{
    // https://github.com/ocornut/imgui/issues/3518
    ImGuiViewportP* const viewport = static_cast<ImGuiViewportP*>(static_cast<void*>(ImGui::GetMainViewport()));
    return ImGui::BeginViewportSideBar(label, viewport, ImGuiDir_Up, height, flags);
}


bool osc::BeginMainViewportBottomBar(char const* label)
{
    // https://github.com/ocornut/imgui/issues/3518
    ImGuiViewportP* const viewport = static_cast<ImGuiViewportP*>(static_cast<void*>(ImGui::GetMainViewport()));
    ImGuiWindowFlags const flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
    float const height = ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y;

    return ImGui::BeginViewportSideBar(label, viewport, ImGuiDir_Down, height, flags);
}

void osc::TextCentered(std::string const& s)
{
    float const windowWidth = ImGui::GetWindowSize().x;
    float const textWidth   = ImGui::CalcTextSize(s.c_str()).x;

    ImGui::SetCursorPosX(0.5f * (windowWidth - textWidth));
    ImGui::TextUnformatted(s.c_str());
}
