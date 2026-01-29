#include "AddComponentPopup.h"

#include <libopensimcreator/Documents/Model/UndoableModelActions.h>
#include <libopensimcreator/Documents/Model/UndoableModelStatePair.h>
#include <libopensimcreator/Platform/msmicons.h>
#include <libopensimcreator/UI/Shared/BasicWidgets.h>
#include <libopensimcreator/UI/Shared/ObjectPropertiesEditor.h>

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

namespace OpenSim { class AbstractProperty; }
namespace rgs = std::ranges;

namespace
{
    struct PathPoint final {

        PathPoint(OpenSim::ComponentPath userChoice_,
            OpenSim::ComponentPath actualFrame_,
            const SimTK::Vec3& locationInFrame_) :
            userChoice{std::move(userChoice_)},
            actualFrame{std::move(actualFrame_)},
            locationInFrame{locationInFrame_}
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
        std::string_view popupName,
        std::shared_ptr<opyn::ModelStatePair> model,
        std::unique_ptr<OpenSim::Component> prototype,
        OpenSim::ComponentPath targetComponent) :

        PopupPrivate{owner, parent, popupName},
        m_Model{std::move(model)},
        m_Proto{std::move(prototype)},
        m_PrototypePropertiesEditor{&owner, m_Model, [proto = m_Proto]() { return proto.get(); }},
        m_MaybeTargetComponent{std::move(targetComponent)}
    {}

    void draw_content()
    {
        drawNameEditor();

        drawPropertyEditors();

        ui::draw_vertical_spacer(3.0f/15.0f);

        drawSocketEditors();

        ui::draw_vertical_spacer(1.0f/15.0f);

        drawPathPointEditor();

        drawAnyErrorMessages();

        ui::draw_vertical_spacer(1.0f/15.0f);

        drawBottomButtons();
    }

private:
    std::unique_ptr<OpenSim::Component> tryCreateComponentFromState()
    {
        const OpenSim::Model& model = m_Model->getModel();

        if (m_Name.empty()) {
            return nullptr;
        }

        if (m_ProtoSockets.size() != m_SocketConnecteePaths.size()) {
            return nullptr;
        }

        // clone prototype
        std::unique_ptr<OpenSim::Component> rv = opyn::Clone(*m_Proto);

        // set name
        rv->setName(m_Name);

        // assign sockets
        for (size_t i = 0; i < m_ProtoSockets.size(); ++i) {
            const OpenSim::AbstractSocket& socket = *m_ProtoSockets[i];
            const OpenSim::ComponentPath& connecteePath = m_SocketConnecteePaths[i];

            const OpenSim::Component* connectee = opyn::FindComponent(model, connecteePath);

            if (not connectee) {
                return nullptr;  // invalid connectee slipped through
            }

            rv->updSocket(socket.getName()).connect(*connectee);
        }

        // assign path points (if applicable)
        if (auto* pa = dynamic_cast<OpenSim::PathActuator*>(rv.get())) {
            if (m_PathPoints.size() < 2) {
                return nullptr;
            }

            for (size_t i = 0; i < m_PathPoints.size(); ++i) {
                const auto& pp = m_PathPoints[i];

                if (opyn::IsEmpty(pp.actualFrame)) {
                    return nullptr;  // invalid path slipped through
                }

                const auto* pof = opyn::FindComponent<OpenSim::PhysicalFrame>(model, pp.actualFrame);
                if (not pof) {
                    return nullptr;  // invalid path slipped through
                }

                std::stringstream ppName;
                ppName << pa->getName() << "-P" << (i+1);

                pa->addNewPathPoint(ppName.str(), *pof, pp.locationInFrame);
            }
        }

        return rv;
    }

    bool isAbleToAddComponentFromCurrentState() const
    {
        const OpenSim::Model& model = m_Model->getModel();

        const bool hasName = !m_Name.empty();
        const bool allSocketsAssigned = rgs::all_of(m_SocketConnecteePaths, std::bind_front(opyn::ContainsComponent, std::cref(model)));
        const bool hasEnoughPathPoints =
            dynamic_cast<const OpenSim::PathActuator*>(m_Proto.get()) == nullptr or
            m_PathPoints.size() >= 2;

        return hasName and allSocketsAssigned and hasEnoughPathPoints;
    }

    void drawNameEditor()
    {
        ui::set_num_columns(2);

        ui::draw_text("name");
        ui::same_line();
        ui::draw_help_marker("Name the newly-added component will have after being added into the model. Note: this is used to derive the name of subcomponents (e.g. path points)");
        ui::next_column();

        ui::draw_string_input("##componentname", m_Name);
        ui::add_screenshot_annotation_to_last_drawn_item("AddComponentPopup::ComponentNameInput");

        ui::next_column();

        ui::set_num_columns();
    }

    void drawPropertyEditors()
    {
        auto maybeUpdater = m_PrototypePropertiesEditor.onDraw();
        if (maybeUpdater) {
            OpenSim::AbstractProperty* prop = opyn::FindPropertyMut(*m_Proto, maybeUpdater->getPropertyName());
            if (prop) {
                maybeUpdater->apply(*prop);
            }
        }
    }

    void drawSocketEditors()
    {
        if (m_ProtoSockets.empty()) {
            return;
        }

        ui::draw_text("Socket assignments (required)");
        ui::same_line();
        ui::draw_help_marker("The OpenSim::Component being added has `socket`s that connect to other components in the model. You must specify what these sockets should be connected to; otherwise, the component cannot be added to the model.\n\nIn OpenSim, a Socket formalizes the dependency between a Component and another object (typically another Component) without owning that object. While Components can be composites (of multiple components) they often depend on unrelated objects/components that are defined and owned elsewhere. The object that satisfies the requirements of the Socket we term the 'connectee'. When a Socket is satisfied by a connectee we have a successful 'connection' or is said to be connected.");
        ui::draw_separator();

        ui::draw_vertical_spacer(1.0f/15.0f);

        // for each socket in the prototype (cached), check if the user has chosen a
        // connectee for it yet and provide a UI for selecting them
        for (size_t i = 0; i < m_ProtoSockets.size(); ++i) {
            drawIthSocketEditor(i);
            ui::draw_vertical_spacer(0.5f);
        }
    }

    void drawIthSocketEditor(size_t i)
    {
        const OpenSim::AbstractSocket& socket = *m_ProtoSockets[i];
        OpenSim::ComponentPath& connectee = m_SocketConnecteePaths[i];

        ui::set_num_columns(2);

        ui::draw_text(socket.getName());
        ui::same_line();
        ui::draw_help_marker(m_Proto->getPropertyByName("socket_" + socket.getName()).getComment());
        ui::draw_text_disabled(socket.getConnecteeTypeName());
        ui::next_column();

        // rhs: search and connectee choices
        ui::push_id(static_cast<int>(i));
        ui::set_next_item_width(ui::get_content_region_available().x());
        DrawSearchBar(m_SocketSearchStrings[i]);
        ui::begin_child_panel("##pfselector", {ui::get_content_region_available().x(), 5.0f*ui::get_text_line_height_in_current_panel()});

        // iterate through potential connectees in model and print connect-able options
        int innerID = 0;
        for (const OpenSim::Component& c : m_Model->getModel().getComponentList()) {
            if (not opyn::IsAbleToConnectTo(socket, c)) {
                continue;  // can't connect to it
            }

            if (dynamic_cast<const OpenSim::Station*>(&c) and opyn::IsChildOfA<OpenSim::Muscle>(c)) {
                continue;  // it's a muscle point: don't present it (noisy)
            }

            if (!contains_case_insensitive(c.getName(), m_SocketSearchStrings[i])) {
                continue;  // not part of the user-enacted search set
            }

            const OpenSim::ComponentPath absPath = opyn::GetAbsolutePath(c);
            const bool selected = absPath == connectee;

            ui::push_id(innerID++);
            if (ui::draw_selectable(c.getName(), selected)) {
                connectee = absPath;
            }

            const Rect selectableScreenRect = ui::get_last_drawn_item_screen_rect();
            ui::draw_tooltip_if_item_hovered(absPath.toString());

            ui::pop_id();

            if (selected) {
                App::upd().add_main_window_frame_annotation(c.toString(), selectableScreenRect);
            }
        }

        ui::end_child_panel();
        ui::pop_id();
        ui::next_column();
        ui::set_num_columns();
    }

    void drawPathPointEditorChoices()
    {
        const OpenSim::Model& model = m_Model->getModel();

        // show list of choices
        ui::begin_child_panel("##pf_ppchoices", {ui::get_content_region_available().x(), 128.0f});

        // choices
        for (const OpenSim::Component& c : model.getComponentList()) {
            if (cpp23::contains(m_PathPoints, opyn::GetAbsolutePath(c), [](const auto& pp) { return pp.userChoice; })) {
                continue;  // already selected
            }

            const OpenSim::Component* userChoice = nullptr;
            const OpenSim::PhysicalFrame* actualFrame = nullptr;
            SimTK::Vec3 locationInFrame = {0.0, 0.0, 0.0};

            // careful here: the order matters
            //
            // various OpenSim classes compose some of these. E.g. subclasses of
            // AbstractPathPoint *also* contain a station object, but named with a
            // plain name
            if (const auto* pof = dynamic_cast<const OpenSim::PhysicalFrame*>(&c)) {
                userChoice = pof;
                actualFrame = pof;
            }
            else if (const auto* pp = dynamic_cast<const OpenSim::PathPoint*>(&c)) {
                userChoice = pp;
                actualFrame = &pp->getParentFrame();
                locationInFrame = pp->get_location();
            }
            else if (const auto* app = dynamic_cast<const OpenSim::AbstractPathPoint*>(&c)) {
                userChoice = app;
                actualFrame = &app->getParentFrame();
            }
            else if (const auto* station = dynamic_cast<const OpenSim::Station*>(&c)) {
                // check name because it might be a child of one of the above and we
                // don't want to double-count it
                if (station->getName() != "station") {
                    userChoice = station;
                    actualFrame = &station->getParentFrame();
                    locationInFrame = station->get_location();
                }
            }

            if (not userChoice or not actualFrame) {
                continue;  // can't attach a point to it
            }

            if (not contains_case_insensitive(c.getName(), m_PathSearchString)) {
                continue;  // search failed
            }

            if (ui::draw_selectable(c.getName())) {
                m_PathPoints.emplace_back(
                    opyn::GetAbsolutePath(*userChoice),
                    opyn::GetAbsolutePath(*actualFrame),
                    locationInFrame
                );
            }
            ui::draw_tooltip_if_item_hovered(c.getName(), (opyn::GetAbsolutePathString(c) + " " + c.getConcreteClassName()));
        }

        ui::end_child_panel();
    }

    void drawPathPointEditorAlreadyChosenPoints()
    {
        const OpenSim::Model& model = m_Model->getModel();

        ui::begin_child_panel("##pf_pathpoints", {ui::get_content_region_available().x(), 128.0f});

        std::optional<ptrdiff_t> maybeIndexToErase;
        for (ptrdiff_t i = 0; i < std::ssize(m_PathPoints); ++i) {
            ui::push_id(i);

            ui::push_style_var(ui::StyleVar::ItemSpacing, {0.0f, 0.0f});

            if (ui::draw_button(MSMICONS_TRASH)) {
                maybeIndexToErase = i;
            }

            ui::same_line();

            if (i <= 0) {
                ui::begin_disabled();
            }
            if (ui::draw_button(MSMICONS_ARROW_UP) and i > 0) {
                std::swap(m_PathPoints[i], m_PathPoints[i-1]);
            }
            if (i <= 0) {
                ui::end_disabled();
            }

            ui::same_line();

            if (i >= std::ssize(m_PathPoints) - 1) {
                ui::begin_disabled();
            }
            if (ui::draw_button(MSMICONS_ARROW_DOWN) && i < std::ssize(m_PathPoints) - 1) {
                std::swap(m_PathPoints[i], m_PathPoints[i+1]);
            }
            if (i >= std::ssize(m_PathPoints) - 1) {
                ui::end_disabled();
            }

            ui::pop_style_var();
            ui::same_line();

            ui::draw_text(m_PathPoints[i].userChoice.getComponentName());
            if (ui::is_item_hovered()) {
                if (const OpenSim::Component* c = opyn::FindComponent(model, m_PathPoints[i].userChoice)) {
                    ui::draw_tooltip(c->getName(), opyn::GetAbsolutePathString(*c));
                }
            }

            ui::pop_id();
        }

        if (maybeIndexToErase) {
            m_PathPoints.erase(m_PathPoints.begin() + *maybeIndexToErase);
        }

        ui::end_child_panel();
    }

    void drawPathPointEditor()
    {
        auto* protoAsPA = dynamic_cast<OpenSim::PathActuator*>(m_Proto.get());
        if (not protoAsPA) {
            return;  // not a path actuator
        }

        // header
        ui::draw_text("Path Points (at least 2 required)");
        ui::same_line();
        ui::draw_help_marker("The Component being added is (effectively) a line that connects physical frames (e.g. bodies) in the model. For example, an OpenSim::Muscle can be described as an actuator that connects bodies in the model together. You **must** specify at least two physical frames on the line in order to add a PathActuator component.\n\nDetails: in OpenSim, some `Components` are `PathActuator`s. All `Muscle`s are defined as `PathActuator`s. A `PathActuator` is an `Actuator` that actuates along a path. Therefore, a `Model` containing a `PathActuator` with zero or one points would be invalid. This is why it is required that you specify at least two points");
        ui::draw_separator();

        ui::set_num_columns(2);
        int imguiID = 0;

        ui::push_id(imguiID++);
        ui::set_next_item_width(ui::get_content_region_available().x());
        DrawSearchBar(m_PathSearchString);
        drawPathPointEditorChoices();
        ui::pop_id();
        ui::next_column();

        ui::push_id(imguiID++);
        ui::draw_dummy({0.0f, ui::get_style_frame_padding().y()});
        ui::draw_text("Chosen:");
        drawPathPointEditorAlreadyChosenPoints();
        ui::pop_id();
        ui::next_column();

        ui::set_num_columns();
    }

    void drawBottomButtons()
    {
        if (ui::draw_button("cancel")) {
            request_close();
        }

        if (not isAbleToAddComponentFromCurrentState()) {
            return;  // can't add anything yet
        }

        ui::same_line();

        if (ui::draw_button(MSMICONS_PLUS " add")) {
            std::unique_ptr<OpenSim::Component> rv = tryCreateComponentFromState();
            if (rv) {
                try {
                    if (ActionAddComponentToModel(*m_Model, std::move(rv), m_MaybeTargetComponent)) {
                        request_close();
                    }
                }
                catch (const std::exception& ex) {
                    m_CurrentErrors = potentially_nested_exception_to_string(ex);
                    if (auto* undoable = dynamic_cast<UndoableModelStatePair*>(m_Model.get())) {
                        undoable->rollback();
                    }
                }
            }
        }
    }

    void drawAnyErrorMessages()
    {
        if (not m_CurrentErrors.empty()) {
            ui::push_style_color(ui::ColorVar::Text, Color::red());
            ui::draw_vertical_spacer(2.0f/15.0f);
            ui::draw_text_wrapped("Error adding component to model: %s", m_CurrentErrors.c_str());
            ui::draw_vertical_spacer(2.0f/15.0f);
            ui::pop_style_color();
        }
    }

    // the model that the component should be added to
    std::shared_ptr<opyn::ModelStatePair> m_Model;

    // a prototypical version of the component being added
    std::shared_ptr<OpenSim::Component> m_Proto;  // (may be shared with editor popups etc)

    // cached sequence of OpenSim::PhysicalFrame sockets in the prototype
    std::vector<const OpenSim::AbstractSocket*> m_ProtoSockets{opyn::GetAllSockets(*m_Proto)};

    // user-assigned name for the to-be-added component
    std::string m_Name{m_Proto->getConcreteClassName()};

    // a property editor for the prototype's properties
    ObjectPropertiesEditor m_PrototypePropertiesEditor;

    // the component that the added component should be added to (as a subcomponent).
    OpenSim::ComponentPath m_MaybeTargetComponent;

    // user-enacted search strings for each socket input (used to filter each list)
    std::vector<std::string> m_SocketSearchStrings{m_ProtoSockets.size()};

    // absolute paths to user-selected connectees of the prototype's sockets
    std::vector<OpenSim::ComponentPath> m_SocketConnecteePaths{m_ProtoSockets.size()};

    // absolute paths to user-selected physical frames that should be used as path points
    std::vector<PathPoint> m_PathPoints;

    // search string that user edits to search through possible path point locations
    std::string m_PathSearchString;

    // storage for any addition errors
    std::string m_CurrentErrors;
};


osc::AddComponentPopup::AddComponentPopup(
    Widget* parent,
    std::string_view popupName,
    std::shared_ptr<opyn::ModelStatePair> model,
    std::unique_ptr<OpenSim::Component> prototype,
    OpenSim::ComponentPath targetComponent) :

    Popup{std::make_unique<Impl>(*this, parent, popupName, std::move(model), std::move(prototype), std::move(targetComponent))}
{}

void osc::AddComponentPopup::impl_draw_content()
{
    private_data().draw_content();
}
