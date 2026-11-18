#include "add_component_popup.h"

#include <libopensimcreator/documents/model/undoable_model_actions.h>
#include <libopensimcreator/documents/model/undoable_model_state_pair.h>
#include <libopensimcreator/platform/msmicons.h>
#include <libopensimcreator/ui/shared/basic_widgets.h>
#include <libopensimcreator/ui/shared/object_properties_editor.h>

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/graphics/color.h>
#include <liboscar/platform/app.h>
#include <liboscar/shims/cpp23/ranges.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/popups/popup.h>
#include <liboscar/ui/popups/popup_private.h>
#include <liboscar/utilities/exception_helpers.h>
#include <liboscar/utilities/string_helpers.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Simulation/Model/AbstractPathPoint.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PathActuator.h>
#include <OpenSim/Simulation/Model/PathPoint.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <SimTKcommon/SmallMatrix.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace OpenSim
{
    class AbstractProperty;
}

namespace rgs = std::ranges;

namespace
{
    struct PathPoint final {

        PathPoint(
            OpenSim::ComponentPath user_choice,
            OpenSim::ComponentPath actual_frame,
            const SimTK::Vec3& location_in_frame) :

            userChoice{std::move(user_choice)},
            actualFrame{std::move(actual_frame)},
            locationInFrame{location_in_frame}
        {}

        // what the user chose when the clicked in the UI
        OpenSim::ComponentPath userChoice;

        // what the actual frame is that will be attached to
        //
        // (can be different from user choice because the user is permitted to click a station)
        OpenSim::ComponentPath actualFrame;

        // location of the point within the frame
        SimTK::Vec3 locationInFrame;
    };
}

class osc::AddComponentPopup::Impl final : public PopupPrivate {
public:
    explicit Impl(
        AddComponentPopup& owner,
        Widget* parent,
        std::string_view popup_name,
        std::shared_ptr<opyn::ModelStatePair> model,
        std::unique_ptr<OpenSim::Component> prototype,
        OpenSim::ComponentPath target_component) :

        PopupPrivate{owner, parent, popup_name},
        model_{std::move(model)},
        proto_{std::move(prototype)},
        prototype_properties_editor_{
            &owner,
            model_,
            [proto = proto_] {
                return proto.get();
            }},
        maybe_target_component_{std::move(target_component)}
    {}

    void draw_content()
    {
        draw_name_editor();

        draw_property_editors();

        ui::draw_vertical_spacer(3.0f / 15.0f);

        draw_socket_editors();

        ui::draw_vertical_spacer(1.0f / 15.0f);

        draw_path_point_editor();

        draw_any_error_messages();

        ui::draw_vertical_spacer(1.0f / 15.0f);

        draw_bottom_buttons();
    }

private:
    std::unique_ptr<OpenSim::Component> try_create_component_from_state()
    {
        const OpenSim::Model& model = model_->get_model();

        if (name_.empty()) {
            return nullptr;
        }

        if (proto_sockets_.size() != socket_connectee_paths_.size()) {
            return nullptr;
        }

        // clone prototype
        std::unique_ptr<OpenSim::Component> rv = opyn::clone(*proto_);

        // set name
        rv->setName(name_);

        // assign sockets
        for (size_t i = 0; i < proto_sockets_.size(); ++i) {
            const OpenSim::AbstractSocket& socket = *proto_sockets_[i];
            const OpenSim::ComponentPath& connectee_path = socket_connectee_paths_[i];

            const OpenSim::Component* connectee = opyn::find_component(model, connectee_path);

            if (not connectee) {
                return nullptr;  // invalid connectee slipped through
            }

            rv->updSocket(socket.getName()).connect(*connectee);
        }

        // assign path points (if applicable)
        if (auto* pa = dynamic_cast<OpenSim::PathActuator*>(rv.get())) {
            if (path_points_.size() < 2) {
                return nullptr;
            }

            for (size_t i = 0; i < path_points_.size(); ++i) {
                const auto& pp = path_points_[i];

                if (opyn::is_empty(pp.actualFrame)) {
                    return nullptr;  // invalid path slipped through
                }

                const auto* pof =
                    opyn::find_component<OpenSim::PhysicalFrame>(model, pp.actualFrame);
                if (not pof) {
                    return nullptr;  // invalid path slipped through
                }

                std::stringstream pp_name;
                pp_name << pa->getName() << "-P" << (i + 1);

                pa->addNewPathPoint(pp_name.str(), *pof, pp.locationInFrame);
            }
        }

        return rv;
    }

    bool is_able_to_add_component_from_current_state() const
    {
        const OpenSim::Model& model = model_->get_model();

        const bool has_name = !name_.empty();
        const bool all_sockets_assigned = rgs::all_of(
            socket_connectee_paths_, std::bind_front(opyn::contains_component, std::cref(model)));
        const bool has_enough_path_points =
            dynamic_cast<const OpenSim::PathActuator*>(proto_.get()) == nullptr or
            path_points_.size() >= 2;

        return has_name and all_sockets_assigned and has_enough_path_points;
    }

    void draw_name_editor()
    {
        ui::set_num_columns(2);

        ui::draw_text("name");
        ui::same_line();
        ui::draw_help_marker(
            "Name the newly-added component will have after being added into the model. Note: this is used to derive the name of subcomponents (e.g. path points)");
        ui::next_column();

        ui::draw_string_input("##componentname", name_);
        ui::add_screenshot_annotation_to_last_drawn_item("AddComponentPopup::ComponentNameInput");

        ui::next_column();

        ui::set_num_columns();
    }

    void draw_property_editors()
    {
        auto maybe_updater = prototype_properties_editor_.on_draw();
        if (maybe_updater) {
            OpenSim::AbstractProperty* prop =
                opyn::find_property_mut(*proto_, maybe_updater->get_property_name());
            if (prop) {
                maybe_updater->apply(*prop);
            }
        }
    }

    void draw_socket_editors()
    {
        if (proto_sockets_.empty()) {
            return;
        }

        ui::draw_text("Socket assignments (required)");
        ui::same_line();
        ui::draw_help_marker(
            "The OpenSim::Component being added has `socket`s that connect to other components in the model. You must specify what these sockets should be connected to; otherwise, the component cannot be added to the model.\n\nIn OpenSim, a Socket formalizes the dependency between a Component and another object (typically another Component) without owning that object. While Components can be composites (of multiple components) they often depend on unrelated objects/components that are defined and owned elsewhere. The object that satisfies the requirements of the Socket we term the 'connectee'. When a Socket is satisfied by a connectee we have a successful 'connection' or is said to be connected.");
        ui::draw_separator();

        ui::draw_vertical_spacer(1.0f / 15.0f);

        // for each socket in the prototype (cached), check if the user has chosen a
        // connectee for it yet and provide a UI for selecting them
        for (size_t i = 0; i < proto_sockets_.size(); ++i) {
            draw_ith_socket_editor(i);
            ui::draw_vertical_spacer(0.5f);
        }
    }

    void draw_ith_socket_editor(size_t i)
    {
        const OpenSim::AbstractSocket& socket = *proto_sockets_[i];
        OpenSim::ComponentPath& connectee = socket_connectee_paths_[i];

        ui::set_num_columns(2);

        ui::draw_text(socket.getName());
        ui::same_line();
        ui::draw_help_marker(proto_->getPropertyByName("socket_" + socket.getName()).getComment());
        ui::draw_text_disabled(socket.getConnecteeTypeName());
        ui::next_column();

        // rhs: search and connectee choices
        ui::push_id(static_cast<int>(i));
        ui::set_next_item_width(ui::get_content_region_available().x());
        draw_search_bar(socket_search_strings_[i]);
        ui::begin_child_panel(
            "##pfselector",
            {ui::get_content_region_available().x(),
             5.0f * ui::get_text_line_height_in_current_panel()});

        // iterate through potential connectees in model and print connect-able options
        int inner_id = 0;
        for (const OpenSim::Component& c : model_->get_model().getComponentList()) {
            if (not opyn::is_able_to_connect_to(socket, c)) {
                continue;  // can't connect to it
            }

            if (dynamic_cast<const OpenSim::Station*>(&c) and
                opyn::is_child_of_a<OpenSim::Muscle>(c)) {

                continue;  // it's a muscle point: don't present it (noisy)
            }

            if (!contains_case_insensitive(c.getName(), socket_search_strings_[i])) {
                continue;  // not part of the user-enacted search set
            }

            const OpenSim::ComponentPath abs_path = opyn::get_absolute_path(c);
            const bool selected = abs_path == connectee;

            ui::push_id(inner_id++);
            if (ui::draw_selectable(c.getName(), selected)) {
                connectee = abs_path;
            }

            const Rect selectable_screen_rect = ui::get_last_drawn_item_screen_rect();
            ui::draw_tooltip_if_item_hovered(abs_path.toString());

            ui::pop_id();

            if (selected) {
                App::upd().main_window_add_frame_annotation(c.toString(), selectable_screen_rect);
            }
        }

        ui::end_child_panel();
        ui::pop_id();
        ui::next_column();
        ui::set_num_columns();
    }

    void draw_path_point_editor_choices()
    {
        const OpenSim::Model& model = model_->get_model();

        // show list of choices
        ui::begin_child_panel("##pf_ppchoices", {ui::get_content_region_available().x(), 128.0f});

        // choices
        for (const OpenSim::Component& c : model.getComponentList()) {
            if (cpp23::contains(path_points_, opyn::get_absolute_path(c), [](const auto& pp) {
                    return pp.userChoice;
                })) {

                continue;  // already selected
            }

            const OpenSim::Component* user_choice = nullptr;
            const OpenSim::PhysicalFrame* actual_frame = nullptr;
            SimTK::Vec3 location_in_frame = {0.0, 0.0, 0.0};

            // careful here: the order matters
            //
            // various OpenSim classes compose some of these. E.g. subclasses of
            // AbstractPathPoint *also* contain a station object, but named with a
            // plain name
            if (const auto* pof = dynamic_cast<const OpenSim::PhysicalFrame*>(&c)) {
                user_choice = pof;
                actual_frame = pof;
            } else if (const auto* pp = dynamic_cast<const OpenSim::PathPoint*>(&c)) {
                user_choice = pp;
                actual_frame = &pp->getParentFrame();
                location_in_frame = pp->get_location();
            } else if (const auto* app = dynamic_cast<const OpenSim::AbstractPathPoint*>(&c)) {
                user_choice = app;
                actual_frame = &app->getParentFrame();
            } else if (const auto* station = dynamic_cast<const OpenSim::Station*>(&c)) {
                // check name because it might be a child of one of the above and we
                // don't want to double-count it
                if (station->getName() != "station") {
                    user_choice = station;
                    actual_frame = &station->getParentFrame();
                    location_in_frame = station->get_location();
                }
            }

            if (not user_choice or not actual_frame) {
                continue;  // can't attach a point to it
            }

            if (not contains_case_insensitive(c.getName(), path_search_string_)) {
                continue;  // search failed
            }

            if (ui::draw_selectable(c.getName())) {
                path_points_.emplace_back(
                    opyn::get_absolute_path(*user_choice),
                    opyn::get_absolute_path(*actual_frame),
                    location_in_frame);
            }
            ui::draw_tooltip_if_item_hovered(
                c.getName(), (opyn::get_absolute_path_string(c) + " " + c.getConcreteClassName()));
        }

        ui::end_child_panel();
    }

    void draw_path_point_editor_already_chosen_points()
    {
        const OpenSim::Model& model = model_->get_model();

        ui::begin_child_panel("##pf_pathpoints", {ui::get_content_region_available().x(), 128.0f});

        std::optional<ptrdiff_t> maybe_index_to_erase;
        for (ptrdiff_t i = 0; i < std::ssize(path_points_); ++i) {
            ui::push_id(i);

            ui::push_style_var(ui::StyleVar::ItemSpacing, {0.0f, 0.0f});

            if (ui::draw_button(MSMICONS_TRASH)) {
                maybe_index_to_erase = i;
            }

            ui::same_line();

            if (i <= 0) {
                ui::begin_disabled();
            }
            if (ui::draw_button(MSMICONS_ARROW_UP) and i > 0) {
                std::swap(path_points_[i], path_points_[i - 1]);
            }
            if (i <= 0) {
                ui::end_disabled();
            }

            ui::same_line();

            if (i >= std::ssize(path_points_) - 1) {
                ui::begin_disabled();
            }
            if (ui::draw_button(MSMICONS_ARROW_DOWN) && i < std::ssize(path_points_) - 1) {
                std::swap(path_points_[i], path_points_[i + 1]);
            }
            if (i >= std::ssize(path_points_) - 1) {
                ui::end_disabled();
            }

            ui::pop_style_var();
            ui::same_line();

            ui::draw_text(path_points_[i].userChoice.getComponentName());
            if (ui::is_item_hovered()) {
                if (const OpenSim::Component* c =
                        opyn::find_component(model, path_points_[i].userChoice)) {
                    ui::draw_tooltip(c->getName(), opyn::get_absolute_path_string(*c));
                }
            }

            ui::pop_id();
        }

        if (maybe_index_to_erase) {
            path_points_.erase(path_points_.begin() + *maybe_index_to_erase);
        }

        ui::end_child_panel();
    }

    void draw_path_point_editor()
    {
        const auto* proto_as_pa = dynamic_cast<const OpenSim::PathActuator*>(proto_.get());
        if (not proto_as_pa) {
            return;  // not a path actuator
        }

        // header
        ui::draw_text("Path Points (at least 2 required)");
        ui::same_line();
        ui::draw_help_marker(
            "The Component being added is (effectively) a line that connects physical frames (e.g. bodies) in the model. For example, an OpenSim::Muscle can be described as an actuator that connects bodies in the model together. You **must** specify at least two physical frames on the line in order to add a PathActuator component.\n\nDetails: in OpenSim, some `Components` are `PathActuator`s. All `Muscle`s are defined as `PathActuator`s. A `PathActuator` is an `Actuator` that actuates along a path. Therefore, a `Model` containing a `PathActuator` with zero or one points would be invalid. This is why it is required that you specify at least two points");
        ui::draw_separator();

        ui::set_num_columns(2);
        int imgui_id = 0;

        ui::push_id(imgui_id++);
        ui::set_next_item_width(ui::get_content_region_available().x());
        draw_search_bar(path_search_string_);
        draw_path_point_editor_choices();
        ui::pop_id();
        ui::next_column();

        ui::push_id(imgui_id++);
        ui::draw_dummy({0.0f, ui::get_style_frame_padding().y()});
        ui::draw_text("Chosen:");
        draw_path_point_editor_already_chosen_points();
        ui::pop_id();
        ui::next_column();

        ui::set_num_columns();
    }

    void draw_bottom_buttons()
    {
        if (ui::draw_button("cancel")) {
            request_close();
        }

        if (not is_able_to_add_component_from_current_state()) {
            return;  // can't add anything yet
        }

        ui::same_line();

        if (ui::draw_button(MSMICONS_PLUS " add")) {
            std::unique_ptr<OpenSim::Component> rv = try_create_component_from_state();
            if (rv) {
                try {
                    if (action_add_component_to_model(
                            *model_, std::move(rv), maybe_target_component_)) {
                        request_close();
                    }
                } catch (const std::exception& ex) {
                    current_errors_ = potentially_nested_exception_to_string(ex);
                    if (auto* undoable = dynamic_cast<UndoableModelStatePair*>(model_.get())) {
                        undoable->rollback();
                    }
                }
            }
        }
    }

    void draw_any_error_messages()
    {
        if (not current_errors_.empty()) {
            ui::push_style_color(ui::ColorVar::Text, Color::red());
            ui::draw_vertical_spacer(2.0f / 15.0f);
            ui::draw_text_wrapped("Error adding component to model: {}", current_errors_);
            ui::draw_vertical_spacer(2.0f / 15.0f);
            ui::pop_style_color();
        }
    }

    // the model that the component should be added to
    std::shared_ptr<opyn::ModelStatePair> model_;

    // a prototypical version of the component being added
    std::shared_ptr<OpenSim::Component> proto_;  // (may be shared with editor popups etc)

    // cached sequence of OpenSim::PhysicalFrame sockets in the prototype
    std::vector<const OpenSim::AbstractSocket*> proto_sockets_{opyn::get_all_sockets(*proto_)};

    // user-assigned name for the to-be-added component
    std::string name_{proto_->getConcreteClassName()};

    // a property editor for the prototype's properties
    ObjectPropertiesEditor prototype_properties_editor_;

    // the component that the added component should be added to (as a subcomponent).
    OpenSim::ComponentPath maybe_target_component_;

    // user-enacted search strings for each socket input (used to filter each list)
    std::vector<std::string> socket_search_strings_{proto_sockets_.size()};

    // absolute paths to user-selected connectees of the prototype's sockets
    std::vector<OpenSim::ComponentPath> socket_connectee_paths_{proto_sockets_.size()};

    // absolute paths to user-selected physical frames that should be used as path points
    std::vector<PathPoint> path_points_;

    // search string that user edits to search through possible path point locations
    std::string path_search_string_;

    // storage for any addition errors
    std::string current_errors_;
};

osc::AddComponentPopup::AddComponentPopup(
    Widget* parent,
    std::string_view popup_name,
    std::shared_ptr<opyn::ModelStatePair> model,
    std::unique_ptr<OpenSim::Component> prototype,
    OpenSim::ComponentPath target_component) :

    Popup{std::make_unique<Impl>(
        *this,
        parent,
        popup_name,
        std::move(model),
        std::move(prototype),
        std::move(target_component))}
{}

void osc::AddComponentPopup::impl_draw_content()
{
    private_data().draw_content();
}
