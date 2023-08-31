#include "CustomWidgetsTab.hpp"

#include "oscar/Tabs/StandardTabBase.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/Graphics/Color.hpp"

#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <SDL_events.h>

#include <array>
#include <cmath>
#include <string>
#include <utility>

namespace
{
    constexpr osc::CStringView c_TabStringID = "Experiments/CustomWidgets";

    void WidgetTitle(osc::CStringView title, glm::vec2 pos)
    {
        glm::vec2 const textTopLeft = pos + glm::vec2{ImGui::GetStyle().FramePadding};
        ImGui::GetWindowDrawList()->AddText(textTopLeft, ImGui::GetColorU32(ImGuiCol_Text), title.c_str());
    }

    glm::vec2 Midpoint(ImRect const& r)
    {
        return 0.5f * (glm::vec2{r.Min} + glm::vec2{r.Max});
    }

    glm::vec2 Size(ImRect const& r)
    {
        return glm::vec2{r.Max} - glm::vec2{r.Min};
    }

    float ShortestEdgeLength(ImRect const& r)
    {
        const glm::vec2 sz = Size(r);
        return glm::min(sz.x, sz.y);
    }

    ImU32 Brighten(ImU32 color, float factor)
    {
        const osc::Color srgb{ImGui::ColorConvertU32ToFloat4(color)};
        const osc::Color brightened = factor * srgb;
        const osc::Color clamped = osc::ClampToLDR(brightened);
        return ImGui::ColorConvertFloat4ToU32(glm::vec4{clamped});
    }
}

// slider
namespace
{
    bool Slider(
        char const* label,
        float* v,
        float min,
        float max,
        char const* format = "%.3f",
        ImGuiSliderFlags flags = 0)
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
        const ImGuiID id = window->GetID(label);

        // calculate top-level item info for early-cull checks etc.
        const glm::vec2 labelSize = ImGui::CalcTextSize(label, nullptr, true);
        const glm::vec2 frameDims = {ImGui::CalcItemWidth(), labelSize.y + 2.0f*style.FramePadding.y};
        const glm::vec2 cursorScreenPos = ImGui::GetCursorScreenPos();
        const ImRect frameBB = {cursorScreenPos, cursorScreenPos + frameDims};
        const float labelWidthWithSpacing = labelSize.x > 0.0f ? labelSize.x + style.ItemInnerSpacing.x : 0.0f;
        const ImRect totalBB = {frameBB.Min, glm::vec2{frameBB.Max} + glm::vec2{labelWidthWithSpacing, 0.0f}};

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
            // tabbing or CTRL+clicking the slider temporarily transforms it into an input box
            const bool inputRequestedByTabbing = temporaryTextInputAllowed && (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_FocusedByTabbing) != 0;
            const bool clicked = isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left, id);
            const bool makeActive = (inputRequestedByTabbing || clicked || g.NavActivateId == id);

            if (makeActive && clicked)
            {
                ImGui::SetKeyOwner(ImGuiKey_MouseLeft, id);  // tell ImGui that left-click is locked from further interaction etc. this frame
            }
            if (makeActive && temporaryTextInputAllowed)
            {
                if (inputRequestedByTabbing || (clicked && g.IO.KeyCtrl) || (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput)))
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
                label,
                ImGuiDataType_Float,
                static_cast<void*>(v),
                format,
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
            format,
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
            const glm::vec2 sliderNobCenter = Midpoint(grabBoundingBox);
            const float sliderNobRadius = 0.75f * ShortestEdgeLength(grabBoundingBox);
            const float sliderRailThickness = 0.5f * sliderNobRadius;
            const float sliderRailTopY = sliderNobCenter.y - 0.5f*sliderRailThickness;
            const float sliderRailBottomY = sliderNobCenter.y + 0.5f*sliderRailThickness;

            const bool isActive = g.ActiveId == id;
            const ImU32 railColor = ImGui::GetColorU32(isHovered ? ImGuiCol_FrameBgHovered : isActive ? ImGuiCol_FrameBgActive : ImGuiCol_FrameBg);
            const ImU32 grabColor = ImGui::GetColorU32(isActive ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab);

            // render left-hand rail (brighter)
            {
                glm::vec2 const lhsRailTopLeft = {frameBB.Min.x, sliderRailTopY};
                glm::vec2 const lhsRailBottomright = {sliderNobCenter.x, sliderRailBottomY};
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
                glm::vec2 const rhsRailTopLeft = {sliderNobCenter.x, sliderRailTopY};
                glm::vec2 const rhsRailBottomRight = {frameBB.Max.x, sliderRailBottomY};

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
                const char* bufEnd = buf.data() + ImGui::DataTypeFormatString(buf.data(), static_cast<int>(buf.size()), ImGuiDataType_Float, v, format);
                if (g.LogEnabled)
                {
                    ImGui::LogSetNextTextDecoration("{", "}");
                }
                ImGui::RenderTextClipped(frameBB.Min, frameBB.Max, buf.data(), bufEnd, nullptr, ImVec2(0.5f, 0.5f));
            }

            // render input label in remaining space
            if (labelSize.x > 0.0f)
            {
                ImGui::RenderText(ImVec2(frameBB.Max.x + style.ItemInnerSpacing.x, frameBB.Min.y + style.FramePadding.y), label);
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
                const char* bufEnd = buf.data() + ImGui::DataTypeFormatString(buf.data(), static_cast<int>(buf.size()), ImGuiDataType_Float, v, format);
                if (g.LogEnabled)
                {
                    ImGui::LogSetNextTextDecoration("{", "}");
                }
                ImGui::RenderTextClipped(frameBB.Min, frameBB.Max, buf.data(), bufEnd, nullptr, ImVec2(0.5f, 0.5f));
            }

            // render input label in remaining space
            if (labelSize.x > 0.0f)
            {
                ImGui::RenderText(ImVec2(frameBB.Max.x + style.ItemInnerSpacing.x, frameBB.Min.y + style.FramePadding.y), label);
            }
        }

        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));

        return valueChanged;
    }
}

// toggle
namespace
{
    void DrawToggle(bool enabled, bool hovered, ImVec2 pos, ImVec2 size)
    {
        ImDrawList& draw_list = *ImGui::GetWindowDrawList();

        float const radius = size.y * 0.5f;
        float const rounding = size.y * 0.25f;
        float const slot_half_height = size.y * 0.5f;
        bool const circular_grab = false;

        ImU32 const bgColor = hovered ?
            ImGui::GetColorU32(enabled ? ImGuiCol_FrameBgActive : ImGuiCol_FrameBgHovered) :
            ImGui::GetColorU32(enabled ? ImGuiCol_CheckMark : ImGuiCol_FrameBg);

        glm::vec2 const pmid
        {
            pos.x + radius + (enabled ? 1.0f : 0.0f) * (size.x - radius * 2),
            pos.y + size.y / 2.0f,
        };
        ImVec2 const smin = {pos.x, pmid.y - slot_half_height};
        ImVec2 const smax = {pos.x + size.x, pmid.y + slot_half_height};

        draw_list.AddRectFilled(smin, smax, bgColor, rounding);

        if (circular_grab)
        {
            draw_list.AddCircleFilled(pmid, radius * 0.8f, ImGui::GetColorU32(ImGuiCol_SliderGrab));
        }
        else
        {
            glm::vec2 const offs = {radius*0.8f, radius*0.8f};
            draw_list.AddRectFilled(pmid - offs, pmid + offs, ImGui::GetColorU32(ImGuiCol_SliderGrab), rounding);
        }
    }

    bool Toggle(osc::CStringView label, bool* v)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32_BLACK_TRANS);

        ImGuiStyle const& style = ImGui::GetStyle();

        float const titleHeight = ImGui::GetTextLineHeight();

        ImVec2 const p = ImGui::GetCursorScreenPos();
        ImVec2 const bb(ImGui::GetColumnWidth(), ImGui::GetFrameHeight());
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0, 0));
        ImGui::PushID(label.c_str());
        bool const status = ImGui::Button("###toggle_button", bb);

        if (status)
        {
            *v = !*v;
        }

        ImGui::PopID();
        ImGui::PopStyleVar();
        ImVec2 const pMin = ImGui::GetItemRectMin();
        ImVec2 const pMax = ImGui::GetItemRectMax();

        WidgetTitle(label, p);

        float const toggleHeight = titleHeight * 0.9f;
        ImVec2 const toggleSize = {toggleHeight * 1.75f, toggleHeight};
        ImVec2 const togglePos
        {
            pMax.x - toggleSize.x - style.FramePadding.x,
            pMin.y + (titleHeight - toggleSize.y)/2.0f + style.FramePadding.y,
        };
        DrawToggle(*v, ImGui::IsItemHovered(), togglePos, toggleSize);

        ImGui::PopStyleColor();

        return status;
    }
}

class osc::CustomWidgetsTab::Impl final : public osc::StandardTabBase {
public:
    Impl() : StandardTabBase{c_TabStringID}
    {
    }

private:
    void implOnDraw() final
    {
        ImGui::Begin("window");
        ImGui::InputFloat("standardinput", &m_Value);
        Slider("custom slider", &m_Value, 15.0f, 5.0f);
        ImGui::Text("%f", m_Value);
        Toggle("custom toggle", &m_Toggle);
        ImGui::End();
    }

    float m_Value = 10.0f;
    bool m_Toggle = false;
};


// public API

osc::CStringView osc::CustomWidgetsTab::id() noexcept
{
    return c_TabStringID;
}

osc::CustomWidgetsTab::CustomWidgetsTab(ParentPtr<TabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::CustomWidgetsTab::CustomWidgetsTab(CustomWidgetsTab&&) noexcept = default;
osc::CustomWidgetsTab& osc::CustomWidgetsTab::operator=(CustomWidgetsTab&&) noexcept = default;
osc::CustomWidgetsTab::~CustomWidgetsTab() noexcept = default;

osc::UID osc::CustomWidgetsTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::CustomWidgetsTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::CustomWidgetsTab::implOnDraw()
{
    m_Impl->onDraw();
}
