#include "CoordinateEditorPanel.h"

#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/Documents/Model/UndoableModelActions.h>
#include <libopensimcreator/Platform/OSCColors.h>
#include <libopensimcreator/UI/Shared/ComponentContextMenu.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <liboscar/Graphics/Color.h>
#include <liboscar/Platform/App.h>
#include <liboscar/Platform/IconCodepoints.h>
#include <liboscar/Platform/Widget.h>
#include <liboscar/UI/Events/OpenPopupEvent.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Panels/PanelPrivate.h>
#include <liboscar/Utils/Assertions.h>
#include <liboscar/Utils/CStringView.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>

#include <algorithm>
#include <ranges>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

class osc::CoordinateEditorPanel::Impl final : public PanelPrivate {
public:

    explicit Impl(
        CoordinateEditorPanel& owner,
        std::string_view panelName_,
        Widget& parent,
        std::shared_ptr<IModelStatePair> model_) :

        PanelPrivate{owner, &parent, panelName_},
        m_Model{std::move(model_)}
    {}

    void draw_content()
    {
        // load coords
        std::vector<const OpenSim::Coordinate*> coordPtrs = GetCoordinatesInModel(m_Model->getModel());

        // if there's no coordinates in the model, show a warning message and stop drawing
        if (coordPtrs.empty()) {
            ui::draw_text_disabled_and_panel_centered("(no coordinates in the model)");
            return;
        }

        // Draw a menu for bulk-manipulating the model's pose.
        drawPoseDropdownButton();

        // else: there's coordinates, which should be shown in a table
        const ui::TableFlags flags = {
            ui::TableFlag::NoSavedSettings,
            ui::TableFlag::Resizable,
            ui::TableFlag::Sortable,
            ui::TableFlag::SortTristate,
            ui::TableFlag::BordersInnerV,
            ui::TableFlag::SizingStretchSame,
        };
        if (ui::begin_table("##coordinatestable", 3, flags)) {
            ui::table_setup_column("Name");
            ui::table_setup_column("Value", ui::ColumnFlag::NoSort, 1.65f);
            ui::table_setup_column("Speed", ui::ColumnFlag::NoSort, 0.5f);
            ui::table_setup_scroll_freeze(0, 1);
            ui::table_headers_row();

            if (ui::table_column_sort_specs_are_dirty()) {
                const auto specs = ui::get_table_column_sort_specs();

                // we know the user can only sort one column (name) so we don't need to permute
                // through the entire specs structure
                if (specs.size() == 1 && specs.front().column_index == 0 && specs.front().sort_order == 0) {
                    switch (specs.front().sort_direction) {
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

private:
    void drawPoseDropdownButton()
    {
        if (m_Model->isReadonly()) {
            ui::begin_disabled();
        }
        ui::draw_button("Pose " OSC_ICON_CARET_DOWN);
        if (ui::begin_popup_context_menu("##PosePopup", ui::PopupFlag::MouseButtonLeft)) {
            // Draw a button that can be used to zero all coordinates, which can be a cheap way
            // of resetting a model's pose (#957).
            if (ui::draw_menu_item("Zero All Coordinates")) {
                ActionZeroAllCoordinates(*m_Model);
            }
            ui::end_popup();
        }
        if (m_Model->isReadonly()) {
            ui::end_disabled();
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
        if (&c == m_Model->getHovered()) {
            ui::push_style_color(ui::ColorVar::Text, OSCColors::hovered());
            ++stylesPushed;
        }
        if (&c == m_Model->getSelected()) {
            ui::push_style_color(ui::ColorVar::Text, OSCColors::selected());
            ++stylesPushed;
        }

        ui::draw_text(c.getName());
        ui::pop_style_color(std::exchange(stylesPushed, 0));

        if (ui::is_item_hovered()) {
            m_Model->setHovered(&c);

            std::stringstream ss;
            ss << "    motion type = " << GetMotionTypeDisplayName(c) << '\n';
            ss << "    owner = " << TryGetOwnerName(c).value_or("(no owner)");

            ui::draw_tooltip(c.getName(), ss.str());
        }

        if (ui::is_item_clicked(ui::MouseButton::Left)) {
            m_Model->setSelected(&c);
        }
        else if (ui::is_item_clicked(ui::MouseButton::Right)) {
            auto popup = std::make_unique<ComponentContextMenu>(
                "##componentcontextmenu",
                *parent(),
                m_Model,
                GetAbsolutePath(c)
            );
            App::post_event<OpenPopupEvent>(*parent(), std::move(popup));
        }
    }

    void drawDataCell(const OpenSim::Coordinate& c)
    {
        const bool disabled = m_Model->isReadonly();
        if (disabled) {
            ui::begin_disabled();
        }
        drawDataCellLockButton(c);
        ui::same_line(0.0f, 0.0f);
        drawDataCellCoordinateSlider(c);
        if (disabled) {
            ui::end_disabled();
        }
    }

    void drawDataCellLockButton(const OpenSim::Coordinate& c)
    {
        ui::push_style_color(ui::ColorVar::Button, Color::clear());
        ui::push_style_color(ui::ColorVar::ButtonActive, Color::clear());
        ui::push_style_color(ui::ColorVar::ButtonHovered, Color::clear());
        ui::push_style_var(ui::StyleVar::FramePadding, {0.0f, ui::get_style_frame_padding().y});
        if (ui::draw_button(c.getLocked(m_Model->getState()) ? OSC_ICON_LOCK : OSC_ICON_UNLOCK)) {
            const bool newValue = !c.getLocked(m_Model->getState());
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

        const float minValue = ConvertCoordValueToDisplayValue(c, c.getRangeMin());
        const float maxValue = ConvertCoordValueToDisplayValue(c, c.getRangeMax());
        float displayedValue = ConvertCoordValueToDisplayValue(c, c.getValue(m_Model->getState()));

        if (coordinateLocked) {
            ui::push_style_var(ui::StyleVar::DisabledAlpha, 0.2f);
            ui::begin_disabled();
        }
        if (ui::draw_float_circular_slider("##coordinatevalueeditor", &displayedValue, minValue, maxValue)) {
            const double storedValue = ConvertCoordDisplayValueToStorageValue(c, displayedValue);
            ActionSetCoordinateValue(*m_Model, c, storedValue);
        }
        if (coordinateLocked) {
            ui::end_disabled();
            ui::pop_style_var();
        }
        if (ui::is_item_deactivated_after_edit()) {
            const double storedValue = ConvertCoordDisplayValueToStorageValue(c, displayedValue);
            ActionSetCoordinateValueAndSave(*m_Model, c, storedValue);
        }
        ui::draw_tooltip_body_only_if_item_hovered("Ctrl-click the slider to edit");
    }

    void drawSpeedCell(const OpenSim::Coordinate& c)
    {
        float displayedSpeed = ConvertCoordValueToDisplayValue(c, c.getSpeedValue(m_Model->getState()));

        ui::set_next_item_width(ui::get_content_region_available().x);
        if (ui::draw_float_meters_input("##coordinatespeededitor", displayedSpeed)) {
            const double storedSpeed = ConvertCoordDisplayValueToStorageValue(c, displayedSpeed);
            ActionSetCoordinateSpeed(*m_Model, c, storedSpeed);
        }

        if (ui::is_item_deactivated_after_edit()) {
            const double storedSpeed = ConvertCoordDisplayValueToStorageValue(c, displayedSpeed);
            ActionSetCoordinateSpeedAndSave(*m_Model, c, storedSpeed);
        }
    }

    std::shared_ptr<IModelStatePair> m_Model;
};

osc::CoordinateEditorPanel::CoordinateEditorPanel(
    std::string_view panelName,
    Widget& parent,
    std::shared_ptr<IModelStatePair> model) :

    Panel{std::make_unique<Impl>(*this, panelName, parent, std::move(model))}
{}
void osc::CoordinateEditorPanel::impl_draw_content() { private_data().draw_content(); }
