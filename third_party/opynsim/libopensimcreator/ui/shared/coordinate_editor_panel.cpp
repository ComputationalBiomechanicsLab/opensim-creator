#include "coordinate_editor_panel.h"

#include <libopensimcreator/documents/model/undoable_model_actions.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/platform/open_sim_creator_colors.h>
#include <libopensimcreator/ui/shared/component_context_menu.h>

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/graphics/color.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/widget.h>
#include <liboscar/ui/events/open_popup_event.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/panels/panel_private.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/c_string_view.h>
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
        Widget* parent,
        std::string_view panel_name,
        std::shared_ptr<opyn::ModelStatePair> model) :

        PanelPrivate{owner, parent, panel_name},
        model_{std::move(model)}
    {}

    void draw_content()
    {
        // load coords
        std::vector<const OpenSim::Coordinate*> coord_ptrs =
            opyn::get_coordinates_in_model(model_->get_model());

        // if there's no coordinates in the model, show a warning message and stop drawing
        if (coord_ptrs.empty()) {
            ui::draw_text_disabled_and_panel_centered("(no coordinates in the model)");
            return;
        }

        // Draw a menu for bulk-manipulating the model's pose.
        draw_pose_dropdown_button();

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
                if (specs.size() == 1 and specs.front().column_index == 0 and
                    specs.front().sort_order == 0) {

                    switch (specs.front().sort_direction) {
                    case ui::SortDirection::Ascending:
                        rgs::sort(coord_ptrs, rgs::less{}, [](const auto& ptr) {
                            return ptr->getName();
                        });
                        break;
                    case ui::SortDirection::Descending:
                        rgs::sort(coord_ptrs, rgs::greater{}, [](const auto& ptr) {
                            return ptr->getName();
                        });
                        break;
                    case ui::SortDirection::None:
                    default: break;  // just use them as-is
                    }
                }
            }

            int id = 0;
            for (const OpenSim::Coordinate* coord_ptr : coord_ptrs) {
                ui::push_id(id++);
                draw_row(*coord_ptr);
                ui::pop_id();
            }

            ui::end_table();
        }
    }

private:
    void draw_pose_dropdown_button()
    {
        if (model_->is_readonly()) {
            ui::begin_disabled();
        }
        ui::draw_button("Pose " MSMICONS_CARET_DOWN);
        if (ui::begin_popup_context_menu("##PosePopup", ui::PopupFlag::MouseButtonLeft)) {
            // Draw a button that can be used to zero all coordinates, which can be a cheap way
            // of resetting a model's pose (#957).
            if (ui::draw_menu_item("Zero All Coordinates")) {
                action_zero_all_coordinates(*model_);
            }
            ui::end_popup();
        }
        if (model_->is_readonly()) {
            ui::end_disabled();
        }
    }

    void draw_row(const OpenSim::Coordinate& c)
    {
        ui::table_next_row();

        int column = 0;
        ui::table_set_column_index(column++);
        draw_name_cell(c);

        ui::table_set_column_index(column++);
        draw_data_cell(c);
        OSC_ASSERT_ALWAYS(
            c.hasOwner() &&
            "An `OpenSim::Coordinate` must always have an owner. This bug can occur when using is_free_to_satisfy_coordinates (see issue #888)");

        ui::table_set_column_index(column++);
        draw_speed_cell(c);
        OSC_ASSERT_ALWAYS(
            c.hasOwner() &&
            "An `OpenSim::Coordinate` must always have an owner. This bug can occur when using is_free_to_satisfy_coordinates (see issue #888)");
    }

    void draw_name_cell(const OpenSim::Coordinate& c)
    {
        int styles_pushed = 0;
        if (&c == model_->get_hovered()) {
            ui::push_style_color(ui::ColorVar::Text, OSCColors::hovered());
            ++styles_pushed;
        }
        if (&c == model_->get_selected()) {
            ui::push_style_color(ui::ColorVar::Text, OSCColors::selected());
            ++styles_pushed;
        }

        ui::draw_text(c.getName());
        ui::pop_style_color(std::exchange(styles_pushed, 0));

        if (ui::is_item_hovered()) {
            model_->set_hovered(&c);

            std::stringstream ss;
            ss << "    motion type = " << opyn::get_motion_type_display_name(c) << '\n';
            ss << "    owner = " << opyn::try_get_owner_name(c).value_or("(no owner)");

            ui::draw_tooltip(c.getName(), ss.str());
        }

        if (ui::is_item_clicked(MouseButton::Left)) {
            model_->set_selected(&c);
        } else if (ui::is_item_clicked(MouseButton::Right)) {
            auto popup = std::make_unique<ComponentContextMenu>(
                parent(), "##componentcontextmenu", model_, opyn::get_absolute_path(c));
            App::post_event<OpenPopupEvent>(*parent(), std::move(popup));
        }
    }

    void draw_data_cell(const OpenSim::Coordinate& c)
    {
        const bool disabled = model_->is_readonly();
        if (disabled) {
            ui::begin_disabled();
        }
        draw_data_cell_lock_button(c);
        ui::same_line(0.0f, 0.0f);
        draw_data_cell_coordinate_slider(c);
        if (disabled) {
            ui::end_disabled();
        }
    }

    void draw_data_cell_lock_button(const OpenSim::Coordinate& c)
    {
        ui::push_style_color(ui::ColorVar::Button, Color::clear());
        ui::push_style_color(ui::ColorVar::ButtonActive, Color::clear());
        ui::push_style_color(ui::ColorVar::ButtonHovered, Color::clear());
        ui::push_style_var(ui::StyleVar::FramePadding, {0.0f, ui::get_style_frame_padding().y()});
        if (ui::draw_button(c.getLocked(model_->get_state()) ? MSMICONS_LOCK : MSMICONS_UNLOCK)) {
            const bool new_value = !c.getLocked(model_->get_state());
            action_set_coordinate_locked_and_save(*model_, c, new_value);
        }
        ui::pop_style_var();
        ui::pop_style_color();
        ui::pop_style_color();
        ui::pop_style_color();
        ui::draw_tooltip_if_item_hovered(
            "Toggle Coordinate Lock",
            "Lock/unlock the coordinate's value.\n\nLocking a coordinate indicates whether the coordinate's value should be constrained to this value during the simulation.");
    }

    void draw_data_cell_coordinate_slider(const OpenSim::Coordinate& c)
    {
        const bool coordinate_locked = c.getLocked(model_->get_state());

        ui::set_next_item_width(ui::get_content_region_available().x());

        const float min_value = opyn::convert_coord_value_to_display_value(c, c.getRangeMin());
        const float max_value = opyn::convert_coord_value_to_display_value(c, c.getRangeMax());
        float displayed_value =
            opyn::convert_coord_value_to_display_value(c, c.getValue(model_->get_state()));

        if (coordinate_locked) {
            ui::push_style_var(ui::StyleVar::DisabledAlpha, 0.2f);
            ui::begin_disabled();
        }
        if (ui::draw_float_circular_slider(
                "##coordinatevalueeditor", &displayed_value, min_value, max_value)) {
            const double stored_value =
                opyn::convert_coord_display_value_to_storage_value(c, displayed_value);
            action_set_coordinate_value(*model_, c, stored_value);
        }
        if (coordinate_locked) {
            ui::end_disabled();
            ui::pop_style_var();
        }
        if (ui::is_item_deactivated_after_edit()) {
            const double stored_value =
                opyn::convert_coord_display_value_to_storage_value(c, displayed_value);
            action_set_coordinate_value_and_save(*model_, c, stored_value);
        }
        ui::draw_tooltip_body_only_if_item_hovered("Ctrl-click the slider to edit");
    }

    void draw_speed_cell(const OpenSim::Coordinate& c)
    {
        float displayed_speed =
            opyn::convert_coord_value_to_display_value(c, c.getSpeedValue(model_->get_state()));

        ui::set_next_item_width(ui::get_content_region_available().x());
        if (ui::draw_float_meters_input("##coordinatespeededitor", displayed_speed)) {
            const double stored_speed =
                opyn::convert_coord_display_value_to_storage_value(c, displayed_speed);
            action_set_coordinate_speed(*model_, c, stored_speed);
        }

        if (ui::is_item_deactivated_after_edit()) {
            const double stored_speed =
                opyn::convert_coord_display_value_to_storage_value(c, displayed_speed);
            action_set_coordinate_speed_and_save(*model_, c, stored_speed);
        }
    }

    std::shared_ptr<opyn::ModelStatePair> model_;
};

osc::CoordinateEditorPanel::CoordinateEditorPanel(
    Widget* parent, std::string_view panel_name, std::shared_ptr<opyn::ModelStatePair> model) :

    Panel{std::make_unique<Impl>(*this, parent, panel_name, std::move(model))}
{}
void osc::CoordinateEditorPanel::impl_draw_content()
{
    private_data().draw_content();
}
