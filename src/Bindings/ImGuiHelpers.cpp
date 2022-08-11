#include "ImGuiHelpers.hpp"

#include "src/Graphics/Renderer.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Geometry.hpp"
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
#include <cstdint>
#include <cstddef>
#include <string>

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

void osc::UpdatePolarCameraFromImGuiUserInput(glm::vec2 viewportDims, osc::PolarPerspectiveCamera& camera)
{
    using osc::operator<<;

    // handle mousewheel scrolling
    camera.radius *= 1.0f - ImGui::GetIO().MouseWheel/10.0f;
    camera.rescaleZNearAndZFarBasedOnRadius();

    // these camera controls try to be the union of OpenSim and Blender
    //
    // left drag: drags/orbits camera (OpenSim behavior)
    // left drag + L/R SHIFT: pans camera (CUSTOM behavior: can be handy on laptops where right-click + drag sucks)
    // left drag + L/R CTRL: zoom camera (CUSTOM behavior: can be handy on laptops where right-click + drag sucks)
    // middle drag: drags/orbits camera (Blender behavior)
    // middle drag + L/R SHIFT: pans camera (Blender behavior)
    // middle drag + L/R CTRL: zooms camera (Blender behavior)
    // right drag: pans camera (OpenSim behavior)
    //
    // the reason it's like this is to please legacy OpenSim users *and*
    // users who use modelling software like Blender (which is more popular
    // among newer users looking to make new models)

    float aspectRatio = viewportDims.x / viewportDims.y;

    bool leftDragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
    bool middleDragging = ImGui::IsMouseDragging(ImGuiMouseButton_Middle);

    glm::vec2 delta = ImGui::GetIO().MouseDelta;

    if (leftDragging || middleDragging)
    {
        if (IsCtrlDown())
        {
            camera.pan(aspectRatio, delta/viewportDims);
        }
        else if (IsCtrlOrSuperDown())
        {
            camera.radius *= 1.0f + 4.0f*delta.y/viewportDims.y;
        }
        else
        {
            camera.drag(delta/viewportDims);
        }

    }
    else if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    {
        if (IsAltDown())
        {
            camera.radius *= 1.0f + 4.0f*delta.y/viewportDims.y;
        }
        else
        {
            camera.pan(aspectRatio, delta/viewportDims);
        }
    }
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
    if (ImGui::IsKeyDown(SDL_SCANCODE_W))
    {
        pos += displacement * front;
    }
    if (ImGui::IsKeyDown(SDL_SCANCODE_S))
    {
        pos -= displacement * front;
    }
    if (ImGui::IsKeyDown(SDL_SCANCODE_A))
    {
        pos -= displacement * right;
    }
    if (ImGui::IsKeyDown(SDL_SCANCODE_D))
    {
        pos += displacement * right;
    }
    if (ImGui::IsKeyDown(SDL_SCANCODE_SPACE))
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
    glm::vec2 topLeft = ImGui::GetCursorScreenPos();
    glm::vec2 dims = ImGui::GetContentRegionAvail();
    glm::vec2 bottomRight = topLeft + dims;

    return Rect{topLeft, bottomRight};
}

void osc::DrawTextureAsImGuiImage(Texture2D& t, glm::vec2 dims)
{
    ImVec2 uv0{0.0f, 1.0f};
    ImVec2 uv1{1.0f, 0.0f};
    ImGui::Image(t.updTextureHandleHACK(), dims, uv0, uv1);
}

void osc::DrawTextureAsImGuiImage(RenderTexture& t, glm::vec2 dims)
{
    ImVec2 uv0{0.0f, 1.0f};
    ImVec2 uv1{1.0f, 0.0f};
    ImGui::Image(t.updTextureHandleHACK(), dims, uv0, uv1);
}

osc::ImGuiImageHittestResult::ImGuiImageHittestResult() :
    rect{},
    isHovered{false},
    isLeftClickReleasedWithoutDragging{false},
    isRightClickReleasedWithoutDragging{false}
{
}

osc::ImGuiImageHittestResult osc::DrawTextureAsImGuiImageAndHittest(Texture2D& tex, glm::vec2 dims, float dragThreshold)
{
    osc::DrawTextureAsImGuiImage(tex, dims);

    ImGuiImageHittestResult rv;
    rv.rect.p1 = ImGui::GetItemRectMin();
    rv.rect.p2 = ImGui::GetItemRectMax();
    rv.isHovered = ImGui::IsItemHovered();
    rv.isLeftClickReleasedWithoutDragging = rv.isHovered && osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Left, dragThreshold);
    rv.isRightClickReleasedWithoutDragging = rv.isHovered && osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right, dragThreshold);
    return rv;
}

osc::ImGuiImageHittestResult osc::DrawTextureAsImGuiImageAndHittest(RenderTexture& tex, glm::vec2 dims, float dragThreshold)
{
    osc::DrawTextureAsImGuiImage(tex, dims);

    ImGuiImageHittestResult rv;
    rv.rect.p1 = ImGui::GetItemRectMin();
    rv.rect.p2 = ImGui::GetItemRectMax();
    rv.isHovered = ImGui::IsItemHovered();
    rv.isLeftClickReleasedWithoutDragging = rv.isHovered && osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Left, dragThreshold);
    rv.isRightClickReleasedWithoutDragging = rv.isHovered && osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right, dragThreshold);
    return rv;
}

bool osc::IsAnyKeyDown(nonstd::span<int const> keys)
{
    for (auto key : keys)
    {
        if (ImGui::IsKeyDown(key))
        {
            return true;
        }
    }
    return false;
}

bool osc::IsAnyKeyDown(std::initializer_list<int const> keys)
{

    return IsAnyKeyDown(nonstd::span<int const>{keys.begin(), keys.end()});
}

bool osc::IsAnyKeyPressed(nonstd::span<int const> keys)
{
    for (int key : keys)
    {
        if (ImGui::IsKeyPressed(key))
        {
            return true;
        }
    }
    return false;
}
bool osc::IsAnyKeyPressed(std::initializer_list<int const> keys)
{
    return IsAnyKeyPressed(nonstd::span<int const>{keys.begin(), keys.end()});
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

bool osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton btn, float threshold)
{
    using osc::operator<<;

    if (!ImGui::IsMouseReleased(btn))
    {
        return false;
    }

    glm::vec2 dragDelta = ImGui::GetMouseDragDelta(btn);
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
    ImDrawList* dd = ImGui::GetWindowDrawList();

    constexpr float linelen = 35.0f;
    float fontSize = ImGui::GetFontSize();
    float circleRadius = fontSize/1.5f;
    float padding = circleRadius + 3.0f;
    glm::vec2 origin{renderRect.p1.x, renderRect.p2.y};
    origin.x += linelen + padding;
    origin.y -= linelen + padding;

    char const* labels[] = {"X", "Y", "Z"};

    for (int i = 0; i < 3; ++i)
    {
        glm::vec4 world = {0.0f, 0.0f, 0.0f, 0.0f};
        world[i] = 1.0f;
        glm::vec2 view = glm::vec2{viewMtx * world};
        view.y = -view.y;  // y goes down in screen-space

        glm::vec2 p1 = origin;
        glm::vec2 p2 = origin + linelen*view;

        glm::vec4 color = {0.2f, 0.2f, 0.2f, 1.0f};
        color[i] = 0.7f;
        ImVec4 col{color.x, color.y, color.z, color.a};
        dd->AddLine(p1, p2, ImGui::ColorConvertFloat4ToU32(col), 3.0f);
        dd->AddCircleFilled(p2, circleRadius, ImGui::ColorConvertFloat4ToU32(col));
        glm::vec2 ts = ImGui::CalcTextSize(labels[i]);
        dd->AddText(p2 - ts/2.0f, ImGui::ColorConvertFloat4ToU32({1.0f, 1.0f, 1.0f, 1.0f}), labels[i]);
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
    static SynchronizedValue<std::string> g_Buf;

    auto buf = g_Buf.lock();

    *buf = s;
    buf->resize(std::max(maxLen, s.size()));
    (*buf)[s.size()] = '\0';

    bool rv = ImGui::InputText(label, buf->data(), maxLen, flags);

    if (rv)
    {
        s = buf->data();
    }

    return rv;
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

    float copy[3];
    copy[0] = v[0];
    copy[1] = v[1];
    copy[2] = v[2];

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
    return ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar;
}

osc::Rect osc::GetMainViewportWorkspaceScreenRect()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    return Rect{viewport->WorkPos, glm::vec2{viewport->WorkPos} + glm::vec2{viewport->WorkSize}};
}

bool osc::IsMouseInMainViewportWorkspaceScreenRect()
{

    glm::vec2 mousepos = ImGui::GetIO().MousePos;
    osc::Rect hitRect = osc::GetMainViewportWorkspaceScreenRect();
    return osc::IsPointInRect(hitRect, mousepos);
}

bool osc::BeginMainViewportTopBar(char const* label)
{
    // https://github.com/ocornut/imgui/issues/3518
    ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
    float height = ImGui::GetFrameHeight();
    return ImGui::BeginViewportSideBar(label, viewport, ImGuiDir_Up, height, window_flags);
}

void osc::TextCentered(std::string const& s)
{
    auto windowWidth = ImGui::GetWindowSize().x;
    auto textWidth   = ImGui::CalcTextSize(s.c_str()).x;

    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::TextUnformatted(s.c_str());
}
