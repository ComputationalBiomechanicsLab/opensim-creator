#include "AddComponentPopup.hpp"

#include "src/Actions/ActionFunctions.hpp"
#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Widgets/ObjectPropertiesEditor.hpp"
#include "src/Widgets/StandardPopup.hpp"

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

        // what the user chose when the clicked in the UI
        OpenSim::ComponentPath userChoice;

        // what the actual frame is that will be attached to
        //
        // (can be different from user choice because the user is permitted to click a station)
        OpenSim::ComponentPath actualFrame;

        // location of the point within the frame
        SimTK::Vec3 locationInFrame;

        PathPoint(OpenSim::ComponentPath userChoice_,
            OpenSim::ComponentPath actualFrame_,
            SimTK::Vec3 locationInFrame_) :
            userChoice{std::move(userChoice_)},
            actualFrame{std::move(actualFrame_)},
            locationInFrame{std::move(locationInFrame_)}
        {
        }
    };
}

class osc::AddComponentPopup::Impl : public osc::StandardPopup {
public:
    Impl(EditorAPI* api,
         std::shared_ptr<UndoableModelStatePair> uum,
         std::unique_ptr<OpenSim::Component> prototype,
         std::string_view popupName) :

         StandardPopup{std::move(popupName)},
         m_EditorAPI{std::move(api)},
         m_Uum{std::move(uum)},
         m_Proto{std::move(prototype)}
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
        OpenSim::PathActuator* pa = dynamic_cast<OpenSim::PathActuator*>(rv.get());
        if (pa)
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

                OpenSim::PhysicalFrame const* pof = FindComponent<OpenSim::PhysicalFrame>(model, pp.actualFrame);

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
        bool allSocketsAssigned = AllOf(m_SocketConnecteePaths, [&model](OpenSim::ComponentPath const& cp)
        {
            return ContainsComponent(model, cp);
        });
        bool hasEnoughPathPoints = !dynamic_cast<OpenSim::PathActuator const*>(m_Proto.get()) || m_PathPoints.size() >= 2;

        return hasName && allSocketsAssigned && hasEnoughPathPoints;
    }

    void drawNameEditor()
    {
        ImGui::Columns(2);

        ImGui::TextUnformatted("name");
        ImGui::SameLine();
        DrawHelpMarker("Name the newly-added component will have after being added into the model. Note: this is used to derive the name of subcomponents (e.g. path points)");
        ImGui::NextColumn();

        osc::InputString("##componentname", m_Name, 128);
        osc::AddFrameAnnotationToLastItem("AddComponentPopup::ComponentNameInput");

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

        auto maybeUpdater = m_PropEditor.draw(*m_Proto);
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
            ImGui::BeginChild("##pfselector", {ImGui::GetContentRegionAvailWidth(), 128.0f});

            // iterate through PFs in model and print them out
            for (OpenSim::PhysicalFrame const& pf : model.getComponentList<OpenSim::PhysicalFrame>())
            {
                bool selected = pf.getAbsolutePath() == connectee;

                if (ImGui::Selectable(pf.getName().c_str(), selected))
                {
                    connectee = pf.getAbsolutePath();
                }

                if (selected)
                {
                    osc::AddFrameAnnotationToLastItem(pf.getName());
                }
            }

            ImGui::EndChild();
            ImGui::PopID();
            ImGui::NextColumn();
        }
    }

    void drawPathPointEditorChoices()
    {
        OpenSim::Model const& model = m_Uum->getModel();

        // show list of choices
        ImGui::BeginChild("##pf_ppchoices", {ImGui::GetContentRegionAvailWidth(), 128.0f});

        // choices
        for (OpenSim::Component const& c : model.getComponentList())
        {
            if (ContainsIf(m_PathPoints, [&c](auto const& p) { return p.userChoice == c.getAbsolutePath(); }))
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
                    userChoice->getAbsolutePath(),
                    actualFrame->getAbsolutePath(),
                    locationInFrame
                );
            }
            DrawTooltipIfItemHovered(c.getName().c_str(), (c.getAbsolutePathString() + " " + c.getConcreteClassName()).c_str());
        }

        ImGui::EndChild();
    }

    void drawPathPointEditorAlreadyChosenPoints()
    {
        OpenSim::Model const& model = m_Uum->getModel();

        ImGui::BeginChild("##pf_pathpoints", {ImGui::GetContentRegionAvailWidth(), 128.0f});

        // selections
        for (size_t i = 0; i < m_PathPoints.size(); ++i)
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
            if (ImGui::Button(ICON_FA_ARROW_DOWN) && i+1 < m_PathPoints.size())
            {
                std::swap(m_PathPoints[i], m_PathPoints[i+1]);
                break;
            }
            ImGui::SameLine();
            ImGui::Text("%s", m_PathPoints[i].userChoice.getComponentName().c_str());

            if (ImGui::IsItemHovered())
            {
                OpenSim::Component const* c = FindComponent(model, m_PathPoints[i].userChoice);
                DrawTooltip(c->getName().c_str(), c->getAbsolutePathString().c_str());
            }
        }

        ImGui::EndChild();
    }

    void drawPathPointEditor()
    {
        OpenSim::Model const& model = m_Uum->getModel();

        OpenSim::PathActuator* protoAsPA = dynamic_cast<OpenSim::PathActuator*>(m_Proto.get());

        if (!protoAsPA)
        {
            return;  // not a path actuator
        }

        // header
        ImGui::TextUnformatted("Path Points (at least 2 required)");
        ImGui::SameLine();
        DrawHelpMarker("The Component being added is (effectively) a line that connects physical frames (e.g. bodies) in the model. For example, an OpenSim::Muscle can be described as an actuator that connects bodies in the model together. You **must** specify at least two physical frames on the line in order to add a PathActuator component.\n\nDetails: in OpenSim, some `Components` are `PathActuator`s. All `Muscle`s are defined as `PathActuator`s. A `PathActuator` is an `Actuator` that actuates along a path. Therefore, a `Model` containing a `PathActuator` with zero or one points would be invalid. This is why it is required that you specify at least two points");
        ImGui::Separator();

        osc::InputString(ICON_FA_SEARCH " search", m_PathSearchString, 128);

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
                if (ActionAddComponentToModel(*m_Uum, std::move(rv)))
                {
                    requestClose();
                }
            }
        }
    }

    void implDraw()
    {
        drawNameEditor();

        drawPropertyEditors();

        ImGui::Dummy({0.0f, 3.0f});

        drawSocketEditors();

        ImGui::Dummy({0.0f, 1.0f});

        drawPathPointEditor();

        ImGui::Dummy({0.0f, 1.0f});

        drawBottomButtons();
    }

private:
    EditorAPI* m_EditorAPI;

    // the model that the component should be added to
    std::shared_ptr<UndoableModelStatePair> m_Uum;

    // a prototypical version of the component being added
    std::unique_ptr<OpenSim::Component> m_Proto;

    // cached sequence of OpenSim::PhysicalFrame sockets in the prototype
    std::vector<OpenSim::AbstractSocket const*> m_ProtoSockets{GetAllSockets(*m_Proto)};

    // user-assigned name for the to-be-added component
    std::string m_Name{m_Proto->getConcreteClassName()};

    // a property editor for the prototype's properties
    ObjectPropertiesEditor m_PropEditor;

    // absolute paths to user-selected connectees of the prototype's sockets
    std::vector<OpenSim::ComponentPath> m_SocketConnecteePaths{m_ProtoSockets.size()};

    // absolute paths to user-selected physical frames that should be used as path points
    std::vector<PathPoint> m_PathPoints;

    // search string that user edits to search through possible path point locations
    std::string m_PathSearchString;
};


// public API

osc::AddComponentPopup::AddComponentPopup(
    EditorAPI* api,
    std::shared_ptr<UndoableModelStatePair> uum,
    std::unique_ptr<OpenSim::Component> prototype,
    std::string_view popupName) :

    m_Impl{new Impl{std::move(api), std::move(uum), std::move(prototype), std::move(popupName)}}
{
}

osc::AddComponentPopup::AddComponentPopup(AddComponentPopup&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::AddComponentPopup& osc::AddComponentPopup::operator=(AddComponentPopup&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::AddComponentPopup::~AddComponentPopup() noexcept
{
    delete m_Impl;
}

bool osc::AddComponentPopup::isOpen() const
{
    return m_Impl->isOpen();
}

void osc::AddComponentPopup::open()
{
    m_Impl->open();
}

void osc::AddComponentPopup::close()
{
    m_Impl->close();
}

bool osc::AddComponentPopup::beginPopup()
{
    return m_Impl->beginPopup();
}

void osc::AddComponentPopup::drawPopupContent()
{
    m_Impl->drawPopupContent();
}

void osc::AddComponentPopup::endPopup()
{
    m_Impl->endPopup();
}
