#include "ModelEditorTab.hpp"

#include <SDL_events.h>

#include <memory>
#include <string>
#include <utility>

#include "src/Actions/ActionFunctions.hpp"
#include "src/OpenSimBindings/ForwardDynamicSimulation.hpp"
#include "src/OpenSimBindings/ForwardDynamicSimulatorParams.hpp"
#include "src/OpenSimBindings/Simulation.hpp"


// ---------------


#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/AutoFinalizingModelStatePair.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/StoFileSimulation.hpp"
#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Config.hpp"
#include "src/Platform/Log.hpp"
#include "src/Platform/os.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/FileChangePoller.hpp"
#include "src/Utils/ScopeGuard.hpp"
#include "src/Tabs/ErrorTab.hpp"
#include "src/Tabs/PerformanceAnalyzerTab.hpp"
#include "src/Tabs/SimulatorTab.hpp"
#include "src/Widgets/BasicWidgets.hpp"
#include "src/Widgets/CoordinateEditor.hpp"
#include "src/Widgets/ComponentDetails.hpp"
#include "src/Widgets/MainMenu.hpp"
#include "src/Widgets/ModelActionsMenuItems.hpp"
#include "src/Widgets/ModelHierarchyPanel.hpp"
#include "src/Widgets/ModelMusclePlotPanel.hpp"
#include "src/Widgets/ParamBlockEditorPopup.hpp"
#include "src/Widgets/LogViewer.hpp"
#include "src/Widgets/ObjectPropertiesEditor.hpp"
#include "src/Widgets/ReassignSocketPopup.hpp"
#include "src/Widgets/SelectComponentPopup.hpp"
#include "src/Widgets/SelectGeometryPopup.hpp"
#include "src/Widgets/Select1PFPopup.hpp"
#include "src/Widgets/Select2PFsPopup.hpp"
#include "src/Widgets/UiModelViewer.hpp"

#include <imgui.h>
#include <implot.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/Object.h>
#include <OpenSim/Common/Property.h>
#include <OpenSim/Common/Set.h>
#include <OpenSim/Simulation/Control/Controller.h>
#include <OpenSim/Simulation/Model/BodySet.h>
#include <OpenSim/Simulation/Model/ContactGeometrySet.h>
#include <OpenSim/Simulation/Model/ControllerSet.h>
#include <OpenSim/Simulation/Model/ConstraintSet.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/ForceSet.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/JointSet.h>
#include <OpenSim/Simulation/Model/MarkerSet.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PathActuator.h>
#include <OpenSim/Simulation/Model/PathPoint.h>
#include <OpenSim/Simulation/Model/PathPointSet.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/Model/ProbeSet.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <OpenSim/Simulation/Wrap/WrapObject.h>
#include <OpenSim/Simulation/Wrap/WrapObjectSet.h>
#include <SDL_keyboard.h>
#include <SimTKcommon.h>

#include <array>
#include <cstddef>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>


static std::array<std::string, 5> const g_EditorScreenPanels =
{
    "Actions",
    "Hierarchy",
    "Property Editor",
    "Log",
    "Coordinate Editor",
};

//////////
// DRAWING
//////////

// draw an editor for top-level selected Component members (e.g. name)
static void DrawTopLevelMembersEditor(osc::UndoableModelStatePair& uim)
{
    OpenSim::Component const* selection = uim.getSelected();

    if (!selection)
    {
        ImGui::Text("cannot draw top level editor: nothing selected?");
        return;
    }

    ImGui::PushID(selection);
    ImGui::Columns(2);

    ImGui::TextUnformatted("name");
    ImGui::SameLine();
    osc::DrawHelpMarker("The name of the component", "The component's name can be important. It can be used when components want to refer to eachover. E.g. a joint will name the two frames it attaches to.");

    ImGui::NextColumn();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
    std::string nameBuf = selection->getName();  // allowed, because EnterReturnsTrue needs to internally buffer stuff anyway
    if (osc::InputString("##nameeditor", nameBuf, 128, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        osc::ActionSetSelectedComponentName(uim, nameBuf);
    }

    ImGui::NextColumn();

    ImGui::Columns();
    ImGui::PopID();
}

// draw UI element that lets user change a model joint's type
static void DrawSelectionJointTypeSwitcher(osc::UndoableModelStatePair& uim)
{
    OpenSim::Joint const* selection = uim.getSelectedAs<OpenSim::Joint>();

    if (!selection)
    {
        return;
    }

    int idx = osc::FindJointInParentJointSet(*selection);

    if (idx == -1)
    {
        return;
    }

    ImGui::TextUnformatted("joint type");
    ImGui::NextColumn();

    // look the Joint up in the type registry so we know where it should be in the ImGui::Combo
    std::optional<size_t> maybeTypeIndex = osc::JointRegistry::indexOf(*selection);
    int typeIndex = maybeTypeIndex ? static_cast<int>(*maybeTypeIndex) : -1;

    auto jointNames = osc::JointRegistry::nameCStrings();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
    if (ImGui::Combo(
        "##newjointtypeselector",
        &typeIndex,
        jointNames.data(),
        static_cast<int>(jointNames.size())) &&
        typeIndex >= 0)
    {
        // copy + fixup  a prototype of the user's selection
        std::unique_ptr<OpenSim::Joint> newJoint{osc::JointRegistry::prototypes()[static_cast<size_t>(typeIndex)]->clone()};
        osc::ActionChangeSelectedJointTypeTo(uim, std::move(newJoint));
    }

    ImGui::NextColumn();
}

// draw contextual actions (buttons, sliders) for a selected physical frame
static void DrawPhysicalFrameContextualActions(osc::SelectGeometryPopup& attachGeomPopup, osc::UndoableModelStatePair& uim)
{
    OpenSim::PhysicalFrame const* selection = uim.getSelectedAs<OpenSim::PhysicalFrame>();

    if (!selection)
    {
        return;
    }

    ImGui::Columns(2);

    ImGui::TextUnformatted("geometry");
    ImGui::SameLine();
    osc::DrawHelpMarker("Geometry that is attached to this physical frame. Multiple pieces of geometry can be attached to the frame");
    ImGui::NextColumn();

    if (ImGui::Button("add geometry"))
    {
        attachGeomPopup.open();
    }
    osc::DrawTooltipIfItemHovered("Add Geometry", "Add geometry to this component. Geometry can be removed by selecting it in the hierarchy editor and pressing DELETE");

    if (auto attached = attachGeomPopup.draw(); attached)
    {
        osc::ActionAttachGeometryToSelectedPhysicalFrame(uim, std::move(attached));
    }
    ImGui::NextColumn();

    ImGui::TextUnformatted("offset frame");
    ImGui::NextColumn();
    if (ImGui::Button("add offset frame"))
    {
        osc::ActionAddOffsetFrameToSelection(uim);
    }
    osc::DrawTooltipIfItemHovered("Add Offset Frame", "Add an OpenSim::OffsetFrame as a child of this Component. Other components in the model can then connect to this OffsetFrame, rather than the base Component, so that it can connect at some offset that is relative to the parent Component");
    ImGui::NextColumn();

    ImGui::Columns();
}


// draw contextual actions (buttons, sliders) for a selected joint
static void DrawJointContextualActions(osc::UndoableModelStatePair& uim)
{
    OpenSim::Joint const* selection = uim.getSelectedAs<OpenSim::Joint>();

    if (!selection)
    {
        return;
    }

    ImGui::Columns(2);

    DrawSelectionJointTypeSwitcher(uim);

    if (CanRezeroSelectedJoint(uim))
    {
        ImGui::Text("rezero joint");
        ImGui::NextColumn();
        if (ImGui::Button("rezero"))
        {
            osc::ActionRezeroSelectedJoint(uim);
        }
        osc::DrawTooltipIfItemHovered("Re-zero the joint", "Given the joint's current geometry due to joint defaults, coordinate defaults, and any coordinate edits made in the coordinate editor, this will reorient the joint's parent (if it's an offset frame) to match the child's transformation. Afterwards, it will then resets all of the joints coordinates to zero. This effectively sets the 'zero point' of the joint (i.e. the geometry when all coordinates are zero) to match whatever the current geometry is.");
        ImGui::NextColumn();
    }

    // BEWARE: broke
    {
        ImGui::Text("add offset frame");
        ImGui::NextColumn();

        if (ImGui::Button("parent"))
        {
            osc::ActionAddParentOffsetFrameToSelectedJoint(uim);
        }
        ImGui::SameLine();
        if (ImGui::Button("child"))
        {
            osc::ActionAddChildOffsetFrameToSelectedJoint(uim);
        }
        ImGui::NextColumn();
    }

    ImGui::Columns();
}

// draw contextual actions (buttons, sliders) for a selected joint
static void DrawHCFContextualActions(osc::UndoableModelStatePair& uim)
{
    OpenSim::HuntCrossleyForce const* hcf = uim.getSelectedAs<OpenSim::HuntCrossleyForce>();

    if (!hcf)
    {
        return;
    }

    if (hcf->get_contact_parameters().getSize() > 1)
    {
        ImGui::Text("cannot edit: has more than one HuntCrossleyForce::Parameter");
        return;
    }

    ImGui::Columns(2);

    ImGui::TextUnformatted("add contact geometry");
    ImGui::SameLine();
    osc::DrawHelpMarker("Add OpenSim::ContactGeometry to this OpenSim::HuntCrossleyForce.\n\nCollisions are evaluated for all OpenSim::ContactGeometry attached to the OpenSim::HuntCrossleyForce. E.g. if you want an OpenSim::ContactSphere component to collide with an OpenSim::ContactHalfSpace component during a simulation then you should add both of those components to this force");
    ImGui::NextColumn();

    // allow user to add geom
    {
        if (ImGui::Button("add contact geometry"))
        {
            ImGui::OpenPopup("select contact geometry");
        }

        OpenSim::ContactGeometry const* selected =
            osc::SelectComponentPopup<OpenSim::ContactGeometry>{}.draw("select contact geometry", uim.getModel());

        if (selected)
        {
            osc::ActionAssignContactGeometryToSelectedHCF(uim, *selected);
        }
    }
    ImGui::NextColumn();

    ImGui::Columns();

    // render standard, easy to render, props of the contact params
    if (hcf->get_contact_parameters().getSize() > 0)
    {
        OpenSim::HuntCrossleyForce::ContactParameters const& params = hcf->get_contact_parameters()[0];

        auto easyToHandleProps = std::array<int, 6>
        {
            params.PropertyIndex_geometry,
                params.PropertyIndex_stiffness,
                params.PropertyIndex_dissipation,
                params.PropertyIndex_static_friction,
                params.PropertyIndex_dynamic_friction,
                params.PropertyIndex_viscous_friction,
        };

        osc::ObjectPropertiesEditor st;
        auto maybe_updater = st.draw(params, easyToHandleProps);

        if (maybe_updater)
        {
            osc::ActionApplyPropertyEdit(uim, *maybe_updater);
        }
    }
}

// draw contextual actions (buttons, sliders) for a selected path actuator
static void DrawPathActuatorContextualParams(osc::UndoableModelStatePair& uim)
{
    OpenSim::PathActuator const* pa = uim.getSelectedAs<OpenSim::PathActuator>();

    if (!pa)
    {
        return;
    }

    ImGui::Columns(2);

    char const* modalName = "select physical frame";

    ImGui::TextUnformatted("add path point to end");
    ImGui::NextColumn();

    if (ImGui::Button("add"))
    {
        ImGui::OpenPopup(modalName);
    }
    osc::DrawTooltipIfItemHovered("Add Path Point", "Add a new path point, attached to an OpenSim::PhysicalFrame in the model, to the end of the sequence of path points in this OpenSim::PathActuator");

    // handle popup
    {
        OpenSim::PhysicalFrame const* pf = osc::Select1PFPopup{}.draw(modalName, uim.getModel());

        if (pf)
        {
            osc::ActionAddPathPointToSelectedPathActuator(uim, *pf);
        }
    }

    ImGui::NextColumn();
    ImGui::Columns();
}

static void DrawModelContextualActions(osc::UndoableModelStatePair& uim)
{
    OpenSim::Model const* m = uim.getSelectedAs<OpenSim::Model>();

    if (!m)
    {
        return;
    }

    ImGui::Columns(2);

    ImGui::Text("show frames");
    ImGui::NextColumn();

    if (ImGui::Button("toggle"))
    {
        osc::ActionToggleFrames(uim);
    }
    ImGui::NextColumn();

    ImGui::Columns();
}

// draw socket editor for current selection
static void DrawSocketEditor(osc::ReassignSocketPopup& reassignSocketPopup, osc::UndoableModelStatePair& uim)
{
    OpenSim::Component const* selected = uim.getSelected();

    if (!selected)
    {
        ImGui::TextUnformatted("cannot draw socket editor: selection is blank (shouldn't be)");
        return;
    }

    std::vector<std::string> socknames = osc::GetSocketNames(*selected);

    if (socknames.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, OSC_GREYED_RGBA);
        ImGui::Text("    (OpenSim::%s has no sockets)", selected->getConcreteClassName().c_str());
        ImGui::PopStyleColor();
        return;
    }

    // else: it has sockets with names, list each socket and provide the user
    //       with the ability to reassign the socket's connectee

    ImGui::Columns(2);
    for (std::string const& sn : socknames)
    {
        ImGui::TextUnformatted(sn.c_str());
        ImGui::NextColumn();

        OpenSim::AbstractSocket const& socket = selected->getSocket(sn);
        std::string sockname = socket.getConnecteePath();
        std::string popupname = std::string{"reassign"} + sockname;

        if (ImGui::Button(sockname.c_str()))
        {
            ImGui::OpenPopup(popupname.c_str());
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(socket.getConnecteeAsObject().getName().c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("%s", socket.getConnecteeAsObject().getConcreteClassName().c_str());
            ImGui::NewLine();
            ImGui::TextDisabled("Left-Click: Reassign this socket's connectee");
            ImGui::TextDisabled("Right-Click: Select the connectee");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && dynamic_cast<OpenSim::Component const*>(&socket.getConnecteeAsObject()))
        {
            uim.setSelected(dynamic_cast<OpenSim::Component const*>(&socket.getConnecteeAsObject()));
            ImGui::NextColumn();
            break;  // don't continue to traverse the sockets, because the selection changed
        }

        if (OpenSim::Object const* connectee = reassignSocketPopup.draw(popupname.c_str(), uim.getModel(), socket); connectee)
        {
            ImGui::CloseCurrentPopup();

            std::string error;
            if (osc::ActionReassignSelectedComponentSocket(uim, sn, *connectee, error))
            {
                reassignSocketPopup.clear();
                ImGui::CloseCurrentPopup();
            }
            else
            {
                reassignSocketPopup.setError(error);
            }
        }

        ImGui::NextColumn();
    }
    ImGui::Columns();
}

// draw breadcrumbs for current selection
//
// eg: Model > Joint > PhysicalFrame
static void DrawSelectionBreadcrumbs(osc::UndoableModelStatePair& uim)
{
    OpenSim::Component const* selection = uim.getSelected();

    if (!selection)
    {
        return;
    }

    auto lst = osc::GetPathElements(*selection);

    if (lst.empty())
    {
        return;  // this shouldn't happen, but you never know...
    }

    float indent = 0.0f;

    for (auto it = lst.begin(); it != lst.end() - 1; ++it)
    {
        ImGui::Dummy({indent, 0.0f});
        ImGui::SameLine();

        if (ImGui::Button((*it)->getName().c_str()))
        {
            uim.setSelected(*it);
        }

        if (ImGui::IsItemHovered())
        {
            uim.setHovered(*it);
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::Text("OpenSim::%s", (*it)->getConcreteClassName().c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", (*it)->getConcreteClassName().c_str());
        indent += 15.0f;
    }

    ImGui::Dummy({indent, 0.0f});
    ImGui::SameLine();
    ImGui::TextUnformatted((*(lst.end() - 1))->getName().c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("(%s)", (*(lst.end() - 1))->getConcreteClassName().c_str());
}

static std::string GetDocumentName(osc::UndoableModelStatePair const& uim)
{
    if (uim.hasFilesystemLocation())
    {
        return uim.getFilesystemPath().filename().string();
    }
    else
    {
        return "untitled.osim";
    }
}

class osc::ModelEditorTab::Impl final {
public:
	Impl(MainUIStateAPI* parent, std::unique_ptr<UndoableModelStatePair> model) :
		m_Parent{std::move(parent)},
		m_Model{std::move(model)}
	{
	}

	UID getID() const
	{
		return m_ID;
	}

	CStringView getName() const
	{
		return m_Name;
	}

	TabHost* parent()
	{
		return m_Parent;
	}

    bool isUnsaved() const
    {
        return !m_Model->isUpToDateWithFilesystem();
    }

    bool trySave()
    {
        return ActionSaveModel(m_Parent, *m_Model);
    }

	void onMount()
	{
        App::upd().makeMainEventLoopWaiting();
        m_Name = computeTabName();
        ImPlot::CreateContext();
	}

	void onUnmount()
	{
        ImPlot::DestroyContext();
        App::upd().makeMainEventLoopPolling();
	}

	bool onEvent(SDL_Event const& e)
	{
        if (e.type == SDL_KEYDOWN)
        {
            return onKeydown(e.key);
        }
        else if (e.type == SDL_DROPFILE)
        {
            return onDropEvent(e.drop);
        }
        else
        {
            return false;
        }
	}

	void onTick()
	{
        if (m_FileChangePoller.changeWasDetected(m_Model->getModel().getInputFileName()))
        {
            osc::ActionUpdateModelFromBackingFile(*m_Model);
        }

        m_Name = computeTabName();
	}

	void onDrawMainMenu()
	{
        m_MainMenuFileTab.draw(m_Parent, m_Model.get());
        drawMainMenuEditTab();
        drawMainMenuSimulateTab();
        drawMainMenuWindowTab();
        m_MainMenuAboutTab.draw();

        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
        if (ImGui::Button(ICON_FA_PLAY " Simulate (Ctrl+R)"))
        {
            osc::ActionStartSimulatingModel(m_Parent, *m_Model);
        }
        ImGui::PopStyleColor();

        if (ImGui::Button(ICON_FA_EDIT " Edit simulation settings"))
        {
            m_ParamBlockEditorPopup.open();
        }
	}

	void onDraw()
	{
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        try
        {
            drawUNGUARDED();
            m_ExceptionThrownLastFrame = false;
        }
        catch (std::exception const& ex)
        {
            log::error("an OpenSim::Exception was thrown while drawing the editor");
            log::error("    message = %s", ex.what());
            log::error("OpenSim::Exceptions typically happen when the model is damaged or made invalid by an edit (e.g. setting a property to an invalid value)");

            if (m_ExceptionThrownLastFrame)
            {
                UID tabID = m_Parent->addTab<ErrorTab>(m_Parent, ex);
                m_Parent->selectTab(tabID);
                m_Parent->closeTab(m_ID);
            }
            else
            {
                try
                {
                    m_Model->rollback();
                    log::error("model rollback succeeded");
                    m_ExceptionThrownLastFrame = true;
                }
                catch (std::exception const& ex2)
                {
                    UID tabID = m_Parent->addTab<ErrorTab>(m_Parent, ex2);
                    m_Parent->selectTab(tabID);
                    m_Parent->closeTab(m_ID);
                }
            }

            m_Parent->resetImgui();
        }
	}

private:

    std::string computeTabName()
    {
        std::stringstream ss;
        ss << ICON_FA_EDIT << " ";
        ss << GetDocumentName(*m_Model);
        return std::move(ss).str();
    }

    bool onDropEvent(SDL_DropEvent const& e)
    {
        if (e.file != nullptr && CStrEndsWith(e.file, ".sto"))
        {
            return osc::ActionLoadSTOFileAgainstModel(m_Parent, *m_Model, e.file);
        }

        return false;
    }

    bool onKeydown(SDL_KeyboardEvent const& e)
    {
        bool superDown = e.keysym.mod & (KMOD_CTRL | KMOD_GUI);
        if (superDown)
        {
            if (e.keysym.mod & KMOD_SHIFT)
            {
                switch (e.keysym.sym) {
                case SDLK_z:  // Ctrl+Shift+Z : undo focused model
                    osc::ActionRedoCurrentlyEditedModel(*m_Model);
                    return true;
                }
                return false;
            }

            switch (e.keysym.sym) {
            case SDLK_z:  // Ctrl+Z: undo focused model
                osc::ActionUndoCurrentlyEditedModel(*m_Model);
                return true;
            case SDLK_r:
            {
                // Ctrl+R: start a new simulation from focused model
                return osc::ActionStartSimulatingModel(m_Parent, *m_Model);
            }                
            case SDLK_a:  // Ctrl+A: clear selection
                osc::ActionClearSelectionFromEditedModel(*m_Model);
                return true;
            }

            return false;
        }

        switch (e.keysym.sym) {
        case SDLK_BACKSPACE:
        case SDLK_DELETE:  // BACKSPACE/DELETE: delete selection
            osc::ActionTryDeleteSelectionFromEditedModel(*m_Model);
            return true;
        }

        return false;
    }

    void drawAddMusclePlotMenu(OpenSim::Muscle const& m)
    {
        if (ImGui::BeginMenu("Add Muscle Plot vs:"))
        {
            for (OpenSim::Coordinate const& c : m_Model->getModel().getComponentList<OpenSim::Coordinate>())
            {
                if (ImGui::MenuItem(c.getName().c_str()))
                {
                    addMusclePlot(c, m);
                }
            }

            ImGui::EndMenu();
        }
    }

    // draw contextual actions for selection
    void drawContextualActions()
    {
        if (!m_Model->getSelected())
        {
            ImGui::TextUnformatted("cannot draw contextual actions: selection is blank (shouldn't be)");
            return;
        }

        ImGui::Columns(2);
        ImGui::TextUnformatted("isolate in visualizer");
        ImGui::NextColumn();

        if (m_Model->getSelected() != m_Model->getIsolated())
        {
            if (ImGui::Button("isolate"))
            {
                osc::ActionSetModelIsolationTo(*m_Model, m_Model->getSelected());
            }
        }
        else
        {
            if (ImGui::Button("clear isolation"))
            {
                osc::ActionSetModelIsolationTo(*m_Model, nullptr);
            }
        }
        osc::DrawTooltipIfItemHovered("Toggle Isolation", "Only show this component in the visualizer\n\nThis can be disabled from the Edit menu (Edit -> Clear Isolation)");
        ImGui::NextColumn();


        ImGui::TextUnformatted("copy abspath");
        ImGui::NextColumn();
        if (ImGui::Button("copy"))
        {
            osc::SetClipboardText(m_Model->getSelected()->getAbsolutePathString().c_str());
        }
        osc::DrawTooltipIfItemHovered("Copy Component Absolute Path", "Copy the absolute path to this component to your clipboard.\n\n(This is handy if you are separately using absolute component paths to (e.g.) manipulate the model in a script or something)");
        ImGui::NextColumn();

        ImGui::Columns();

        if (m_Model->getSelectedAs<OpenSim::Model>())
        {
            DrawModelContextualActions(*m_Model);
        }
        else if (m_Model->getSelectedAs<OpenSim::PhysicalFrame>())
        {
            DrawPhysicalFrameContextualActions(m_AttachGeomPopup, *m_Model);
        }
        else if (m_Model->getSelectedAs<OpenSim::Joint>())
        {
            DrawJointContextualActions(*m_Model);
        }
        else if (m_Model->getSelectedAs<OpenSim::HuntCrossleyForce>())
        {
            DrawHCFContextualActions(*m_Model);
        }
        else if (m_Model->getSelectedAs<OpenSim::PathActuator>())
        {
            DrawPathActuatorContextualParams(*m_Model);
        }
    }

    int getNumMusclePlots() const
    {
        return static_cast<int>(m_ModelMusclePlots.size());
    }

    ModelMusclePlotPanel const& getMusclePlot(int idx) const
    {
        return m_ModelMusclePlots.at(idx);
    }

    ModelMusclePlotPanel& updMusclePlot(int idx)
    {
        return m_ModelMusclePlots.at(idx);
    }

    ModelMusclePlotPanel& addMusclePlot()
    {
        return m_ModelMusclePlots.emplace_back(m_Model, std::string{"MusclePlot_"} + std::to_string(m_LatestMusclePlot++));
    }

    ModelMusclePlotPanel& addMusclePlot(OpenSim::Coordinate const& coord, OpenSim::Muscle const& muscle)
    {
        return m_ModelMusclePlots.emplace_back(m_Model, std::string{"MusclePlot_"} + std::to_string(m_LatestMusclePlot++), coord.getAbsolutePath(), muscle.getAbsolutePath());
    }

    void removeMusclePlot(int idx)
    {
        OSC_ASSERT(0 <= idx && idx < static_cast<int>(m_ModelMusclePlots.size()));
        m_ModelMusclePlots.erase(m_ModelMusclePlots.begin() + idx);
    }

    void drawSelectionEditor()
    {
        if (!m_Model->getSelected())
        {
            ImGui::TextUnformatted("(nothing selected)");
            return;
        }

        ImGui::PushID(m_Model->getSelected());

        ImGui::Dummy({0.0f, 1.0f});
        ImGui::TextUnformatted("hierarchy:");
        ImGui::SameLine();
        osc::DrawHelpMarker("Where the selected component is in the model's component hierarchy");
        ImGui::Separator();
        DrawSelectionBreadcrumbs(*m_Model);

        // contextual actions
        ImGui::Dummy({0.0f, 5.0f});
        ImGui::TextUnformatted("contextual actions:");
        ImGui::SameLine();
        osc::DrawHelpMarker("Actions that are specific to the type of OpenSim::Component that is currently selected");
        ImGui::Separator();
        drawContextualActions();

        // a contextual action may have changed this
        if (!m_Model->getSelected())
        {
            return;
        }

        // property editors
        ImGui::Dummy({0.0f, 5.0f});
        ImGui::TextUnformatted("properties:");
        ImGui::SameLine();
        osc::DrawHelpMarker("Properties of the selected OpenSim::Component. These are declared in the Component's implementation.");
        ImGui::Separator();

        // top-level property editors
        {
            DrawTopLevelMembersEditor(*m_Model);
        }

        // top-level member edits may have changed this
        if (!m_Model->getSelected())
        {
            return;
        }

        // property editors
        {
            auto maybeUpdater = m_ObjectPropsEditor.draw(*m_Model->getSelected());
            if (maybeUpdater)
            {
                osc::ActionApplyPropertyEdit(*m_Model, *maybeUpdater);
            }
        }

        // socket editor
        ImGui::Dummy({0.0f, 5.0f});
        ImGui::TextUnformatted("sockets:");
        ImGui::SameLine();
        osc::DrawHelpMarker("What components this component is connected to.\n\nIn OpenSim, a Socket formalizes the dependency between a Component and another object (typically another Component) without owning that object. While Components can be composites (of multiple components) they often depend on unrelated objects/components that are defined and owned elsewhere. The object that satisfies the requirements of the Socket we term the 'connectee'. When a Socket is satisfied by a connectee we have a successful 'connection' or is said to be connected.");
        ImGui::Separator();
        DrawSocketEditor(m_ReassignSocketPopup, *m_Model);

        ImGui::PopID();
    }

    void drawMainMenuEditTab()
    {
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem(ICON_FA_UNDO " Undo", "Ctrl+Z", false, m_Model->canUndo()))
            {
                osc::ActionUndoCurrentlyEditedModel(*m_Model);
            }

            if (ImGui::MenuItem(ICON_FA_REDO " Redo", "Ctrl+Shift+Z", false, m_Model->canRedo()))
            {
                osc::ActionRedoCurrentlyEditedModel(*m_Model);
            }

            if (ImGui::MenuItem(ICON_FA_EYE_SLASH " Clear Isolation", nullptr, false, m_Model->getIsolated()))
            {
                osc::ActionSetModelIsolationTo(*m_Model, nullptr);
            }
            DrawTooltipIfItemHovered("Clear Isolation", "Clear current isolation setting. This is effectively the opposite of 'Isolate'ing a component.");

            {
                float scaleFactor = m_Model->getFixupScaleFactor();
                if (ImGui::InputFloat("set scale factor", &scaleFactor))
                {
                    osc::ActionSetModelSceneScaleFactorTo(*m_Model, scaleFactor);
                }
            }

            if (ImGui::MenuItem(ICON_FA_EXPAND_ARROWS_ALT " autoscale scale factor"))
            {
                osc::ActionAutoscaleSceneScaleFactor(*m_Model);
            }
            DrawTooltipIfItemHovered("Autoscale Scale Factor", "Try to autoscale the model's scale factor based on the current dimensions of the model");

            if (ImGui::MenuItem(ICON_FA_ARROWS_ALT " toggle frames"))
            {
                osc::ActionToggleFrames(*m_Model);
            }
            DrawTooltipIfItemHovered("Toggle Frames", "Set the model's display properties to display physical frames");

            bool modelHasBackingFile = osc::HasInputFileName(m_Model->getModel());

            if (ImGui::MenuItem(ICON_FA_REDO " Reload osim", nullptr, false, modelHasBackingFile))
            {
                osc::ActionReloadOsimFromDisk(*m_Model);
            }
            DrawTooltipIfItemHovered("Reload osim file", "Attempts to reload the osim file from scratch. This can be useful if (e.g.) editing third-party files that OpenSim Creator doesn't automatically track.");

            if (ImGui::MenuItem(ICON_FA_FOLDER " Open .osim's parent directory", nullptr, false, modelHasBackingFile))
            {
                osc::ActionOpenOsimParentDirectory(*m_Model);
            }

            if (ImGui::MenuItem(ICON_FA_LINK " Open .osim in external editor", nullptr, false, modelHasBackingFile))
            {
                osc::ActionOpenOsimInExternalEditor(*m_Model);
            }
            DrawTooltipIfItemHovered("Open .osim in external editor", "Open the .osim file currently being edited in an external text editor. The editor that's used depends on your operating system's default for opening .osim files.");

            ImGui::EndMenu();
        }
    }

    void drawMainMenuSimulateTab()
    {
        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem(ICON_FA_PLAY " Simulate", "Ctrl+R"))
            {
                osc::ActionStartSimulatingModel(m_Parent, *m_Model);
            }

            if (ImGui::MenuItem(ICON_FA_EDIT " Edit simulation settings"))
            {
                m_ParamBlockEditorPopup.open();
            }

            if (ImGui::MenuItem("Disable all wrapping surfaces"))
            {
                osc::ActionDisableAllWrappingSurfaces(*m_Model);
            }

            if (ImGui::MenuItem("Enable all wrapping surfaces"))
            {
                osc::ActionEnableAllWrappingSurfaces(*m_Model);
            }

            if (ImGui::MenuItem("Simulate Against All Integrators (advanced)"))
            {
                osc::ActionSimulateAgainstAllIntegrators(m_Parent, *m_Model);
            }
            osc::DrawTooltipIfItemHovered("Simulate Against All Integrators", "Simulate the given model against all available SimTK integrators. This takes the current simulation parameters and permutes the integrator, reporting the overall simulation wall-time to the user. It's an advanced feature that's handy for developers to figure out which integrator best-suits a particular model");

            ImGui::EndMenu();
        }
    }

    void drawMainMenuWindowTab()
    {
        // draw "window" tab
        if (ImGui::BeginMenu("Window"))
        {
            Config const& cfg = App::get().getConfig();
            for (std::string const& panel : g_EditorScreenPanels)
            {
                bool currentVal = cfg.getIsPanelEnabled(panel);
                if (ImGui::MenuItem(panel.c_str(), nullptr, &currentVal))
                {
                    App::upd().updConfig().setIsPanelEnabled(panel, currentVal);
                }
            }

            ImGui::Separator();

            // active 3D viewers (can be disabled)
            for (int i = 0; i < static_cast<int>(m_ModelViewers.size()); ++i)
            {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "viewer%i", i);

                bool enabled = true;
                if (ImGui::MenuItem(buf, nullptr, &enabled))
                {
                    m_ModelViewers.erase(m_ModelViewers.begin() + i);
                    --i;
                }
            }

            if (ImGui::MenuItem("add viewer"))
            {
                m_ModelViewers.emplace_back();
            }

            ImGui::Separator();

            // active muscle plots
            for (int i = 0; i < getNumMusclePlots(); ++i)
            {
                ModelMusclePlotPanel const& plot = getMusclePlot(i);

                bool enabled = true;
                if (!plot.isOpen() || ImGui::MenuItem(plot.getName().c_str(), nullptr, &enabled))
                {
                    removeMusclePlot(i);
                    --i;
                }
            }

            if (ImGui::MenuItem("add muscle plot"))
            {
                addMusclePlot();
            }

            ImGui::EndMenu();
        }
    }

    // draw a single 3D model viewer
    bool draw3DViewer(osc::UiModelViewer& viewer, char const* name)
    {
        bool isOpen = true;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        bool shown = ImGui::Begin(name, &isOpen, ImGuiWindowFlags_MenuBar);
        ImGui::PopStyleVar();

        if (!isOpen)
        {
            ImGui::End();
            return false;  // closed by the user
        }

        if (!shown)
        {
            ImGui::End();
            return true;  // it's open, but not shown
        }

        auto resp = viewer.draw(*m_Model);
        ImGui::End();

        // update hover
        if (resp.isMousedOver && resp.hovertestResult != m_Model->getHovered())
        {
            m_Model->setHovered(resp.hovertestResult);
        }

        // if left-clicked, update selection
        if (resp.isMousedOver && resp.isLeftClicked)
        {
            m_Model->setSelected(resp.hovertestResult);
        }

        // if hovered, draw hover tooltip
        if (resp.isMousedOver && resp.hovertestResult)
        {
            DrawComponentHoverTooltip(*resp.hovertestResult);
        }

        // if right-clicked, draw context menu
        {
            std::string menuName = std::string{name} + "_contextmenu";

            if (resp.isMousedOver && osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right))
            {
                m_Model->setSelected(resp.hovertestResult);  // can be empty
                ImGui::OpenPopup(menuName.c_str());
            }

            OpenSim::Component const* selected = m_Model->getSelected();

            if (ImGui::BeginPopup(menuName.c_str()))
            {
                if (selected)
                {
                    // draw context menu for whatever's selected
                    ImGui::TextUnformatted(selected->getName().c_str());
                    ImGui::SameLine();
                    ImGui::TextDisabled("%s", selected->getConcreteClassName().c_str());
                    ImGui::Separator();
                    ImGui::Dummy({0.0f, 3.0f});

                    DrawSelectOwnerMenu(*m_Model, *selected);
                    DrawWatchOutputMenu(*m_Parent, *selected);
                    if (OpenSim::Muscle const* m = dynamic_cast<OpenSim::Muscle const*>(selected))
                    {
                        drawAddMusclePlotMenu(*m);
                    }
                }
                else
                {
                    // draw context menu that's shown when nothing was right-clicked
                    m_ContextMenuActionsMenuBar.draw();
                }
                ImGui::EndPopup();
            }
            m_ContextMenuActionsMenuBar.drawAnyOpenPopups();
        }

        return true;
    }

    // draw all user-enabled 3D model viewers
    void draw3DViewers()
    {
        for (int i = 0; i < static_cast<int>(m_ModelViewers.size()); ++i)
        {
            osc::UiModelViewer& viewer = m_ModelViewers[i];

            char buf[64];
            std::snprintf(buf, sizeof(buf), "viewer%i", i);

            bool isOpen = draw3DViewer(viewer, buf);

            if (!isOpen)
            {
                m_ModelViewers.erase(m_ModelViewers.begin() + i);
                --i;
            }
        }
    }

    void drawUNGUARDED()
    {
        // draw 3D viewers (if any)
        draw3DViewers();

        // draw editor actions panel
        //
        // contains top-level actions (e.g. "add body")
        osc::Config const& config = osc::App::get().getConfig();

        if (bool actionsPanelOldState = config.getIsPanelEnabled("Actions"))
        {
            bool actionsPanelNewState = actionsPanelOldState;
            if (ImGui::Begin("Actions", &actionsPanelNewState, ImGuiWindowFlags_MenuBar))
            {
                if (ImGui::BeginMenuBar())
                {
                    m_ModelActionsMenuBar.draw();
                    ImGui::EndMenuBar();
                }
            }
            ImGui::End();

            if (actionsPanelNewState != actionsPanelOldState)
            {
                osc::App::upd().updConfig().setIsPanelEnabled("Actions", actionsPanelNewState);
            }
        }

        // draw hierarchy viewer
        {
            auto resp = m_ComponentHierarchyPanel.draw(*m_Model);

            if (resp.type == osc::ModelHierarchyPanel::ResponseType::SelectionChanged)
            {
                m_Model->setSelected(resp.ptr);
            }
            else if (resp.type == osc::ModelHierarchyPanel::ResponseType::HoverChanged)
            {
                m_Model->setHovered(resp.ptr);
            }
        }

        // draw property editor
        if (bool propertyEditorOldState = config.getIsPanelEnabled("Property Editor"))
        {
            bool propertyEditorState = propertyEditorOldState;
            if (ImGui::Begin("Property Editor", &propertyEditorState))
            {
                drawSelectionEditor();
            }
            ImGui::End();

            if (propertyEditorState != propertyEditorOldState)
            {
                osc::App::upd().updConfig().setIsPanelEnabled("Property Editor", propertyEditorState);
            }
        }

        // draw application log
        if (bool logOldState = config.getIsPanelEnabled("Log"))
        {
            bool logState = logOldState;
            if (ImGui::Begin("Log", &logState, ImGuiWindowFlags_MenuBar))
            {
                m_LogViewer.draw();
            }
            ImGui::End();

            if (logState != logOldState)
            {
                osc::App::upd().updConfig().setIsPanelEnabled("Log", logState);
            }
        }

        // draw coordinate editor
        if (bool coordEdOldState = config.getIsPanelEnabled("Coordinate Editor"))
        {
            bool coordEdState = coordEdOldState;
            if (ImGui::Begin("Coordinate Editor", &coordEdState))
            {
                m_CoordEditor.draw();
            }

            if (coordEdState != coordEdOldState)
            {
                osc::App::upd().updConfig().setIsPanelEnabled("Coordinate Editor", coordEdState);
            }
        }

        // draw model muscle plots (if applicable)
        for (int i = 0; i < getNumMusclePlots(); ++i)
        {
            updMusclePlot(i).draw();
        }

        // draw any currently-open popups

        if (m_ParamBlockEditorPopup.isOpen())
        {
            m_ParamBlockEditorPopup.draw(m_Parent->updSimulationParams());
        }

        m_ModelActionsMenuBar.drawAnyOpenPopups();
    }

	UID m_ID;
	std::string m_Name = "ModelEditorTab";
	MainUIStateAPI* m_Parent;
    std::shared_ptr<UndoableModelStatePair> m_Model;

	// polls changes to a file
	FileChangePoller m_FileChangePoller{std::chrono::milliseconds{1000}, m_Model->getModel().getInputFileName()};

	// UI widgets/popups
	MainMenuFileTab m_MainMenuFileTab;
	MainMenuAboutTab m_MainMenuAboutTab;
	ObjectPropertiesEditor m_ObjectPropsEditor;
	ReassignSocketPopup m_ReassignSocketPopup;
	Select2PFsPopup m_Select2PFsPopup;
	LogViewer m_LogViewer;
	ModelHierarchyPanel m_ComponentHierarchyPanel{"Hierarchy"};
	ModelActionsMenuItems m_ModelActionsMenuBar{m_Model};
	ModelActionsMenuItems m_ContextMenuActionsMenuBar{m_Model};
	CoordinateEditor m_CoordEditor{m_Model};
	SelectGeometryPopup m_AttachGeomPopup{"select geometry to add"};
	ParamBlockEditorPopup m_ParamBlockEditorPopup{"simulation parameters"};
    int m_LatestMusclePlot = 1;
    std::vector<ModelMusclePlotPanel> m_ModelMusclePlots;

    std::vector<UiModelViewer> m_ModelViewers = std::vector<UiModelViewer>(1);

	// flag that's set+reset each frame to prevent continual
	// throwing
	bool m_ExceptionThrownLastFrame = false;
};


// public API

osc::ModelEditorTab::ModelEditorTab(MainUIStateAPI* parent,  std::unique_ptr<UndoableModelStatePair> model) :
	m_Impl{new Impl{std::move(parent), std::move(model)}}
{
}

osc::ModelEditorTab::ModelEditorTab(ModelEditorTab&& tmp) noexcept :
	m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ModelEditorTab& osc::ModelEditorTab::operator=(ModelEditorTab&& tmp) noexcept
{
	std::swap(m_Impl, tmp.m_Impl);
	return *this;
}

osc::ModelEditorTab::~ModelEditorTab() noexcept
{
	delete m_Impl;
}

osc::UID osc::ModelEditorTab::implGetID() const
{
	return m_Impl->getID();
}

osc::CStringView osc::ModelEditorTab::implGetName() const
{
	return m_Impl->getName();
}

osc::TabHost* osc::ModelEditorTab::implParent() const
{
	return m_Impl->parent();
}

bool osc::ModelEditorTab::implIsUnsaved() const
{
    return m_Impl->isUnsaved();
}

bool osc::ModelEditorTab::implTrySave()
{
    return m_Impl->trySave();
}

void osc::ModelEditorTab::implOnMount()
{
	m_Impl->onMount();
}

void osc::ModelEditorTab::implOnUnmount()
{
	m_Impl->onUnmount();
}

bool osc::ModelEditorTab::implOnEvent(SDL_Event const& e)
{
	return m_Impl->onEvent(e);
}

void osc::ModelEditorTab::implOnTick()
{
	m_Impl->onTick();
}

void osc::ModelEditorTab::implOnDrawMainMenu()
{
	m_Impl->onDrawMainMenu();
}

void osc::ModelEditorTab::implOnDraw()
{
	m_Impl->onDraw();
}
