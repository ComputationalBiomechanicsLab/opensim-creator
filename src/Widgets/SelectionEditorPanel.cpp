#include "SelectionEditorPanel.hpp"

#include "src/Actions/ActionFunctions.hpp"
#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/os.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Widgets/ObjectPropertiesEditor.hpp"
#include "src/Widgets/ReassignSocketPopup.hpp"
#include "src/Widgets/SelectComponentPopup.hpp"
#include "src/Widgets/Select1PFPopup.hpp"
#include "src/Widgets/SelectGeometryPopup.hpp"

#include <imgui.h>
#include <nonstd/span.hpp>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Common/Object.h>
#include <OpenSim/Common/Property.h>
#include <OpenSim/Common/Set.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PathActuator.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


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

class osc::SelectionEditorPanel::Impl final {
public:
    Impl(std::shared_ptr<UndoableModelStatePair> model) :
        m_Model{std::move(model)}
    {
    }

    void draw()
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

private:
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

    std::shared_ptr<UndoableModelStatePair> m_Model;
    SelectGeometryPopup m_AttachGeomPopup{"select geometry to add"};
    ReassignSocketPopup m_ReassignSocketPopup;
    ObjectPropertiesEditor m_ObjectPropsEditor;
};

osc::SelectionEditorPanel::SelectionEditorPanel(std::shared_ptr<UndoableModelStatePair> model) :
    m_Impl{new Impl{std::move(model)}}
{
}
osc::SelectionEditorPanel::SelectionEditorPanel(SelectionEditorPanel&&) noexcept = default;
osc::SelectionEditorPanel& osc::SelectionEditorPanel::operator=(SelectionEditorPanel&&) noexcept = default;
osc::SelectionEditorPanel::~SelectionEditorPanel() noexcept = default;

void osc::SelectionEditorPanel::draw()
{
    m_Impl->draw();
}
