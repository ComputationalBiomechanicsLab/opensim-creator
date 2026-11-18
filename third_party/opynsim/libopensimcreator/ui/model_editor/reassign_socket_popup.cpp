#include "reassign_socket_popup.h"

#include <libopensimcreator/documents/model/undoable_model_actions.h>
#include <libopensimcreator/ui/shared/basic_widgets.h>

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/popups/popup.h>
#include <liboscar/ui/popups/popup_private.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <algorithm>
#include <compare>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    // parameters that affect which sockets are displayed
    struct PopupParams final {

        PopupParams(UID model_version, OpenSim::ComponentPath path, std::string socket_name) :

            modelVersion{model_version},
            componentPath{std::move(path)},
            socketName{std::move(socket_name)}
        {}

        friend bool operator==(const PopupParams&, const PopupParams&) = default;

        UID modelVersion;
        OpenSim::ComponentPath componentPath;
        std::string socketName;
        std::string search;
    };

    // a single user-selectable connectee option
    struct ConnecteeOption final {

        explicit ConnecteeOption(const OpenSim::Component& c) :
            absPath{opyn::get_absolute_path(c)},
            name{c.getName()}
        {}

        friend auto operator<=>(const ConnecteeOption& lhs, const ConnecteeOption& rhs)
        {
            return std::tie(lhs.name, lhs.absPath.toString()) <=>
                   std::tie(rhs.name, rhs.absPath.toString());
        }

        [[maybe_unused]] friend bool operator==(
            const ConnecteeOption& lhs, const ConnecteeOption& rhs)
        {
            return std::tie(lhs.name, lhs.absPath.toString()) ==
                   std::tie(rhs.name, rhs.absPath.toString());
        }

        OpenSim::ComponentPath absPath;
        std::string name;
    };

    // generate a list of possible connectee options, given a set of popup parameters
    std::vector<ConnecteeOption> generate_selection_options(
        const OpenSim::Model& model, const PopupParams& params)
    {
        std::vector<ConnecteeOption> rv;

        const OpenSim::Component* component = opyn::find_component(model, params.componentPath);
        if (not component) {
            return rv;  // component isn't in model?
        }

        const OpenSim::AbstractSocket* socket = opyn::find_socket(*component, params.socketName);
        if (not socket) {
            return rv;  // socket isn't in model?
        }

        for (const OpenSim::Component& other : model.getComponentList()) {
            if (&other == component) {
                continue;  // hide redundant reconnnections
            }

            if (not other.getName().contains(params.search)) {
                continue;  // filtered out by search string
            }

            if (not opyn::is_able_to_connect_to(*socket, other)) {
                continue;  // connection would be rejected anyway
            }

            rv.emplace_back(other);
        }

        rgs::sort(rv);

        return rv;
    }
}

class osc::ReassignSocketPopup::Impl final : public PopupPrivate {
public:
    explicit Impl(
        ReassignSocketPopup& owner,
        Widget* parent,
        std::string_view popup_name,
        std::shared_ptr<opyn::ModelStatePair> model,
        std::string_view component_abs_path,
        std::string_view socket_name) :

        PopupPrivate{owner, parent, popup_name},
        model_{std::move(model)},
        params_{
            model_->get_model_version(), std::string{component_abs_path}, std::string{socket_name}}
    {}

    void draw_content()
    {
        // caching: regenerate cached socket list, if necessary
        //
        // we cache the list because searching+filtering all possible connectees is
        // very slow in OpenSim (#384)
        edited_params_.modelVersion = model_->get_model_version();
        if (edited_params_ != params_) {
            options_ = generate_selection_options(model_->get_model(), edited_params_);
            params_ = edited_params_;
        }

        // check: ensure the "from" side of the socket still exists
        const OpenSim::Component* component =
            opyn::find_component(model_->get_model(), params_.componentPath);
        if (not component) {
            request_close();
            return;
        }

        // check: ensure the socket still exists
        const OpenSim::AbstractSocket* socket = opyn::find_socket(*component, params_.socketName);
        if (not socket) {
            request_close();
            return;
        }

        // draw UI

        ui::draw_text("{}'s new connectee:", socket->getName());

        ui::set_next_item_width(ui::get_content_region_available().x());
        draw_search_bar(edited_params_.search);

        ui::begin_child_panel(
            "##componentlist",
            Vector2{-1.0f, 16.0f * ui::get_text_line_height_in_current_panel()},
            ui::ChildPanelFlag::Border);

        int id = 0;  // care: necessary because multiple connectees may have the same name
        for (const ConnecteeOption& option : options_) {
            ui::push_id(id++);
            if (ui::draw_selectable(option.name, option.absPath == user_selection_abs_path_)) {
                user_selection_abs_path_ = option.absPath;
            }
            ui::draw_tooltip_if_item_hovered(option.absPath.toString());
            ui::pop_id();
        }
        ui::end_child_panel();

        if (not error_.empty()) {
            ui::set_next_item_width(ui::get_content_region_available().x());
            ui::draw_text_wrapped(error_);
        }

        ui::start_new_line();  // breathing room

        if (not user_selection_abs_path_) {
            ui::begin_disabled();
        }
        if (ui::draw_button("Ok") and user_selection_abs_path_) {
            const SocketReassignmentFlags flags =
                try_reexpress_in_different_frame_
                    ? SocketReassignmentFlags::TryReexpressComponentInNewConnectee
                    : SocketReassignmentFlags::None;

            const OpenSim::Component* selected =
                opyn::find_component(model_->get_model(), *user_selection_abs_path_);

            if (selected and
                action_reassign_component_socket(
                    *model_, params_.componentPath, params_.socketName, *selected, flags, error_)) {
                request_close();
                return;
            }
        }
        if (ui::is_item_hovered(ui::HoveredFlag::AllowWhenDisabled)) {
            if (not user_selection_abs_path_) {
                ui::draw_tooltip("Disabled", "A new connectee hasn't been selected.");
            }
        }
        if (not user_selection_abs_path_) {
            ui::end_disabled();
        }
        ui::same_line();
        if (ui::draw_button("Cancel")) {
            request_close();
            return;
        }
        // Add a checkbox that lets the user re-express a component in a new frame (#326),
        // make sure the checkbox is hard to miss (#959).
        ui::same_line();
        ui::draw_vertical_separator();
        ui::same_line();
        try_draw_reexpress_property_in_frame_checkbox(*component, *socket);
    }

    void on_close()
    {
        edited_params_.search.clear();
        error_.clear();
    }

private:
    void try_draw_reexpress_property_in_frame_checkbox(
        const OpenSim::Component& component, const OpenSim::AbstractSocket& abstract_socket)
    {
        const std::string label = [&component] {
            std::stringstream ss;
            ss << "Re-express " << component.getName() << " in chosen frame";
            return std::move(ss).str();
        }();

        const auto* const phys_frame_socket =
            dynamic_cast<const OpenSim::Socket<OpenSim::PhysicalFrame>*>(&abstract_socket);
        if (not phys_frame_socket) {
            bool v = false;  // always `false`
            ui::begin_disabled();
            ui::draw_checkbox(label, &v);
            ui::draw_tooltip_body_only_if_item_hovered(
                "Disabled: the socket doesn't connect to a physical frame");
            ui::end_disabled();
            return;
        }

        const auto component_spatial_representation =
            opyn::try_get_spatial_representation(component, model_->get_state());
        if (not component_spatial_representation) {
            bool v = false;  // always `false`
            ui::begin_disabled();
            ui::draw_checkbox(label, &v);
            ui::draw_tooltip_body_only_if_item_hovered(
                "Disabled: the component doesn't have a spatial representation that OSC knows how to re-express");
            ui::end_disabled();
            return;
        }

        ui::draw_checkbox(label, &try_reexpress_in_different_frame_);
        ui::same_line();
        ui::draw_help_marker(
            "Component Re-Expression",
            "This will recalculate the socket owner's appropriate spatial property such that it remains in the same location in ground after changing this socket.");
    }

    std::shared_ptr<opyn::ModelStatePair> model_;
    PopupParams params_;
    PopupParams edited_params_ = params_;
    std::vector<ConnecteeOption> options_ =
        generate_selection_options(model_->get_model(), edited_params_);
    std::optional<OpenSim::ComponentPath> user_selection_abs_path_;
    std::string error_;
    bool try_reexpress_in_different_frame_ = false;
};

osc::ReassignSocketPopup::ReassignSocketPopup(
    Widget* parent,
    std::string_view popup_name,
    std::shared_ptr<opyn::ModelStatePair> model,
    std::string_view component_abs_path,
    std::string_view socket_name) :

    Popup{std::make_unique<Impl>(
        *this, parent, popup_name, std::move(model), component_abs_path, socket_name)}
{}
void osc::ReassignSocketPopup::impl_draw_content()
{
    private_data().draw_content();
}
void osc::ReassignSocketPopup::impl_on_close()
{
    private_data().on_close();
}
