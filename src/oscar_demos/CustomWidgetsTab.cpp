#include "CustomWidgetsTab.h"

#include <oscar/oscar.h>

#include <cmath>
#include <array>
#include <memory>

using namespace osc;

namespace
{
    constexpr CStringView c_TabStringID = "Demos/CustomWidgets";

    void WidgetTitle(CStringView title, Vec2 pos)
    {
        Vec2 const textTopLeft = pos + Vec2{ui::GetStyle().FramePadding};
        ui::GetWindowDrawList()->AddText(textTopLeft, ui::GetColorU32(ImGuiCol_Text), title.c_str());
    }
}

// toggle
namespace
{
    void DrawToggle(bool enabled, bool hovered, ImVec2 pos, ImVec2 size)
    {
        ImDrawList& draw_list = *ui::GetWindowDrawList();

        float const radius = size.y * 0.5f;
        float const rounding = size.y * 0.25f;
        float const slot_half_height = size.y * 0.5f;
        bool const circular_grab = false;

        ImU32 const bgColor = hovered ?
            ui::GetColorU32(enabled ? ImGuiCol_FrameBgActive : ImGuiCol_FrameBgHovered) :
            ui::GetColorU32(enabled ? ImGuiCol_CheckMark : ImGuiCol_FrameBg);

        Vec2 const pmid{
            pos.x + radius + (enabled ? 1.0f : 0.0f) * (size.x - radius * 2),
            pos.y + size.y / 2.0f,
        };
        ImVec2 const smin = {pos.x, pmid.y - slot_half_height};
        ImVec2 const smax = {pos.x + size.x, pmid.y + slot_half_height};

        draw_list.AddRectFilled(smin, smax, bgColor, rounding);

        if (circular_grab) {
            draw_list.AddCircleFilled(pmid, radius * 0.8f, ui::GetColorU32(ImGuiCol_SliderGrab));
        }
        else {
            Vec2 const offs = {radius*0.8f, radius*0.8f};
            draw_list.AddRectFilled(pmid - offs, pmid + offs, ui::GetColorU32(ImGuiCol_SliderGrab), rounding);
        }
    }

    bool Toggle(CStringView label, bool* v)
    {
        ui::PushStyleColor(ImGuiCol_Button, IM_COL32_BLACK_TRANS);

        ImGuiStyle const& style = ui::GetStyle();

        float const titleHeight = ui::GetTextLineHeight();

        ImVec2 const p = ui::GetCursorScreenPos();
        ImVec2 const bb(ui::GetColumnWidth(), ui::GetFrameHeight());
        ui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0, 0));
        ui::PushID(label);
        bool const status = ui::Button("###toggle_button", bb);

        if (status) {
            *v = !*v;
        }

        ui::PopID();
        ui::PopStyleVar();
        ImVec2 const pMin = ui::GetItemRectMin();
        ImVec2 const pMax = ui::GetItemRectMax();

        WidgetTitle(label, p);

        float const toggleHeight = titleHeight * 0.9f;
        ImVec2 const toggleSize = {toggleHeight * 1.75f, toggleHeight};
        ImVec2 const togglePos{
            pMax.x - toggleSize.x - style.FramePadding.x,
            pMin.y + (titleHeight - toggleSize.y)/2.0f + style.FramePadding.y,
        };
        DrawToggle(*v, ui::IsItemHovered(), togglePos, toggleSize);

        ui::PopStyleColor();

        return status;
    }
}

class osc::CustomWidgetsTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {}

private:
    void implOnDraw() final
    {
        ui::Begin("window");
        ui::InputFloat("standardinput", &m_Value);
        ui::CircularSliderFloat("custom slider", &m_Value, 15.0f, 5.0f);
        ui::Text("%f", m_Value);
        Toggle("custom toggle", &m_Toggle);
        ui::End();
    }

    float m_Value = 10.0f;
    bool m_Toggle = false;
};


// public API

CStringView osc::CustomWidgetsTab::id()
{
    return c_TabStringID;
}

osc::CustomWidgetsTab::CustomWidgetsTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{}

osc::CustomWidgetsTab::CustomWidgetsTab(CustomWidgetsTab&&) noexcept = default;
osc::CustomWidgetsTab& osc::CustomWidgetsTab::operator=(CustomWidgetsTab&&) noexcept = default;
osc::CustomWidgetsTab::~CustomWidgetsTab() noexcept = default;

UID osc::CustomWidgetsTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::CustomWidgetsTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::CustomWidgetsTab::implOnDraw()
{
    m_Impl->onDraw();
}
