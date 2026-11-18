#include "geometry_path_editor_popup.h"

#include <libopensimcreator/platform/msmicons.h>

#include <libopynsim/documents/model/component_accessor.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/popups/popup.h>
#include <liboscar/ui/popups/popup_private.h>
#include <liboscar/utilities/c_string_view.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/AbstractPathPoint.h>
#include <OpenSim/Simulation/Model/GeometryPath.h>
#include <OpenSim/Simulation/Model/PathPoint.h>

#include <array>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>

using namespace osc;

namespace
{
    constexpr auto c_location_input_i_ds =
        std::to_array<CStringView>({"##xinput", "##yinput", "##zinput"});
    static_assert(c_location_input_i_ds.size() == 3);

    OpenSim::GeometryPath copy_or_default_geometry_path(
        const std::function<const OpenSim::GeometryPath*()>& accessor)
    {
        const OpenSim::GeometryPath* maybe_geom_path = accessor();
        if (maybe_geom_path) {
            return *maybe_geom_path;
        } else {
            return OpenSim::GeometryPath{};
        }
    }

    struct RequestedAction final {
        enum class Type {
            MoveUp,
            MoveDown,
            Delete,
            None,
        };

        RequestedAction() = default;

        RequestedAction(Type type, ptrdiff_t path_point_index) :

            type{type},
            pathPointIndex{path_point_index}
        {}

        void reset() { *this = {}; }

        Type type = Type::None;
        ptrdiff_t pathPointIndex = -1;
    };

    void action_move_path_point_up(OpenSim::PathPointSet& pps, ptrdiff_t i)
    {
        if (1 <= i && i < opyn::ssize(pps)) {
            auto tmp = opyn::clone(opyn::at(pps, i));
            opyn::assign(pps, i, opyn::at(pps, i - 1));
            opyn::assign(pps, i - 1, std::move(tmp));
        }
    }

    void action_move_path_point_down(OpenSim::PathPointSet& pps, ptrdiff_t i)
    {
        if (0 <= i && i < opyn::ssize(pps) - 1) {
            auto tmp = opyn::clone(opyn::at(pps, i));
            opyn::assign(pps, i, opyn::at(pps, i + 1));
            opyn::assign(pps, i + 1, std::move(tmp));
        }
    }

    void action_delete_path_point(OpenSim::PathPointSet& pps, ptrdiff_t i)
    {
        if (0 <= i and i < opyn::ssize(pps)) {
            opyn::erase_at(pps, i);
        }
    }

    void action_set_path_point_frame_path(
        OpenSim::PathPointSet& pps, ptrdiff_t i, const std::string& frame_abs_path)
    {
        opyn::at(pps, i).updSocket("parent_frame").setConnecteePath(frame_abs_path);
    }

    void action_add_new_path_point(OpenSim::PathPointSet& pps)
    {
        const std::string frame =
            opyn::empty(pps)
                ? "/ground"
                : opyn::at(pps, opyn::size(pps) - 1).getSocket("parent_frame").getConnecteePath();

        auto pp = std::make_unique<OpenSim::PathPoint>();
        pp->updSocket("parent_frame").setConnecteePath(frame);

        opyn::append(pps, std::move(pp));
    }
}

class osc::GeometryPathEditorPopup::Impl final : public PopupPrivate {
public:
    explicit Impl(
        GeometryPathEditorPopup& owner,
        Widget* parent,
        std::string_view popup_name,
        std::shared_ptr<const opyn::ComponentAccessor> target_component,
        std::function<const OpenSim::GeometryPath*()> geometry_path_getter,
        std::function<void(const OpenSim::GeometryPath&)> on_local_copy_edited) :

        PopupPrivate{owner, parent, popup_name, {768.0f, 0.0f}, ui::PanelFlag::AlwaysAutoResize},
        target_component_{std::move(target_component)},
        geometry_path_getter_{std::move(geometry_path_getter)},
        on_local_copy_edited_{std::move(on_local_copy_edited)},
        edited_geometry_path_{copy_or_default_geometry_path(geometry_path_getter_)}
    {}

    void draw_content()
    {
        if (geometry_path_getter_() == nullptr) {
            // edge-case: the geometry path that this popup is editing no longer
            // exists (e.g. because a muscle was deleted or similar), so it should
            // announce the problem and close itself
            ui::draw_text("The GeometryPath no longer exists - closing this popup");
            request_close();
            return;
        }
        // else: the geometry path exists, but this UI should edit the cached
        // `m_EditedGeometryPath`, which is independent of the original data
        // and the target component (so that edits can be applied transactionally)

        ui::draw_text("Path Points:");
        ui::draw_separator();
        draw_path_point_editor_table();
        ui::draw_separator();
        draw_add_path_point_button();
        ui::start_new_line();
        draw_bottom_buttons();
    }

private:
    void draw_path_point_editor_table()
    {
        OpenSim::PathPointSet& pps = edited_geometry_path_.updPathPointSet();

        if (ui::begin_table("##GeometryPathEditorTable", 6)) {
            ui::table_setup_column("Actions");
            ui::table_setup_column("Type");
            ui::table_setup_column("X");
            ui::table_setup_column("Y");
            ui::table_setup_column("Z");
            ui::table_setup_column("Frame");
            ui::table_setup_scroll_freeze(0, 1);
            ui::table_headers_row();

            for (ptrdiff_t i = 0; i < opyn::ssize(pps); ++i) {
                ui::push_id(i);
                draw_ith_path_point_table_row(pps, i);
                ui::pop_id();
            }

            ui::end_table();
        }

        // perform any actions after rendering the table: in case the action would
        // in some way screw with rendering (e.g. deleting a point midway
        // through rendering a row is probably a bad idea)
        try_execute_requested_action(pps);
    }

    void draw_add_path_point_button()
    {
        if (ui::draw_button(MSMICONS_PLUS_CIRCLE " Add Point")) {
            action_add_new_path_point(edited_geometry_path_.updPathPointSet());
        }
    }

    void draw_ith_path_point_table_row(OpenSim::PathPointSet& pps, ptrdiff_t i)
    {
        int column = 0;

        ui::table_next_row();

        ui::table_set_column_index(column++);
        draw_ith_path_point_actions_cell(pps, i);

        ui::table_set_column_index(column++);
        draw_ith_path_point_type_cell(pps, i);

        try_draw_ith_path_point_location_editor_cells(pps, i, column);

        ui::table_set_column_index(column++);
        draw_ith_path_point_frame_cell(pps, i);
    }

    void draw_ith_path_point_actions_cell(OpenSim::PathPointSet& pps, ptrdiff_t i)
    {
        ui::push_style_var(ui::StyleVar::ItemSpacing, {2.0f, 0.0f});

        if (i <= 0) {
            ui::begin_disabled();
        }
        if (ui::draw_small_button(MSMICONS_ARROW_UP)) {
            requested_action_ = RequestedAction{RequestedAction::Type::MoveUp, i};
        }
        if (i <= 0) {
            ui::end_disabled();
        }

        ui::same_line();

        if (i + 1 >= opyn::ssize(pps)) {
            ui::begin_disabled();
        }
        if (ui::draw_small_button(MSMICONS_ARROW_DOWN)) {
            requested_action_ = RequestedAction{RequestedAction::Type::MoveDown, i};
        }
        if (i + 1 >= opyn::ssize(pps)) {
            ui::end_disabled();
        }

        ui::same_line();

        ui::push_style_color(ui::ColorVar::Text, Color{0.7f, 0.0f, 0.0f});
        if (ui::draw_small_button(MSMICONS_TRASH)) {
            requested_action_ = RequestedAction{RequestedAction::Type::Delete, i};
        }
        ui::pop_style_color();

        ui::pop_style_var();
    }

    void draw_ith_path_point_type_cell(const OpenSim::PathPointSet& pps, ptrdiff_t i)
    {
        ui::draw_text_disabled(opyn::at(pps, i).getConcreteClassName());
    }

    // try, because the path point type might not actually have a set location
    //
    // (e.g. `MovingPathPoint`s)
    void try_draw_ith_path_point_location_editor_cells(
        OpenSim::PathPointSet& pps, ptrdiff_t i, int& column)
    {
        OpenSim::AbstractPathPoint& app = opyn::at(pps, i);

        if (auto* const pp = dynamic_cast<OpenSim::PathPoint*>(&app)) {
            const float input_width = ui::calc_text_size("0.00000").x();

            SimTK::Vec3& location = pp->upd_location();

            static_assert(c_location_input_i_ds.size() == 3);
            for (size_t dim = 0; dim < c_location_input_i_ds.size(); ++dim) {
                auto v = static_cast<float>(location[static_cast<int>(dim)]);

                ui::table_set_column_index(column++);
                ui::set_next_item_width(input_width);
                if (ui::draw_float_input(c_location_input_i_ds[dim], &v)) {
                    location[static_cast<int>(dim)] = static_cast<double>(v);
                }
            }
        } else {
            // it's some other kind of path point, with no editable X, Y, or Z
            ui::table_set_column_index(column++);
            ui::table_set_column_index(column++);
            ui::table_set_column_index(column++);
        }
    }

    void draw_ith_path_point_frame_cell(OpenSim::PathPointSet& pps, ptrdiff_t i)
    {
        const float width = ui::calc_text_size("/bodyset/a_typical_body_name").x();

        const std::string& label = opyn::at(pps, i).getSocket("parent_frame").getConnecteePath();

        ui::set_next_item_width(width);
        if (ui::begin_combobox("##framesel", label)) {
            for (const OpenSim::Frame& frame :
                 target_component_->get_component().getComponentList<OpenSim::Frame>()) {

                const std::string abs_path = frame.getAbsolutePathString();
                if (ui::draw_selectable(abs_path)) {
                    action_set_path_point_frame_path(pps, i, abs_path);
                }
            }
            ui::end_combobox();
        }
    }

    void draw_bottom_buttons()
    {
        if (ui::draw_button("cancel")) {
            request_close();
        }

        ui::same_line();

        if (ui::draw_button("save")) {
            on_local_copy_edited_(edited_geometry_path_);
            request_close();
        }
    }

    void try_execute_requested_action(OpenSim::PathPointSet& pps)
    {
        if (not(0 <= requested_action_.pathPointIndex and
                requested_action_.pathPointIndex < opyn::ssize(pps))) {

            // edge-case: if the index is out of range, ignore the action
            requested_action_.reset();
            return;
        }

        switch (requested_action_.type) {
        case RequestedAction::Type::MoveUp:
            action_move_path_point_up(pps, requested_action_.pathPointIndex);
            break;
        case RequestedAction::Type::MoveDown:
            action_move_path_point_down(pps, requested_action_.pathPointIndex);
            break;
        case RequestedAction::Type::Delete:
            action_delete_path_point(pps, requested_action_.pathPointIndex);
            break;
        default: break;  // (unhandled/do nothing)
        }

        requested_action_.reset();  // action handled: resets
    }

    std::shared_ptr<const opyn::ComponentAccessor> target_component_;
    std::function<const OpenSim::GeometryPath*()> geometry_path_getter_;
    std::function<void(const OpenSim::GeometryPath&)> on_local_copy_edited_;

    OpenSim::GeometryPath edited_geometry_path_;
    RequestedAction requested_action_;
};

osc::GeometryPathEditorPopup::GeometryPathEditorPopup(
    Widget* parent,
    std::string_view popup_name,
    std::shared_ptr<const opyn::ComponentAccessor> target_component,
    std::function<const OpenSim::GeometryPath*()> geometry_path_getter,
    std::function<void(const OpenSim::GeometryPath&)> on_local_copy_edited) :

    Popup{std::make_unique<Impl>(
        *this,
        parent,
        popup_name,
        std::move(target_component),
        std::move(geometry_path_getter),
        std::move(on_local_copy_edited))}
{}
void osc::GeometryPathEditorPopup::impl_draw_content()
{
    private_data().draw_content();
}
