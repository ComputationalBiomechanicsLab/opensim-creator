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
        Vec2 const textTopLeft = pos + ui::get_style_frame_padding();
        ui::get_panel_draw_list()->AddText(textTopLeft, ui::get_color_ImU32(ImGuiCol_Text), title.c_str());
    }
}

// toggle
namespace
{
    void DrawToggle(bool enabled, bool hovered, Vec2 pos, Vec2 size)
    {
        ImDrawList& draw_list = *ui::get_panel_draw_list();

        float const radius = size.y * 0.5f;
        float const rounding = size.y * 0.25f;
        float const slot_half_height = size.y * 0.5f;
        bool const circular_grab = false;

        ImU32 const bgColor = hovered ?
            ui::get_color_ImU32(enabled ? ImGuiCol_FrameBgActive : ImGuiCol_FrameBgHovered) :
            ui::get_color_ImU32(enabled ? ImGuiCol_CheckMark : ImGuiCol_FrameBg);

        Vec2 const pmid{
            pos.x + radius + (enabled ? 1.0f : 0.0f) * (size.x - radius * 2),
            pos.y + size.y / 2.0f,
        };
        Vec2 const smin = {pos.x, pmid.y - slot_half_height};
        Vec2 const smax = {pos.x + size.x, pmid.y + slot_half_height};

        draw_list.AddRectFilled(smin, smax, bgColor, rounding);

        if (circular_grab) {
            draw_list.AddCircleFilled(pmid, radius * 0.8f, ui::get_color_ImU32(ImGuiCol_SliderGrab));
        }
        else {
            Vec2 const offs = {radius*0.8f, radius*0.8f};
            draw_list.AddRectFilled(pmid - offs, pmid + offs, ui::get_color_ImU32(ImGuiCol_SliderGrab), rounding);
        }
    }

    bool Toggle(CStringView label, bool* v)
    {
        ui::push_style_color(ImGuiCol_Button, IM_COL32_BLACK_TRANS);

        float const titleHeight = ui::get_text_line_height();

        Vec2 const p = ui::get_cursor_screen_pos();
        Vec2 const bb(ui::get_column_width(), ui::get_frame_height());
        ui::push_style_var(ImGuiStyleVar_ButtonTextAlign, {0.0f, 0.0f});
        ui::push_id(label);
        bool const status = ui::draw_button("###toggle_button", bb);

        if (status) {
            *v = !*v;
        }

        ui::pop_id();
        ui::pop_style_var();
        Vec2 const pMin = ui::get_item_topleft();
        Vec2 const pMax = ui::get_item_bottomright();

        WidgetTitle(label, p);

        float const toggleHeight = titleHeight * 0.9f;
        Vec2 const framePadding = ui::get_style_frame_padding();
        Vec2 const toggleSize = {toggleHeight * 1.75f, toggleHeight};
        Vec2 const togglePos{
            pMax.x - toggleSize.x - framePadding.x,
            pMin.y + (titleHeight - toggleSize.y)/2.0f + framePadding.y,
        };
        DrawToggle(*v, ui::is_item_hovered(), togglePos, toggleSize);

        ui::pop_style_color();

        return status;
    }
}

class osc::CustomWidgetsTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {}

private:
    void impl_on_draw() final
    {
        ui::begin_panel("window");
        ui::draw_float_input("standardinput", &m_Value);
        ui::draw_float_circular_slider("custom slider", &m_Value, 15.0f, 5.0f);
        ui::draw_text("%f", m_Value);
        Toggle("custom toggle", &m_Toggle);
        ui::end_panel();
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

UID osc::CustomWidgetsTab::impl_get_id() const
{
    return m_Impl->id();
}

CStringView osc::CustomWidgetsTab::impl_get_name() const
{
    return m_Impl->name();
}

void osc::CustomWidgetsTab::impl_on_draw()
{
    m_Impl->on_draw();
}
