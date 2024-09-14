#include "CoordinateEditorPanel.h"

#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Platform/OSCColors.h>
#include <OpenSimCreator/UI/ModelEditor/ComponentContextMenu.h>
#include <OpenSimCreator/UI/ModelEditor/IEditorAPI.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <oscar/Graphics/Color.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Panels/StandardPanelImpl.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/ParentPtr.h>

#include <algorithm>
#include <ranges>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

class osc::CoordinateEditorPanel::Impl final : public StandardPanelImpl {
public:

    Impl(
        std::string_view panelName_,
        const ParentPtr<IMainUIStateAPI>& mainUIStateAPI_,
        IEditorAPI* editorAPI_,
        std::shared_ptr<UndoableModelStatePair> uum_) :

        StandardPanelImpl{panelName_},
        m_MainUIStateAPI{mainUIStateAPI_},
        m_EditorAPI{editorAPI_},
        m_Model{std::move(uum_)}
    {}

private:

    void impl_draw_content() final
    {
        // load coords
        std::vector<const OpenSim::Coordinate*> coordPtrs = GetCoordinatesInModel(m_Model->getModel());

        // if there's no coordinates in the model, show a warning message and stop drawing
        if (coordPtrs.empty())
        {
            ui::draw_text_disabled_and_panel_centered("(no coordinates in the model)");
            return;
        }

        // else: there's coordinates, which should be shown in a table
        const ui::TableFlags flags = {
            ui::TableFlag::NoSavedSettings,
            ui::TableFlag::Resizable,
            ui::TableFlag::Sortable,
            ui::TableFlag::SortTristate,
            ui::TableFlag::BordersInnerV,
            ui::TableFlag::SizingStretchSame,
        };
        if (ui::begin_table("##coordinatestable", 3, flags))
        {
            ui::table_setup_column("Name");
            ui::table_setup_column("Value", ui::ColumnFlag::NoSort, 1.65f);
            ui::table_setup_column("Speed", ui::ColumnFlag::NoSort, 0.5f);
            ui::table_setup_scroll_freeze(0, 1);
            ui::table_headers_row();

            if (ui::table_column_sort_specs_are_dirty()) {
                const auto specs = ui::get_table_column_sort_specs();

                // we know the user can only sort one column (name) so we don't need to permute
                // through the entire specs structure
                if (specs.size() == 1 && specs.front().column_index == 0 && specs.front().sort_order == 0)
                {
                    switch (specs.front().sort_direction)
                    {
                    case ui::SortDirection::Ascending:
                        rgs::sort(coordPtrs, rgs::less{}, [](const auto& ptr) { return ptr->getName(); });
                        break;
                    case ui::SortDirection::Descending:
                        rgs::sort(coordPtrs, rgs::greater{}, [](const auto& ptr) { return ptr->getName(); });
                        break;
                    case ui::SortDirection::None:
                    default:
                        break;  // just use them as-is
                    }
                }
            }

            int id = 0;
            for (const OpenSim::Coordinate* coordPtr : coordPtrs) {
                ui::push_id(id++);
                drawRow(*coordPtr);
                ui::pop_id();
            }

            ui::end_table();
        }
    }

    void drawRow(const OpenSim::Coordinate& c)
    {
        ui::table_next_row();

        int column = 0;
        ui::table_set_column_index(column++);
        drawNameCell(c);

        ui::table_set_column_index(column++);
        drawDataCell(c);
        OSC_ASSERT_ALWAYS(c.hasOwner() && "An `OpenSim::Coordinate` must always have an owner. This bug can occur when using is_free_to_satisfy_coordinates (see issue #888)");

        ui::table_set_column_index(column++);
        drawSpeedCell(c);
        OSC_ASSERT_ALWAYS(c.hasOwner() && "An `OpenSim::Coordinate` must always have an owner. This bug can occur when using is_free_to_satisfy_coordinates (see issue #888)");
    }

    void drawNameCell(const OpenSim::Coordinate& c)
    {
        int stylesPushed = 0;
        if (&c == m_Model->getHovered())
        {
            ui::push_style_color(ui::ColorVar::Text, OSCColors::hovered());
            ++stylesPushed;
        }
        if (&c == m_Model->getSelected())
        {
            ui::push_style_color(ui::ColorVar::Text, OSCColors::selected());
            ++stylesPushed;
        }

        ui::draw_text_unformatted(c.getName());
        ui::pop_style_color(std::exchange(stylesPushed, 0));

        if (ui::is_item_hovered())
        {
            m_Model->setHovered(&c);

            std::stringstream ss;
            ss << "    motion type = " << GetMotionTypeDisplayName(c) << '\n';
            ss << "    owner = " << TryGetOwnerName(c).value_or("(no owner)");

            ui::draw_tooltip(c.getName(), ss.str());
        }

        if (ui::is_item_clicked(ui::MouseButton::Left))
        {
            m_Model->setSelected(&c);
        }
        else if (ui::is_item_clicked(ui::MouseButton::Right))
        {
            auto popup = std::make_unique<ComponentContextMenu>(
                "##componentcontextmenu",
                m_MainUIStateAPI,
                m_EditorAPI,
                m_Model,
                GetAbsolutePath(c)
            );
            popup->open();
            m_EditorAPI->pushPopup(std::move(popup));
        }
    }

    void drawDataCell(const OpenSim::Coordinate& c)
    {
        drawDataCellLockButton(c);
        ui::same_line(0.0f, 0.0f);
        drawDataCellCoordinateSlider(c);
    }

    void drawDataCellLockButton(const OpenSim::Coordinate& c)
    {
        ui::push_style_color(ui::ColorVar::Button, Color::clear());
        ui::push_style_color(ui::ColorVar::ButtonActive, Color::clear());
        ui::push_style_color(ui::ColorVar::ButtonHovered, Color::clear());
        ui::push_style_var(ui::StyleVar::FramePadding, {0.0f, ui::get_style_frame_padding().y});
        if (ui::draw_button(c.getLocked(m_Model->getState()) ? ICON_FA_LOCK : ICON_FA_UNLOCK))
        {
            bool newValue = !c.getLocked(m_Model->getState());
            ActionSetCoordinateLockedAndSave(*m_Model, c, newValue);
        }
        ui::pop_style_var();
        ui::pop_style_color();
        ui::pop_style_color();
        ui::pop_style_color();
        ui::draw_tooltip_if_item_hovered("Toggle Coordinate Lock", "Lock/unlock the coordinate's value.\n\nLocking a coordinate indicates whether the coordinate's value should be constrained to this value during the simulation.");
    }

    void drawDataCellCoordinateSlider(const OpenSim::Coordinate& c)
    {
        const bool coordinateLocked = c.getLocked(m_Model->getState());

        ui::set_next_item_width(ui::get_content_region_available().x);

        float minValue = ConvertCoordValueToDisplayValue(c, c.getRangeMin());
        float maxValue = ConvertCoordValueToDisplayValue(c, c.getRangeMax());
        float displayedValue = ConvertCoordValueToDisplayValue(c, c.getValue(m_Model->getState()));

        if (coordinateLocked)
        {
            ui::push_style_var(ui::StyleVar::DisabledAlpha, 0.2f);
            ui::begin_disabled();
        }
        if (ui::draw_float_circular_slider("##coordinatevalueeditor", &displayedValue, minValue, maxValue))
        {
            double storedValue = ConvertCoordDisplayValueToStorageValue(c, displayedValue);
            ActionSetCoordinateValue(*m_Model, c, storedValue);
        }
        if (coordinateLocked)
        {
            ui::end_disabled();
            ui::pop_style_var();
        }
        if (ui::is_item_deactivated_after_edit())
        {
            double storedValue = ConvertCoordDisplayValueToStorageValue(c, displayedValue);
            ActionSetCoordinateValueAndSave(*m_Model, c, storedValue);
        }
        ui::draw_tooltip_body_only_if_item_hovered("Ctrl-click the slider to edit");
    }

    void drawSpeedCell(const OpenSim::Coordinate& c)
    {
        float displayedSpeed = ConvertCoordValueToDisplayValue(c, c.getSpeedValue(m_Model->getState()));

        ui::set_next_item_width(ui::get_content_region_available().x);
        if (ui::draw_float_meters_input("##coordinatespeededitor", displayedSpeed))
        {
            double storedSpeed = ConvertCoordDisplayValueToStorageValue(c, displayedSpeed);
            ActionSetCoordinateSpeed(*m_Model, c, storedSpeed);
        }

        if (ui::is_item_deactivated_after_edit())
        {
            double storedSpeed = ConvertCoordDisplayValueToStorageValue(c, displayedSpeed);
            ActionSetCoordinateSpeedAndSave(*m_Model, c, storedSpeed);
        }
    }

    ParentPtr<IMainUIStateAPI> m_MainUIStateAPI;
    IEditorAPI* m_EditorAPI;
    std::shared_ptr<UndoableModelStatePair> m_Model;
};


osc::CoordinateEditorPanel::CoordinateEditorPanel(
    std::string_view panelName_,
    const ParentPtr<IMainUIStateAPI>& mainUIStateAPI_,
    IEditorAPI* editorAPI_,
    std::shared_ptr<UndoableModelStatePair> uum_) :

    m_Impl{std::make_unique<Impl>(panelName_, mainUIStateAPI_, editorAPI_, std::move(uum_))}
{}
osc::CoordinateEditorPanel::CoordinateEditorPanel(CoordinateEditorPanel&&) noexcept = default;
osc::CoordinateEditorPanel& osc::CoordinateEditorPanel::operator=(CoordinateEditorPanel&&) noexcept = default;
osc::CoordinateEditorPanel::~CoordinateEditorPanel() noexcept = default;

CStringView osc::CoordinateEditorPanel::impl_get_name() const
{
    return m_Impl->name();
}

bool osc::CoordinateEditorPanel::impl_is_open() const
{
    return m_Impl->is_open();
}

void osc::CoordinateEditorPanel::impl_open()
{
    m_Impl->open();
}

void osc::CoordinateEditorPanel::impl_close()
{
    m_Impl->close();
}

void osc::CoordinateEditorPanel::impl_on_draw()
{
    m_Impl->on_draw();
}
