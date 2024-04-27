#include "AddComponentPopup.h"

#include <OpenSimCreator/Documents/Model/UndoableModelActions.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/UI/Shared/ObjectPropertiesEditor.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
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
#include <oscar/Graphics/Color.h>
#include <oscar/Platform/App.h>
#include <oscar/Shims/Cpp23/ranges.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Widgets/StandardPopup.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/StringHelpers.h>
#include <SimTKcommon/SmallMatrix.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
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
            SimTK::Vec3 const& locationInFrame_) :
            userChoice{std::move(userChoice_)},
            actualFrame{std::move(actualFrame_)},
            locationInFrame{locationInFrame_}
        {
        }

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

class osc::AddComponentPopup::Impl final : public StandardPopup {
public:
    Impl(
        std::string_view popupName,
        IPopupAPI* api,
        std::shared_ptr<UndoableModelStatePair> uum,
        std::unique_ptr<OpenSim::Component> prototype) :

        StandardPopup{popupName},
        m_Uum{std::move(uum)},
        m_Proto{std::move(prototype)},
        m_PrototypePropertiesEditor{api, m_Uum, [proto = m_Proto]() { return proto.get(); }}
    {}

private:

    std::unique_ptr<OpenSim::Component> tryCreateComponentFromState()
    {
        OpenSim::Model const& model = m_Uum->getModel();

        if (m_Name.empty())
        {
            return nullptr;
        }

        if (m_ProtoSockets.size() != m_SocketConnecteePaths.size())
        {
            return nullptr;
        }

        // clone prototype
        std::unique_ptr<OpenSim::Component> rv = Clone(*m_Proto);

        // set name
        rv->setName(m_Name);

        // assign sockets
        for (size_t i = 0; i < m_ProtoSockets.size(); ++i)
        {
            OpenSim::AbstractSocket const& socket = *m_ProtoSockets[i];
            OpenSim::ComponentPath const& connecteePath = m_SocketConnecteePaths[i];

            OpenSim::Component const* connectee = FindComponent(model, connecteePath);

            if (!connectee)
            {
                return nullptr;  // invalid connectee slipped through
            }

            rv->updSocket(socket.getName()).connect(*connectee);
        }

        // assign path points (if applicable)
        if (auto* pa = dynamic_cast<OpenSim::PathActuator*>(rv.get()))
        {
            if (m_PathPoints.size() < 2)
            {
                return nullptr;
            }

            for (size_t i = 0; i < m_PathPoints.size(); ++i)
            {
                auto const& pp = m_PathPoints[i];

                if (IsEmpty(pp.actualFrame))
                {
                    return nullptr;  // invalid path slipped through
                }

                auto const* pof = FindComponent<OpenSim::PhysicalFrame>(model, pp.actualFrame);
                if (!pof)
                {
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
        OpenSim::Model const& model = m_Uum->getModel();

        bool hasName = !m_Name.empty();
        bool allSocketsAssigned = rgs::all_of(m_SocketConnecteePaths, std::bind_front(ContainsComponent, model));
        bool hasEnoughPathPoints =
            dynamic_cast<OpenSim::PathActuator const*>(m_Proto.get()) == nullptr ||
            m_PathPoints.size() >= 2;

        return hasName && allSocketsAssigned && hasEnoughPathPoints;
    }

    void drawNameEditor()
    {
        ui::Columns(2);

        ui::TextUnformatted("name");
        ui::SameLine();
        ui::DrawHelpMarker("Name the newly-added component will have after being added into the model. Note: this is used to derive the name of subcomponents (e.g. path points)");
        ui::NextColumn();

        ui::InputString("##componentname", m_Name);
        App::upd().add_frame_annotation("AddComponentPopup::ComponentNameInput", ui::GetItemRect());

        ui::NextColumn();

        ui::Columns();
    }

    void drawPropertyEditors()
    {
        ui::TextUnformatted("Properties");
        ui::SameLine();
        ui::DrawHelpMarker("These are properties of the OpenSim::Component being added. Their datatypes, default values, and help text are defined in the source code (see OpenSim_DECLARE_PROPERTY in OpenSim's C++ source code, if you want the details). Their default values are typically sane enough to let you add the component directly into your model.");
        ui::Separator();

        ui::Dummy({0.0f, 3.0f});

        auto maybeUpdater = m_PrototypePropertiesEditor.onDraw();
        if (maybeUpdater)
        {
            OpenSim::AbstractProperty* prop = FindPropertyMut(*m_Proto, maybeUpdater->getPropertyName());
            if (prop)
            {
                maybeUpdater->apply(*prop);
            }
        }
    }

    void drawSocketEditors()
    {
        if (m_ProtoSockets.empty())
        {
            return;
        }

        ui::TextUnformatted("Socket assignments (required)");
        ui::SameLine();
        ui::DrawHelpMarker("The OpenSim::Component being added has `socket`s that connect to other components in the model. You must specify what these sockets should be connected to; otherwise, the component cannot be added to the model.\n\nIn OpenSim, a Socket formalizes the dependency between a Component and another object (typically another Component) without owning that object. While Components can be composites (of multiple components) they often depend on unrelated objects/components that are defined and owned elsewhere. The object that satisfies the requirements of the Socket we term the 'connectee'. When a Socket is satisfied by a connectee we have a successful 'connection' or is said to be connected.");
        ui::Separator();

        ui::Dummy({0.0f, 1.0f});

        // for each socket in the prototype (cached), check if the user has chosen a
        // connectee for it yet and provide a UI for selecting them
        for (size_t i = 0; i < m_ProtoSockets.size(); ++i)
        {
            drawIthSocketEditor(i);
            ui::Dummy({0.0f, 0.5f*ui::GetTextLineHeight()});
        }
    }

    void drawIthSocketEditor(size_t i)
    {
        OpenSim::AbstractSocket const& socket = *m_ProtoSockets[i];
        OpenSim::ComponentPath& connectee = m_SocketConnecteePaths[i];

        ui::Columns(2);

        ui::TextUnformatted(socket.getName());
        ui::SameLine();
        ui::DrawHelpMarker(m_Proto->getPropertyByName("socket_" + socket.getName()).getComment());
        ui::TextDisabled(socket.getConnecteeTypeName());
        ui::NextColumn();

        // rhs: search and connectee choices
        ui::PushID(static_cast<int>(i));
        ui::TextUnformatted(ICON_FA_SEARCH);
        ui::SameLine();
        ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
        ui::InputString("##search", m_SocketSearchStrings[i]);
        ui::BeginChild("##pfselector", {ui::GetContentRegionAvail().x, 128.0f});

        // iterate through potential connectees in model and print connect-able options
        int innerID = 0;
        for (OpenSim::Component const& c : m_Uum->getModel().getComponentList())
        {
            if (!IsAbleToConnectTo(socket, c)) {
                continue;  // can't connect to it
            }

            if (dynamic_cast<OpenSim::Station const*>(&c) && IsChildOfA<OpenSim::Muscle>(c)) {
                continue;  // it's a muscle point: don't present it (noisy)
            }

            if (!ContainsCaseInsensitive(c.getName(), m_SocketSearchStrings[i])) {
                continue;  // not part of the user-enacted search set
            }

            OpenSim::ComponentPath const absPath = GetAbsolutePath(c);
            bool selected = absPath == connectee;

            ui::PushID(innerID++);
            if (ui::Selectable(c.getName(), selected))
            {
                connectee = absPath;
            }

            Rect const selectableRect = ui::GetItemRect();
            ui::DrawTooltipIfItemHovered(absPath.toString());

            ui::PopID();

            if (selected)
            {
                App::upd().add_frame_annotation(c.toString(), selectableRect);
            }
        }

        ui::EndChild();
        ui::PopID();
        ui::NextColumn();
        ui::Columns();
    }

    void drawPathPointEditorChoices()
    {
        OpenSim::Model const& model = m_Uum->getModel();

        // show list of choices
        ui::BeginChild("##pf_ppchoices", {ui::GetContentRegionAvail().x, 128.0f});

        // choices
        for (OpenSim::Component const& c : model.getComponentList())
        {
            if (cpp23::contains(m_PathPoints, GetAbsolutePath(c), [](auto const& pp) { return pp.userChoice; }))
            {
                continue;  // already selected
            }

            OpenSim::Component const* userChoice = nullptr;
            OpenSim::PhysicalFrame const* actualFrame = nullptr;
            SimTK::Vec3 locationInFrame = {0.0, 0.0, 0.0};

            // careful here: the order matters
            //
            // various OpenSim classes compose some of these. E.g. subclasses of
            // AbstractPathPoint *also* contain a station object, but named with a
            // plain name
            if (auto const* pof = dynamic_cast<OpenSim::PhysicalFrame const*>(&c))
            {
                userChoice = pof;
                actualFrame = pof;
            }
            else if (auto const* pp = dynamic_cast<OpenSim::PathPoint const*>(&c))
            {
                userChoice = pp;
                actualFrame = &pp->getParentFrame();
                locationInFrame = pp->get_location();
            }
            else if (auto const* app = dynamic_cast<OpenSim::AbstractPathPoint const*>(&c))
            {
                userChoice = app;
                actualFrame = &app->getParentFrame();
            }
            else if (auto const* station = dynamic_cast<OpenSim::Station const*>(&c))
            {
                // check name because it might be a child of one of the above and we
                // don't want to double-count it
                if (station->getName() != "station")
                {
                    userChoice = station;
                    actualFrame = &station->getParentFrame();
                    locationInFrame = station->get_location();
                }
            }

            if (!userChoice || !actualFrame)
            {
                continue;  // can't attach a point to it
            }

            if (!ContainsCaseInsensitive(c.getName(), m_PathSearchString))
            {
                continue;  // search failed
            }

            if (ui::Selectable(c.getName()))
            {
                m_PathPoints.emplace_back(
                    GetAbsolutePath(*userChoice),
                    GetAbsolutePath(*actualFrame),
                    locationInFrame
                );
            }
            ui::DrawTooltipIfItemHovered(c.getName(), (GetAbsolutePathString(c) + " " + c.getConcreteClassName()));
        }

        ui::EndChild();
    }

    void drawPathPointEditorAlreadyChosenPoints()
    {
        OpenSim::Model const& model = m_Uum->getModel();

        ui::BeginChild("##pf_pathpoints", {ui::GetContentRegionAvail().x, 128.0f});

        std::optional<ptrdiff_t> maybeIndexToErase;
        for (ptrdiff_t i = 0; i < std::ssize(m_PathPoints); ++i)
        {
            ui::PushID(i);

            ui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});

            if (ui::Button(ICON_FA_TRASH))
            {
                maybeIndexToErase = i;
            }

            ui::SameLine();

            if (i <= 0)
            {
                ui::BeginDisabled();
            }
            if (ui::Button(ICON_FA_ARROW_UP) && i > 0)
            {
                std::swap(m_PathPoints[i], m_PathPoints[i-1]);
            }
            if (i <= 0)
            {
                ui::EndDisabled();
            }

            ui::SameLine();

            if (i >= std::ssize(m_PathPoints) - 1)
            {
                ui::BeginDisabled();
            }
            if (ui::Button(ICON_FA_ARROW_DOWN) && i < std::ssize(m_PathPoints) - 1)
            {
                std::swap(m_PathPoints[i], m_PathPoints[i+1]);
            }
            if (i >= std::ssize(m_PathPoints) - 1)
            {
                ui::EndDisabled();
            }

            ui::PopStyleVar();
            ui::SameLine();

            ui::Text(m_PathPoints[i].userChoice.getComponentName());
            if (ui::IsItemHovered())
            {
                if (OpenSim::Component const* c = FindComponent(model, m_PathPoints[i].userChoice))
                {
                    ui::DrawTooltip(c->getName(), GetAbsolutePathString(*c));
                }
            }

            ui::PopID();
        }

        if (maybeIndexToErase)
        {
            m_PathPoints.erase(m_PathPoints.begin() + *maybeIndexToErase);
        }

        ui::EndChild();
    }

    void drawPathPointEditor()
    {
        auto* protoAsPA = dynamic_cast<OpenSim::PathActuator*>(m_Proto.get());
        if (!protoAsPA)
        {
            return;  // not a path actuator
        }

        // header
        ui::TextUnformatted("Path Points (at least 2 required)");
        ui::SameLine();
        ui::DrawHelpMarker("The Component being added is (effectively) a line that connects physical frames (e.g. bodies) in the model. For example, an OpenSim::Muscle can be described as an actuator that connects bodies in the model together. You **must** specify at least two physical frames on the line in order to add a PathActuator component.\n\nDetails: in OpenSim, some `Components` are `PathActuator`s. All `Muscle`s are defined as `PathActuator`s. A `PathActuator` is an `Actuator` that actuates along a path. Therefore, a `Model` containing a `PathActuator` with zero or one points would be invalid. This is why it is required that you specify at least two points");
        ui::Separator();

        ui::InputString(ICON_FA_SEARCH " search", m_PathSearchString);

        ui::Columns(2);
        int imguiID = 0;

        ui::PushID(imguiID++);
        drawPathPointEditorChoices();
        ui::PopID();
        ui::NextColumn();

        ui::PushID(imguiID++);
        drawPathPointEditorAlreadyChosenPoints();
        ui::PopID();
        ui::NextColumn();

        ui::Columns();
    }

    void drawBottomButtons()
    {
        if (ui::Button("cancel"))
        {
            request_close();
        }

        if (!isAbleToAddComponentFromCurrentState())
        {
            return;  // can't add anything yet
        }

        ui::SameLine();

        if (ui::Button(ICON_FA_PLUS " add"))
        {
            std::unique_ptr<OpenSim::Component> rv = tryCreateComponentFromState();
            if (rv)
            {
                if (ActionAddComponentToModel(*m_Uum, std::move(rv), m_CurrentErrors))
                {
                    request_close();
                }
            }
        }
    }

    void drawAnyErrorMessages()
    {
        if (!m_CurrentErrors.empty())
        {
            ui::PushStyleColor(ImGuiCol_Text, Color::red());
            ui::Dummy({0.0f, 2.0f});
            ui::TextWrapped("Error adding component to model: %s", m_CurrentErrors.c_str());
            ui::Dummy({0.0f, 2.0f});
            ui::PopStyleColor();
        }
    }

    void implDrawContent() final
    {
        drawNameEditor();

        drawPropertyEditors();

        ui::Dummy({0.0f, 3.0f});

        drawSocketEditors();

        ui::Dummy({0.0f, 1.0f});

        drawPathPointEditor();

        drawAnyErrorMessages();

        ui::Dummy({0.0f, 1.0f});

        drawBottomButtons();
    }

    // the model that the component should be added to
    std::shared_ptr<UndoableModelStatePair> m_Uum;

    // a prototypical version of the component being added
    std::shared_ptr<OpenSim::Component> m_Proto;  // (may be shared with editor popups etc)

    // cached sequence of OpenSim::PhysicalFrame sockets in the prototype
    std::vector<OpenSim::AbstractSocket const*> m_ProtoSockets{GetAllSockets(*m_Proto)};

    // user-assigned name for the to-be-added component
    std::string m_Name{m_Proto->getConcreteClassName()};

    // a property editor for the prototype's properties
    ObjectPropertiesEditor m_PrototypePropertiesEditor;

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


// public API

osc::AddComponentPopup::AddComponentPopup(
    std::string_view popupName,
    IPopupAPI* api,
    std::shared_ptr<UndoableModelStatePair> uum,
    std::unique_ptr<OpenSim::Component> prototype) :

    m_Impl{std::make_unique<Impl>(popupName, api, std::move(uum), std::move(prototype))}
{
}

osc::AddComponentPopup::AddComponentPopup(AddComponentPopup&&) noexcept = default;
osc::AddComponentPopup& osc::AddComponentPopup::operator=(AddComponentPopup&&) noexcept = default;
osc::AddComponentPopup::~AddComponentPopup() noexcept = default;

bool osc::AddComponentPopup::implIsOpen() const
{
    return m_Impl->isOpen();
}

void osc::AddComponentPopup::implOpen()
{
    m_Impl->open();
}

void osc::AddComponentPopup::implClose()
{
    m_Impl->close();
}

bool osc::AddComponentPopup::implBeginPopup()
{
    return m_Impl->beginPopup();
}

void osc::AddComponentPopup::implOnDraw()
{
    m_Impl->onDraw();
}

void osc::AddComponentPopup::implEndPopup()
{
    m_Impl->endPopup();
}

