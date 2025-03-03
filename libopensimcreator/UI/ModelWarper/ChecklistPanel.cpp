#include "ChecklistPanel.h"

#include <libopensimcreator/Documents/ModelWarper/ValidationCheckResult.h>
#include <libopensimcreator/Documents/ModelWarper/ValidationCheckState.h>
#include <libopensimcreator/Documents/ModelWarper/WarpableOpenSimComponent.h>
#include <libopensimcreator/UI/ModelWarper/ModelWarperUIHelpers.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <liboscar/Graphics/Color.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/EnumHelpers.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>

#include <concepts>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

using namespace osc;
using namespace osc::mow;

// UI (generic)
namespace
{
    template<WarpableOpenSimComponent T>
    EntryStyling CalcStyle(const UIState& state, const T& c)
    {
        return ToStyle(state.state(c));
    }

    void DrawIcon(const EntryStyling& style)
    {
        ui::push_style_color(ui::ColorVar::Text, style.color);
        ui::draw_text(style.icon);
        ui::pop_style_color();
    }

    void DrawEntryIconAndText(
        const UIState&,
        const OpenSim::Component& component,
        EntryStyling style)
    {
        DrawIcon(style);
        ui::same_line();
        ui::draw_text(component.getName());
    }

    template<WarpableOpenSimComponent T>
    void DrawEntryIconAndText(const UIState& state, const T& c)
    {
        DrawEntryIconAndText(state, c, CalcStyle(state, c));
    }

    void DrawTooltipHeader(const UIState&, const OpenSim::Component& component)
    {
        ui::draw_text(GetAbsolutePathString(component));
        ui::same_line();
        ui::draw_text_disabled(component.getConcreteClassName());
        ui::draw_separator();
        ui::draw_dummy({0.0f, 3.0f});
    }

    template<WarpableOpenSimComponent T>
    void DrawDetailsTable(const UIState& state, const T& c)
    {
        if (ui::begin_table("##Details", 2)) {

            ui::table_setup_column("Label");
            ui::table_setup_column("Value");
            ui::table_headers_row();

            for (auto&& detail : state.details(c)) {
                ui::table_next_row();
                ui::table_set_column_index(0);
                ui::draw_text(detail.name());
                ui::table_set_column_index(1);
                ui::draw_text(detail.value());
            }

            ui::end_table();
        }
    }

    template<WarpableOpenSimComponent T>
    void DrawChecklist(const UIState& state, const T& c)
    {
        ui::indent(5.0f);
        int id = 0;
        for (auto&& check : state.validate(c)) {
            ui::push_id(id++);
            auto style = ToStyle(check.state());
            DrawIcon(style);
            ui::same_line();
            ui::draw_text(check.description());
            ui::pop_id();
        }
        ui::unindent(5.0f);
    }

    template<WarpableOpenSimComponent T>
    void DrawTooltipContent(const UIState& state, const T& c)
    {
        DrawTooltipHeader(state, c);

        ui::draw_text("Checklist:");
        ui::draw_dummy({0.0f, 3.0f});
        DrawChecklist(state, c);

        ui::start_new_line();

        ui::draw_text("Details:");
        ui::draw_dummy({0.0f, 3.0f});
        DrawDetailsTable(state, c);
    }

    template<WarpableOpenSimComponent T>
    void DrawEntry(const UIState& state, const T& c)
    {
        DrawEntryIconAndText(state, c);
        if (ui::is_item_hovered(ui::HoveredFlag::ForTooltip)) {
            ui::begin_tooltip_nowrap();
            DrawTooltipContent(state, c);
            ui::end_tooltip_nowrap();
        }
    }
}

// UI (meshes/mesh pairing)
namespace
{
    void DrawMeshSectionHeader(const UIState& state)
    {
        ui::draw_text("Meshes");
        ui::same_line();
        ui::draw_text_disabled("(%zu)", GetNumChildren<OpenSim::Mesh>(state.model()));
        ui::same_line();
        ui::draw_help_marker("Shows which meshes are elegible for warping in the source model - and whether the model warper has enough information to warp them (plus any other useful validation checks)");
    }

    void DrawMeshSection(const UIState& state)
    {
        DrawMeshSectionHeader(state);
        ui::draw_separator();
        int id = 0;
        for (const auto& mesh : state.model().getComponentList<OpenSim::Mesh>()) {
            ui::push_id(id++);
            DrawEntry(state, mesh);
            ui::pop_id();
        }
    }
}

// UI (frames)
namespace
{
    void DrawFramesSectionHeader(const UIState& state)
    {
        ui::draw_text("Warpable Frames");
        ui::same_line();
        ui::draw_text_disabled("(%zu)", GetNumChildren<OpenSim::PhysicalOffsetFrame>(state.model()));
        ui::same_line();
        ui::draw_help_marker("Shows which frames are eligible for warping in the source model - and whether the model warper has enough information to warp them");
    }

    void DrawFramesSection(const UIState& state)
    {
        DrawFramesSectionHeader(state);
        ui::draw_separator();
        int id = 0;
        for (const auto& pof : state.model().getComponentList<OpenSim::PhysicalOffsetFrame>()) {
            ui::push_id(id++);
            DrawEntry(state, pof);
            ui::pop_id();
        }
    }
}


// public API

void osc::mow::ChecklistPanel::impl_draw_content()
{
    int id = 0;

    ui::push_id(id++);
    DrawMeshSection(*m_State);
    ui::pop_id();

    ui::start_new_line();

    ui::push_id(id++);
    DrawFramesSection(*m_State);
    ui::pop_id();
}
