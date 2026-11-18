#include "component_context_menu.h"

#include <libopensimcreator/documents/model/environment.h>
#include <libopensimcreator/documents/model/model_state_pair_with_shared_environment.h>
#include <libopensimcreator/documents/model/undoable_model_actions.h>
#include <libopensimcreator/ui/model_editor/reassign_socket_popup.h>
#include <libopensimcreator/ui/shared/add_muscle_plot_event.h>
#include <libopensimcreator/ui/shared/basic_widgets.h>
#include <libopensimcreator/ui/shared/model_add_menu_items.h>

#include <libopynsim/component_registry/component_registry.h>
#include <libopynsim/component_registry/static_component_registries.h>
#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/documents/output_extractors/component_output_extractor.h>
#include <libopynsim/documents/output_extractors/shared_output_extractor.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/os.h>
#include <liboscar/platform/widget.h>
#include <liboscar/ui/events/open_named_panel_event.h>
#include <liboscar/ui/events/open_popup_event.h>
#include <liboscar/ui/icon_cache.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/popups/popup.h>
#include <liboscar/ui/popups/popup_private.h>
#include <liboscar/utilities/assertions.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Common/Object.h>
#include <OpenSim/Common/Set.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>

#include <memory>
#include <sstream>
#include <string>
#include <utility>

using namespace osc;

// helpers
namespace
{
    // draw UI element that lets user change a model joint's type
    void draw_selection_joint_type_switcher(
        opyn::ModelStatePair& model, const OpenSim::ComponentPath& joint_path)
    {
        const auto* joint = opyn::find_component<OpenSim::Joint>(model.get_model(), joint_path);
        if (not joint) {
            return;
        }

        const auto& registry = opyn::get_component_registry<OpenSim::Joint>();

        std::optional<ptrdiff_t> selected_idx;
        if (ui::begin_menu("Change Joint Type", model.can_upd_model())) {
            // look the Joint up in the type registry so we know where it should be in the
            // ui::draw_combobox
            std::optional<size_t> maybe_type_index = index_of(registry, *joint);

            for (ptrdiff_t i = 0; i < std::ssize(registry); ++i) {
                bool selected = i == maybe_type_index;
                bool was_selected = selected;

                if (ui::draw_menu_item(registry[i].name(), {}, &selected)) {
                    if (not was_selected) {
                        selected_idx = i;
                    }
                }
            }
            ui::end_menu();
        }

        if (selected_idx and *selected_idx < std::ssize(registry)) {
            // copy + fixup  a prototype of the user's selection
            action_change_joint_type_to(model, joint_path, registry[*selected_idx].instantiate());
        }
    }

    // draw contextual actions (buttons, sliders) for a selected physical frame
    void draw_physical_frame_contextual_actions(
        Widget&,
        const std::shared_ptr<opyn::ModelStatePair>& model_state,
        const OpenSim::ComponentPath& pf_path)
    {
        if (const auto* pf =
                opyn::find_component<OpenSim::PhysicalFrame>(model_state->get_model(), pf_path)) {
            draw_calculate_menu(
                model_state->get_model(),
                model_state->get_state(),
                *pf,
                CalculateMenuFlags::NoCalculatorIcon);
        }
    }

    // draw contextual actions (buttons, sliders) for a selected joint
    void draw_joint_contextual_actions(
        opyn::ModelStatePair& model_state, const OpenSim::ComponentPath& joint_path)
    {
        draw_selection_joint_type_switcher(model_state, joint_path);

        if (ui::draw_menu_item(
                "Rezero Joint", {}, nullptr, can_rezero_joint(model_state, joint_path))) {
            action_rezero_joint(model_state, joint_path);
        }
        ui::draw_tooltip_if_item_hovered(
            "Re-zero the joint",
            "Given the joint's current geometry due to joint defaults, coordinate defaults, and any coordinate edits made in the coordinates panel, this will reorient the joint's parent (if it's an offset frame) to match the child's transformation. Afterwards, it will then resets all of the joints coordinates to zero. This effectively sets the 'zero point' of the joint (i.e. the geometry when all coordinates are zero) to match whatever the current geometry is.");
    }

    void draw_station_contextual_actions(
        const opyn::ModelStatePair& model_state, const OpenSim::Station& station)
    {
        draw_calculate_menu(
            model_state.get_model(),
            model_state.get_state(),
            station,
            CalculateMenuFlags::NoCalculatorIcon);
    }

    void draw_marker_contextual_actions(
        opyn::ModelStatePair& model_state, const OpenSim::Marker& marker)
    {
        draw_calculate_menu(
            model_state.get_model(),
            model_state.get_state(),
            marker,
            CalculateMenuFlags::NoCalculatorIcon);

        // Show a specialized `Move To` menu that lets users move the marker to
        // the model's `MarkerSet`, which can be required for backwards compatibility
        // with OpenSim GUI (#1102).
        if (ui::begin_menu("Move To", not model_state.is_readonly())) {

            // Only enable this option if the marker isn't already part of the model's `MarkerSet`
            // (otherwise, we assume it's remove-able from its current owner).
            const OpenSim::Component* owner = opyn::get_owner<OpenSim::MarkerSet>(marker);
            bool disabled = (owner != nullptr) and
                            opyn::get_owner<OpenSim::Model>(*owner) == &model_state.get_model();

            if (ui::draw_menu_item("/markerset", std::nullopt, nullptr, not disabled)) {
                action_move_marker_to_model_marker_set(model_state, marker);
            }
            if (disabled and ui::is_item_hovered(ui::HoveredFlag::AllowWhenDisabled)) {
                ui::draw_tooltip_body_only("This marker is already part of /markerset");
            }

            ui::end_menu();
        }
    }

    void draw_point_contextual_actions(
        const opyn::ModelStatePair& model_state, const OpenSim::Point& point)
    {
        draw_calculate_menu(
            model_state.get_model(),
            model_state.get_state(),
            point,
            CalculateMenuFlags::NoCalculatorIcon);
    }

    void draw_ellipsoid_contextual_actions(
        const opyn::ModelStatePair& model_state, const OpenSim::Ellipsoid& ellipsoid)
    {
        draw_calculate_menu(
            model_state.get_model(),
            model_state.get_state(),
            ellipsoid,
            CalculateMenuFlags::NoCalculatorIcon);
    }

    void draw_mesh_contextual_actions(opyn::ModelStatePair& model_state, const OpenSim::Mesh& mesh)
    {
        if (ui::begin_menu("Fit Analytic Geometry", model_state.can_upd_model())) {
            ui::draw_help_marker(
                "Uses shape-fitting algorithms to fit analytic geometry to the points in the given mesh.\n\nThe 'htbad'-suffixed algorithms were adapted (potentially, with bugs - report them) from the MATLAB code in:\n\n        Bishop P., How to build a dinosaur..., doi:10.1017/pab.2020.46");

            if (ui::draw_menu_item("Sphere (htbad)", {}, nullptr, model_state.can_upd_model())) {
                action_fit_sphere_to_mesh(model_state, mesh);
            }

            if (ui::draw_menu_item("Ellipsoid (htbad)", {}, nullptr, model_state.can_upd_model())) {
                action_fit_ellipsoid_to_mesh(model_state, mesh);
            }

            if (ui::draw_menu_item("Plane (htbad)", {}, nullptr, model_state.can_upd_model())) {
                action_fit_plane_to_mesh(model_state, mesh);
            }

            ui::end_menu();
        }

        if (ui::begin_menu("Export")) {
            draw_mesh_export_context_menu_content(model_state, mesh);
            ui::end_menu();
        }
    }

    void draw_geometry_contextual_actions(
        const opyn::ModelStatePair& model_state, const OpenSim::Geometry& geometry)
    {
        draw_calculate_menu(
            model_state.get_model(),
            model_state.get_state(),
            geometry,
            CalculateMenuFlags::NoCalculatorIcon);
    }

    bool any_descendent_inclusive_has_appearance_property(const OpenSim::Component& component)
    {
        const OpenSim::Component* const c = opyn::find_first_descendent_inclusive(
            component, [](const OpenSim::Component& desc) -> bool {
                return opyn::try_get_appearance(desc) != nullptr;
            });
        return c != nullptr;
    }
}

class osc::ComponentContextMenu::Impl final : public PopupPrivate {
public:
    explicit Impl(
        ComponentContextMenu& owner,
        Widget* parent,
        std::string_view popup_name,
        std::shared_ptr<opyn::ModelStatePair> model,
        OpenSim::ComponentPath path,
        ComponentContextMenuFlags flags) :

        PopupPrivate{owner, parent, popup_name, {10.0f, 10.0f}, ui::PanelFlag::NoMove},
        model_{std::move(model)},
        path_{std::move(path)},
        flags_{flags}
    {
        set_modal(false);
        OSC_ASSERT(model_ != nullptr);
    }

    void draw_content()
    {
        const OpenSim::Component* c = opyn::find_component(model_->get_model(), path_);
        if (not c) {
            // draw context menu content that's shown when nothing was right-clicked
            draw_nothing_right_clicked_context_menu_header();
            draw_context_menu_separator();
            if (ui::begin_menu("Add", model_->can_upd_model())) {
                // i.e. the target parent component should default to the model
                model_add_menu_items_.set_target_parent_component({});
                model_add_menu_items_.on_draw();
                ui::end_menu();
            }

            // draw a display menu to match the display menu that appears when right-clicking
            // something, but this display menu only contains the functionality to show everything
            // in the model
            //
            // it's handy when users have selectively hidden this-or-that, or have hidden everything
            // in the model (#422)
            if (ui::begin_menu("Display", model_->can_upd_model())) {
                if (ui::draw_menu_item("Show All")) {
                    action_set_component_and_all_childrens_is_visible_to(
                        *model_, opyn::get_root_component_path(), true);
                }
                ui::draw_tooltip_if_item_hovered(
                    "Show All",
                    "Sets the visiblity of all components within the model to 'visible', handy for undoing selective hiding etc.");
                ui::draw_vertical_spacer(0.5f);
                ui::draw_text_disabled("Model Visual Preferences");
                ui::draw_separator();
                draw_all_decoration_toggle_buttons(*model_, *icon_cache_);
                ui::end_menu();
            }
            if (ui::begin_menu("Watch Output", false)) {
                ui::end_menu();
            }
            if (ui::begin_menu("Sockets", false)) {
                ui::end_menu();
            }
            if (ui::begin_menu("Copy", false)) {
                ui::end_menu();
            }
            return;
        }

        draw_right_clicked_component_context_menu_header(*c);
        draw_context_menu_separator();

        if (ui::begin_menu("Add", model_->can_upd_model())) {
            model_add_menu_items_.set_target_parent_component(path_);
            model_add_menu_items_.on_draw();
            ui::end_menu();
        }

        if (ui::begin_menu("Display", model_->can_upd_model())) {
            draw_display_menu_content(*c);
            ui::end_menu();
        }

        if (auto* model_with_env =
                dynamic_cast<ModelStatePairWithSharedEnvironment*>(model_.get())) {
            draw_watch_output_menu(
                *c, [this, model_with_env](const opyn::SharedOutputExtractor& output_extractor) {
                    model_with_env->try_upd_environment()->add_user_output_extractor(
                        output_extractor);

                    // when the user asks to watch an output, make sure the "Output Watches" panel
                    // is open, so that they can immediately see the side-effect of watching an
                    // output (#567)
                    App::post_event<OpenNamedPanelEvent>(owner(), "Output Watches");
                });
        }

        draw_socket_menu(*c);

        if (ui::begin_menu("Copy")) {
            if (ui::draw_menu_item("Name to Clipboard")) {
                set_clipboard_text(c->getName());
            }
            if (ui::draw_menu_item("Absolute Path to Clipboard")) {
                set_clipboard_text(opyn::get_absolute_path_string(*c));
            }
            ui::draw_tooltip_if_item_hovered(
                "Copy Component Absolute Path",
                "Copy the absolute path to this component to your clipboard.\n\n(This is handy if you are separately using absolute component paths to (e.g.) manipulate the model in a script or something)");
            if (ui::draw_menu_item("Concrete Class Name to Clipboard")) {
                set_clipboard_text(c->getConcreteClassName());
            }
            if (ui::draw_menu_item("Component XML to Clipboard")) {
                set_clipboard_text(opyn::write_object_xml_to_string(*c));
            }
            ui::end_menu();
        }

        if (dynamic_cast<const OpenSim::PhysicalFrame*>(c)) {
            ui::draw_separator();
            draw_physical_frame_contextual_actions(owner(), model_, path_);
        } else if (dynamic_cast<const OpenSim::Joint*>(c)) {
            ui::draw_separator();
            draw_joint_contextual_actions(*model_, path_);
        } else if (const auto* muscle_ptr = dynamic_cast<const OpenSim::Muscle*>(c)) {
            ui::draw_separator();
            draw_plot_vs_coordinate_menu(*muscle_ptr);
        } else if (const auto* marker_ptr = dynamic_cast<const OpenSim::Marker*>(c)) {
            ui::draw_separator();
            draw_marker_contextual_actions(*model_, *marker_ptr);
        } else if (const auto* station_ptr = dynamic_cast<const OpenSim::Station*>(c)) {
            ui::draw_separator();
            draw_station_contextual_actions(*model_, *station_ptr);
        } else if (const auto* point_ptr = dynamic_cast<const OpenSim::Point*>(c)) {
            ui::draw_separator();
            draw_point_contextual_actions(*model_, *point_ptr);
        } else if (const auto* ellipsoid_ptr = dynamic_cast<const OpenSim::Ellipsoid*>(c)) {
            ui::draw_separator();
            draw_ellipsoid_contextual_actions(*model_, *ellipsoid_ptr);
        } else if (const auto* mesh_ptr = dynamic_cast<const OpenSim::Mesh*>(c)) {
            ui::draw_separator();
            draw_mesh_contextual_actions(*model_, *mesh_ptr);
        } else if (const auto* geom_ptr = dynamic_cast<const OpenSim::Geometry*>(c)) {
            ui::draw_separator();
            draw_geometry_contextual_actions(*model_, *geom_ptr);
        }
    }

private:
    void draw_display_menu_content(const OpenSim::Component& c)
    {
        const bool is_enabled =
            model_->can_upd_model() and any_descendent_inclusive_has_appearance_property(c);

        // togges that are specific to this components (+ its descendants)

        if (ui::draw_menu_item("Show", {}, nullptr, is_enabled)) {
            action_set_component_and_all_childrens_is_visible_to(
                *model_, opyn::get_absolute_path(c), true);
        }

        if (ui::draw_menu_item("Show Only This", {}, nullptr, is_enabled)) {
            action_show_only_component_and_all_children(*model_, opyn::get_absolute_path(c));
        }

        if (ui::draw_menu_item("Hide", {}, nullptr, is_enabled)) {
            action_set_component_and_all_childrens_is_visible_to(
                *model_, opyn::get_absolute_path(c), false);
        }

        // add a seperator between probably commonly-used, simple, diplay toggles and the more
        // advanced ones
        ui::draw_separator();

        // redundantly put a "Show All" option here, also, so that the user doesn't have
        // to "know" that they need to right-click in the middle of nowhere or on the
        // model
        if (ui::draw_menu_item("Show All", {}, nullptr, is_enabled)) {
            action_set_component_and_all_childrens_is_visible_to(
                *model_, opyn::get_root_component_path(), true);
        }

        {
            std::stringstream ss;
            ss << "Show All '" << c.getConcreteClassName() << "' Components";
            const std::string label = std::move(ss).str();
            if (ui::draw_menu_item(label, {}, nullptr, is_enabled)) {
                action_set_component_and_all_children_with_given_concrete_class_name_is_visible_to(
                    *model_,
                    opyn::get_absolute_path(model_->get_model()),
                    c.getConcreteClassName(),
                    true);
            }
        }

        {
            std::stringstream ss;
            ss << "Hide All '" << c.getConcreteClassName() << "' Components";
            const std::string label = std::move(ss).str();
            if (ui::draw_menu_item(label, {}, nullptr, is_enabled)) {
                action_set_component_and_all_children_with_given_concrete_class_name_is_visible_to(
                    *model_,
                    opyn::get_absolute_path(model_->get_model()),
                    c.getConcreteClassName(),
                    false);
            }
        }

        ui::draw_vertical_spacer(0.5f);
        ui::draw_text_disabled("Model Visual Preferences");
        ui::draw_separator();
        draw_all_decoration_toggle_buttons(*model_, *icon_cache_);
    }

    void draw_socket_menu(const OpenSim::Component& c)
    {
        if (ui::begin_menu("Sockets", model_->can_upd_model())) {

            ui::draw_text_centered("Outbound Sockets");
            ui::draw_separator();
            draw_outbound_sockets_info(c);

            ui::start_new_line();

            ui::draw_text_centered("Inbound Connections");
            ui::draw_separator();
            draw_inbound_connections_info(c);

            ui::end_menu();
        }
    }

    void draw_outbound_sockets_info(const OpenSim::Component& c)
    {
        if (c.getNumSockets() != 0) {
            draw_outbound_sockets_table(c);
        } else {
            ui::draw_dummy({256.0f, 0.0f});
            ui::draw_text_disabled_and_centered(c.getName() + " has no outbound sockets.");
        }
    }

    void draw_outbound_sockets_table(const OpenSim::Component& c)
    {
        const std::vector<std::string> socket_names = opyn::get_socket_names(c);
        ui::push_style_var(
            ui::StyleVar::CellPadding, ui::get_text_line_height_in_current_panel() * Vector2{0.5f});

        if (ui::begin_table(
                "outbound sockets table",
                4,
                {ui::TableFlag::SizingStretchProp,
                 ui::TableFlag::BordersInner,
                 ui::TableFlag::PadOuterX})) {
            ui::table_setup_column("Socket Name");
            ui::table_setup_column("Connectee Type");
            ui::table_setup_column("Connectee");
            ui::table_setup_column("Actions");

            ui::table_headers_row();

            int id = 0;
            for (const std::string& socket_name : socket_names) {
                const OpenSim::AbstractSocket& socket = c.getSocket(socket_name);

                int column = 0;
                ui::push_id(id++);
                ui::table_next_row();

                ui::table_set_column_index(column++);
                ui::draw_text_disabled(socket_name);

                ui::table_set_column_index(column++);
                ui::draw_text(socket.getConnecteeTypeName());

                ui::table_set_column_index(column++);
                if (ui::draw_small_button(socket.getConnecteeAsObject().getName())) {
                    model_->set_selected(
                        dynamic_cast<const OpenSim::Component*>(&socket.getConnecteeAsObject()));
                    request_close();
                }
                if (const auto* connectee =
                        dynamic_cast<const OpenSim::Component*>(&socket.getConnecteeAsObject());
                    connectee && ui::is_item_hovered()) {

                    draw_component_hover_tooltip(*connectee);
                }

                ui::table_set_column_index(column++);
                if (ui::draw_small_button("change")) {
                    auto popup = std::make_unique<ReassignSocketPopup>(
                        &owner(),
                        "Reassign " + socket.getName(),
                        model_,
                        opyn::get_absolute_path_string(c),
                        socket_name);
                    App::post_event<OpenPopupEvent>(owner(), std::move(popup));
                }

                ui::pop_id();
            }

            ui::end_table();
        }
        ui::pop_style_var();
    }

    void draw_inbound_connections_info(const OpenSim::Component& c)
    {
        const auto filter = should_filter_inbound_connections_ ?
            [](const OpenSim::Component& c) { return opyn::should_show_in_ui(c) and dynamic_cast<const OpenSim::FrameGeometry*>(&c) == nullptr; } :
            [](const OpenSim::Component&)   { return true; };

        auto els = opyn::for_each_inbound_connection(&model_->get_model(), &c, filter);
        auto it = els.begin();
        const auto end = els.end();

        if (it != end) {
            draw_inbound_connections_table(it, end);
        } else {
            ui::draw_dummy({256.0f, 0.0f});
            ui::draw_text_disabled_and_centered(c.getName() + " has no inbound sockets.");
        }

        ui::indent();
        ui::draw_checkbox("Hide Junk", &should_filter_inbound_connections_);
        ui::unindent();
    }

    void draw_inbound_connections_table(auto& it, std::default_sentinel_t end)
    {
        ui::push_style_var(
            ui::StyleVar::CellPadding, ui::get_text_line_height_in_current_panel() * Vector2{0.5f});
        const ui::TableFlags flags = {
            ui::TableFlag::SizingStretchProp,
            ui::TableFlag::BordersInner,
            ui::TableFlag::PadOuterX,
            ui::TableFlag::ScrollY};
        const Vector2 dimensions =
            Vector2{0.0f, 10.0f * ui::get_text_line_height_with_spacing_in_current_panel()};
        if (ui::begin_table("inbound connections table", 3, flags, dimensions)) {
            ui::table_setup_column("Source Component");
            ui::table_setup_column("Socket Name");
            ui::table_setup_column("Actions");

            ui::table_headers_row();

            // draw data rows
            int id = 0;
            for (; it != end; ++it) {
                const opyn::ComponentConnectionView view{*it};

                int column = 0;
                ui::push_id(id++);
                ui::table_next_row();

                // column: Source Component
                ui::table_set_column_index(column++);
                if (ui::draw_small_button(view.source().getName())) {
                    model_->set_selected(dynamic_cast<const OpenSim::Component*>(&view.source()));
                    request_close();
                }
                if (ui::is_item_hovered()) {
                    draw_component_hover_tooltip(view.source());
                }

                // column: Socket Name
                ui::table_set_column_index(column++);
                ui::draw_text_disabled(view.socket_name());

                // column: actions
                ui::table_set_column_index(column++);
                if (ui::draw_small_button("change")) {
                    auto popup = std::make_unique<ReassignSocketPopup>(
                        &owner(),
                        "Reassign " + view.socket_name(),
                        model_,
                        opyn::get_absolute_path_string(view.source()),
                        view.socket_name());
                    App::post_event<OpenPopupEvent>(owner(), std::move(popup));
                }

                ui::pop_id();
            }

            ui::end_table();
        }
        ui::pop_style_var();
    }

    void draw_plot_vs_coordinate_menu(const OpenSim::Muscle& m)
    {
        if (flags_ & ComponentContextMenuFlag::NoPlotVsCoordinate) {
            return;
        }
        if (ui::begin_menu("Plot vs. Coordinate")) {
            for (const OpenSim::Coordinate& c :
                 model_->get_model().getComponentList<OpenSim::Coordinate>()) {

                if (ui::draw_menu_item(c.getName())) {
                    App::post_event<AddMusclePlotEvent>(owner(), c, m);
                }
            }

            ui::end_menu();
        }
    }

    std::shared_ptr<opyn::ModelStatePair> model_;
    OpenSim::ComponentPath path_;
    ModelAddMenuItems model_add_menu_items_{&owner(), model_};
    ComponentContextMenuFlags flags_;
    bool should_filter_inbound_connections_ = true;
    std::shared_ptr<IconCache> icon_cache_ = App::singleton<IconCache>(
        App::resource_loader().with_prefix("OpenSimCreator/icons/"),
        ui::get_font_base_size() / 128.0f,
        App::get().highest_device_pixel_ratio());
};

osc::ComponentContextMenu::ComponentContextMenu(
    Widget* parent,
    std::string_view popup_name,
    std::shared_ptr<opyn::ModelStatePair> model,
    const OpenSim::ComponentPath& path,
    ComponentContextMenuFlags flags) :

    Popup{std::make_unique<Impl>(*this, parent, popup_name, std::move(model), path, flags)}
{}
void osc::ComponentContextMenu::impl_draw_content()
{
    private_data().draw_content();
}
