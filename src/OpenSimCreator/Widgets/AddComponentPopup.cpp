#include "AddComponentPopup.hpp"

#include "OpenSimCreator/Model/UndoableModelStatePair.hpp"
#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"
#include "OpenSimCreator/Utils/UndoableModelActions.hpp"
#include "OpenSimCreator/Widgets/ObjectPropertiesEditor.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/StringHelpers.hpp>
#include <oscar/Widgets/StandardPopup.hpp>

#include <imgui.h>
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
#include <SimTKcommon/SmallMatrix.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>
#include <sstream>
#include <string>
#include <utility>

namespace OpenSim
{
    class AbstractProperty;
}

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

class osc::AddComponentPopup::Impl final : public osc::StandardPopup {
public:
    Impl(EditorAPI* api,
         std::shared_ptr<UndoableModelStatePair> uum,
         std::unique_ptr<OpenSim::Component> prototype,
         std::string_view popupName) :

        StandardPopup{popupName},
        m_Uum{std::move(uum)},
        m_Proto{std::move(prototype)},
        m_PrototypePropertiesEditor{api, m_Uum, [proto = m_Proto]() { return proto.get(); }}
    {
    }

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
        std::unique_ptr<OpenSim::Component> rv{m_Proto->clone()};

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
        bool allSocketsAssigned = std::all_of(
            m_SocketConnecteePaths.begin(),
            m_SocketConnecteePaths.end(),
            [&model](OpenSim::ComponentPath const& cp)
            {
                return ContainsComponent(model, cp);
            }
        );
        bool hasEnoughPathPoints =
            !dynamic_cast<OpenSim::PathActuator const*>(m_Proto.get()) ||
            m_PathPoints.size() >= 2;

        return hasName && allSocketsAssigned && hasEnoughPathPoints;
    }

    void drawNameEditor()
    {
        ImGui::Columns(2);

        ImGui::TextUnformatted("name");
        ImGui::SameLine();
        DrawHelpMarker("Name the newly-added component will have after being added into the model. Note: this is used to derive the name of subcomponents (e.g. path points)");
        ImGui::NextColumn();

        osc::InputString("##componentname", m_Name);
        App::upd().addFrameAnnotation("AddComponentPopup::ComponentNameInput", osc::GetItemRect());

        ImGui::NextColumn();

        ImGui::Columns();
    }

    void drawPropertyEditors()
    {
        ImGui::TextUnformatted("Properties");
        ImGui::SameLine();
        DrawHelpMarker("These are properties of the OpenSim::Component being added. Their datatypes, default values, and help text are defined in the source code (see OpenSim_DECLARE_PROPERTY in OpenSim's C++ source code, if you want the details). Their default values are typically sane enough to let you add the component directly into your model.");
        ImGui::Separator();

        ImGui::Dummy({0.0f, 3.0f});

        auto maybeUpdater = m_PrototypePropertiesEditor.onDraw();
        if (maybeUpdater)
        {
            OpenSim::AbstractProperty* prop = osc::FindPropertyMut(*m_Proto, maybeUpdater->getPropertyName());
            if (prop)
            {
                maybeUpdater->apply(*prop);
            }
        }
    }

    void drawSocketEditors()
    {
        OpenSim::Model const& model = m_Uum->getModel();

        if (m_ProtoSockets.empty())
        {
            return;
        }

        ImGui::TextUnformatted("Socket assignments (required)");
        ImGui::SameLine();
        DrawHelpMarker("The OpenSim::Component being added has `socket`s that connect to other components in the model. You must specify what these sockets should be connected to; otherwise, the component cannot be added to the model.\n\nIn OpenSim, a Socket formalizes the dependency between a Component and another object (typically another Component) without owning that object. While Components can be composites (of multiple components) they often depend on unrelated objects/components that are defined and owned elsewhere. The object that satisfies the requirements of the Socket we term the 'connectee'. When a Socket is satisfied by a connectee we have a successful 'connection' or is said to be connected.");
        ImGui::Separator();

        ImGui::Dummy({0.0f, 1.0f});

        // lhs: socket name, rhs: connectee choices
        ImGui::Columns(2);

        // for each socket in the prototype (cached), check if the user has chosen a
        // connectee for it yet and provide a UI for selecting them
        for (size_t i = 0; i < m_ProtoSockets.size(); ++i)
        {
            OpenSim::AbstractSocket const& socket = *m_ProtoSockets[i];
            OpenSim::ComponentPath& connectee = m_SocketConnecteePaths[i];

            ImGui::TextUnformatted(socket.getName().c_str());
            ImGui::NextColumn();

            // rhs: connectee choices
            ImGui::PushID(static_cast<int>(i));
            ImGui::BeginChild("##pfselector", {ImGui::GetContentRegionAvail().x, 128.0f});

            // iterate through PFs in model and print them out
            for (OpenSim::PhysicalFrame const& pf : model.getComponentList<OpenSim::PhysicalFrame>())
            {
                bool selected = osc::GetAbsolutePath(pf) == connectee;

                if (ImGui::Selectable(pf.getName().c_str(), selected))
                {
                    connectee = osc::GetAbsolutePath(pf);
                }

                if (selected)
                {
                    App::upd().addFrameAnnotation(pf.getName(), osc::GetItemRect());
                }
            }

            ImGui::EndChild();
            ImGui::PopID();
            ImGui::NextColumn();
        }

        ImGui::Columns();
    }

    void drawPathPointEditorChoices()
    {
        OpenSim::Model const& model = m_Uum->getModel();

        // show list of choices
        ImGui::BeginChild("##pf_ppchoices", {ImGui::GetContentRegionAvail().x, 128.0f});

        // choices
        for (OpenSim::Component const& c : model.getComponentList())
        {
            auto const isSameUserChoiceAsComponent = [&c](PathPoint const& p)
            {
                return p.userChoice == osc::GetAbsolutePath(c);
            };
            if (std::any_of(m_PathPoints.begin(), m_PathPoints.end(), isSameUserChoiceAsComponent))
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

            if (!ContainsSubstringCaseInsensitive(c.getName(), m_PathSearchString))
            {
                continue;  // search failed
            }

            if (ImGui::Selectable(c.getName().c_str()))
            {
                m_PathPoints.emplace_back(
                    osc::GetAbsolutePath(*userChoice),
                    osc::GetAbsolutePath(*actualFrame),
                    locationInFrame
                );
            }
            DrawTooltipIfItemHovered(c.getName(), (osc::GetAbsolutePathString(c) + " " + c.getConcreteClassName()));
        }

        ImGui::EndChild();
    }

    void drawPathPointEditorAlreadyChosenPoints()
    {
        OpenSim::Model const& model = m_Uum->getModel();

        ImGui::BeginChild("##pf_pathpoints", {ImGui::GetContentRegionAvail().x, 128.0f});

        // selections
        for (ptrdiff_t i = 0; i < ssize(m_PathPoints); ++i)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0f, 0.0f});
            if (ImGui::Button(ICON_FA_TRASH))
            {
                m_PathPoints.erase(m_PathPoints.begin() + i);
                ImGui::PopStyleVar();
                break;
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_ARROW_UP) && i > 0)
            {
                std::swap(m_PathPoints[i], m_PathPoints[i-1]);
                ImGui::PopStyleVar();
                break;
            }
            ImGui::SameLine();
            ImGui::PopStyleVar();
            if (ImGui::Button(ICON_FA_ARROW_DOWN) && i+1 < ssize(m_PathPoints))
            {
                std::swap(m_PathPoints[i], m_PathPoints[i+1]);
                break;
            }
            ImGui::SameLine();
            ImGui::Text("%s", m_PathPoints[i].userChoice.getComponentName().c_str());

            if (ImGui::IsItemHovered())
            {
                OpenSim::Component const* c = FindComponent(model, m_PathPoints[i].userChoice);
                DrawTooltip(c->getName(), osc::GetAbsolutePathString(*c));
            }
        }

        ImGui::EndChild();
    }

    void drawPathPointEditor()
    {
        auto* protoAsPA = dynamic_cast<OpenSim::PathActuator*>(m_Proto.get());
        if (!protoAsPA)
        {
            return;  // not a path actuator
        }

        // header
        ImGui::TextUnformatted("Path Points (at least 2 required)");
        ImGui::SameLine();
        DrawHelpMarker("The Component being added is (effectively) a line that connects physical frames (e.g. bodies) in the model. For example, an OpenSim::Muscle can be described as an actuator that connects bodies in the model together. You **must** specify at least two physical frames on the line in order to add a PathActuator component.\n\nDetails: in OpenSim, some `Components` are `PathActuator`s. All `Muscle`s are defined as `PathActuator`s. A `PathActuator` is an `Actuator` that actuates along a path. Therefore, a `Model` containing a `PathActuator` with zero or one points would be invalid. This is why it is required that you specify at least two points");
        ImGui::Separator();

        osc::InputString(ICON_FA_SEARCH " search", m_PathSearchString);

        ImGui::Columns(2);
        int imguiID = 0;

        ImGui::PushID(imguiID++);
        drawPathPointEditorChoices();
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::PushID(imguiID++);
        drawPathPointEditorAlreadyChosenPoints();
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::Columns();
    }

    void drawBottomButtons()
    {
        if (ImGui::Button("cancel"))
        {
            requestClose();
        }

        if (!isAbleToAddComponentFromCurrentState())
        {
            return;  // can't add anything yet
        }

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_PLUS " add"))
        {
            std::unique_ptr<OpenSim::Component> rv = tryCreateComponentFromState();
            if (rv)
            {
                if (ActionAddComponentToModel(*m_Uum, std::move(rv), m_CurrentErrors))
                {
                    requestClose();
                }
            }
        }
    }

    void drawAnyErrorMessages()
    {
        if (!m_CurrentErrors.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, {1.0f, 0.0f, 0.0f, 1.0f});
            ImGui::Dummy({0.0f, 2.0f});
            ImGui::TextWrapped("Error adding component to model: %s", m_CurrentErrors.c_str());
            ImGui::Dummy({0.0f, 2.0f});
            ImGui::PopStyleColor();
        }
    }

    void implDrawContent() final
    {
        drawNameEditor();

        drawPropertyEditors();

        ImGui::Dummy({0.0f, 3.0f});

        drawSocketEditors();

        ImGui::Dummy({0.0f, 1.0f});

        drawPathPointEditor();

        drawAnyErrorMessages();

        ImGui::Dummy({0.0f, 1.0f});

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
    EditorAPI* api,
    std::shared_ptr<UndoableModelStatePair> uum,
    std::unique_ptr<OpenSim::Component> prototype,
    std::string_view popupName) :

    m_Impl{std::make_unique<Impl>(api, std::move(uum), std::move(prototype), popupName)}
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
