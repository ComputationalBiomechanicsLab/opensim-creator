#include "ModelEditorScreen.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/ComponentOutputExtractor.hpp"
#include "src/OpenSimBindings/FdSimulation.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/MainEditorState.hpp"
#include "src/OpenSimBindings/StoFileSimulation.hpp"
#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/OpenSimBindings/UiModel.hpp"
#include "src/Screens/ErrorScreen.hpp"
#include "src/Screens/SimulatorScreen.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Platform/os.hpp"
#include "src/Platform/Styling.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/FileChangePoller.hpp"
#include "src/Utils/ScopeGuard.hpp"
#include "src/Widgets/CoordinateEditor.hpp"
#include "src/Widgets/ComponentDetails.hpp"
#include "src/Widgets/ComponentHierarchy.hpp"
#include "src/Widgets/MainMenu.hpp"
#include "src/Widgets/ModelActionsMenuBar.hpp"
#include "src/Widgets/ParamBlockEditorPopup.hpp"
#include "src/Widgets/LogViewer.hpp"
#include "src/Widgets/ObjectPropertiesEditor.hpp"
#include "src/Widgets/ReassignSocketPopup.hpp"
#include "src/Widgets/SaveChangesPopup.hpp"
#include "src/Widgets/SelectComponentPopup.hpp"
#include "src/Widgets/SelectGeometryPopup.hpp"
#include "src/Widgets/Select1PFPopup.hpp"
#include "src/Widgets/Select2PFsPopup.hpp"
#include "src/Widgets/UiModelViewer.hpp"

#include <imgui.h>
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
#include <stdexcept>
#include <utility>
#include <vector>

// draw component information as a hover tooltip
static void DrawComponentHoverTooltip(OpenSim::Component const& hovered, glm::vec3 const& pos)
{
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() + 400.0f);

    ImGui::TextUnformatted(hovered.getName().c_str());
    ImGui::Dummy(ImVec2{0.0f, 3.0f});
    ImGui::Indent();
    ImGui::TextDisabled("Component Type = %s", hovered.getConcreteClassName().c_str());
    ImGui::TextDisabled("Mouse Location = (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
    ImGui::Unindent();
    ImGui::Dummy(ImVec2{0.0f, 5.0f});
    ImGui::TextDisabled("(right-click for actions)");

    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

static void DrawOutputTooltip(OpenSim::AbstractOutput const& o)
{
    ImGui::BeginTooltip();
    ImGui::Text("%s", o.getTypeName().c_str());
    ImGui::EndTooltip();
}

// try to delete an undoable-model's current selection
//
// "try", because some things are difficult to delete from OpenSim models
static void ActionTryDeleteSelectionFromEditedModel(osc::UndoableUiModel& uim)
{
    if (OpenSim::Component* selected = uim.updSelected())
    {
        if (osc::TryDeleteComponentFromModel(uim.updUiModel().peekModelADVANCED(), *selected))
        {
            uim.setDirty(true);
            uim.commit("deleted compopnent");
        }
        else
        {
            uim.setDirty(false);
        }
    }
}

// draw an editor for top-level selected Component members (e.g. name)
static void DrawTopLevelMembersEditor(osc::UndoableUiModel& st)
{
    OpenSim::Component const* selection = st.getSelected();

    if (!selection)
    {
        ImGui::Text("cannot draw top level editor: nothing selected?");
        return;
    }

    ImGui::PushID(selection);
    ImGui::Columns(2);

    ImGui::TextUnformatted("name");
    ImGui::NextColumn();

    char nambuf[128];
    nambuf[sizeof(nambuf) - 1] = '\0';
    std::strncpy(nambuf, selection->getName().c_str(), sizeof(nambuf) - 1);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
    if (ImGui::InputText("##nameditor", nambuf, sizeof(nambuf), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (std::strlen(nambuf) > 0)
        {
            OpenSim::Component* selectedComponent = st.updSelected();
            selectedComponent->setName(nambuf);
            st.setSelected(selectedComponent);  // CARE: the selection depends on the name of the component
            st.commit("changed component name");
        }
    }
    ImGui::NextColumn();

    ImGui::Columns();
    ImGui::PopID();
}

// draw UI element that lets user change a model joint's type
static void DrawSelectionJointTypeSwitcher(osc::UndoableUiModel& st)
{
    OpenSim::Joint const* selection = st.getSelectedAs<OpenSim::Joint>();

    if (!selection)
    {
        return;
    }

    auto const* parentJointset =
        selection->hasOwner() ? dynamic_cast<OpenSim::JointSet const*>(&selection->getOwner()) : nullptr;

    if (!parentJointset)
    {
        // it's a joint, but it's not owned by a JointSet, so the implementation cannot switch
        // the joint type
        return;
    }

    OpenSim::JointSet const& js = *parentJointset;

    int idx = -1;
    for (int i = 0; i < js.getSize(); ++i) {
        OpenSim::Joint const* j = &js[i];
        if (j == selection) {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        // logically, this should never happen
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
        osc::CopyCommonJointProperties(*selection, *newJoint);

        // overwrite old joint in model
        //
        // note: this will invalidate the `selection` joint, because the
        // OpenSim::JointSet container will automatically kill it
        OpenSim::Joint* ptr = newJoint.get();
        st.setDirty(true);
        const_cast<OpenSim::JointSet&>(js).set(idx, newJoint.release());
        st.setSelected(ptr);
        st.commit("changed joint type");
    }

    ImGui::NextColumn();
}

// try to undo currently edited model to earlier state
static void ActionUndoCurrentlyEditedModel(osc::MainEditorState& mes)
{
    std::shared_ptr<osc::UndoableUiModel> editedModel = mes.editedModel();
    if (editedModel->canUndo())
    {
        editedModel->doUndo();
    }
}

// try to redo currently edited model to later state
static void ActionRedoCurrentlyEditedModel(osc::MainEditorState& mes)
{
    std::shared_ptr<osc::UndoableUiModel> editedModel = mes.editedModel();
    if (editedModel->canRedo())
    {
        editedModel->doRedo();
    }
}

// disable all wrapping surfaces in the current model
static void ActionDisableAllWrappingSurfaces(osc::MainEditorState& mes)
{
    osc::DeactivateAllWrapObjectsIn(mes.editedModel()->updModel());
    mes.editedModel()->commit("disabled all wrapping surfaces");
}

// enable all wrapping surfaces in the current model
static void ActionEnableAllWrappingSurfaces(osc::MainEditorState& mes)
{
    std::shared_ptr<osc::UndoableUiModel> editedModel = mes.editedModel();
    osc::ActivateAllWrapObjectsIn(editedModel->updModel());
    editedModel->commit("enabled all wrapping surfaces");
}

// try to start a new simulation from the currently-edited model
static void ActionStartSimulationFromEditedModel(osc::MainEditorState& mes)
{
    StartSimulatingEditedModel(mes);
}

static void ActionClearSelectionFromEditedModel(osc::MainEditorState& mes)
{
    mes.editedModel()->setSelected(nullptr);
}

// draw contextual actions (buttons, sliders) for a selected physical frame
static void DrawPhysicalFrameContextualActions(osc::SelectGeometryPopup& attachGeomPopup,
                                               osc::UndoableUiModel& uim)
{
    OpenSim::PhysicalFrame const* selection = uim.getSelectedAs<OpenSim::PhysicalFrame>();

    ImGui::Columns(2);

    ImGui::TextUnformatted("geometry");
    ImGui::SameLine();
    osc::DrawHelpMarker("Geometry that is attached to this physical frame. Multiple pieces of geometry can be attached to the frame");
    ImGui::NextColumn();

    if (ImGui::Button("add geometry"))
    {
        attachGeomPopup.open();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted("Add geometry to this component. Geometry can be removed by selecting it in the hierarchy editor and pressing DELETE");
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

    if (auto attached = attachGeomPopup.draw(); attached)
    {
        uim.updSelectedAs<OpenSim::PhysicalFrame>()->attachGeometry(attached.release());
        uim.commit("attached geometry");
    }
    ImGui::NextColumn();

    ImGui::TextUnformatted("offset frame");
    ImGui::NextColumn();
    if (ImGui::Button("add offset frame"))
    {
        auto pof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        pof->setName(selection->getName() + "_offsetframe");
        pof->setParentFrame(*selection);

        auto pofptr = pof.get();
        uim.updSelectedAs<OpenSim::PhysicalFrame>()->addComponent(pof.release());
        uim.setSelected(pofptr);
        uim.commit("added offset frame");
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted("Add an OpenSim::OffsetFrame as a child of this Component. Other components in the model can then connect to this OffsetFrame, rather than the base Component, so that it can connect at some offset that is relative to the parent Component");
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    ImGui::NextColumn();

    ImGui::Columns();
}


// draw contextual actions (buttons, sliders) for a selected joint
static void DrawJointContextualActions(osc::UndoableUiModel& uim)
{
    OpenSim::Joint const* selection = uim.getSelectedAs<OpenSim::Joint>();

    if (!selection)
    {
        return;
    }

    ImGui::Columns(2);

    DrawSelectionJointTypeSwitcher(uim);

    // if the joint uses offset frames for both its parent and child frames then
    // it is possible to reorient those frames such that the joint's new zero
    // point is whatever the current arrangement is (effectively, by pre-transforming
    // the parent into the child and assuming a "zeroed" joint is an identity op)
    {
        OpenSim::PhysicalOffsetFrame const* parentPOF = dynamic_cast<OpenSim::PhysicalOffsetFrame const*>(&selection->getParentFrame());
        OpenSim::PhysicalFrame const& childFrame = selection->getChildFrame();

        if (parentPOF)
        {
            ImGui::Text("rezero joint");
            ImGui::NextColumn();
            if (ImGui::Button("rezero"))
            {
                SimTK::Transform parentXform = parentPOF->getTransformInGround(uim.getState());
                SimTK::Transform childXform = childFrame.getTransformInGround(uim.getState());
                SimTK::Transform child2parent = parentXform.invert() * childXform;
                SimTK::Transform newXform = parentPOF->getOffsetTransform() * child2parent;

                OpenSim::PhysicalOffsetFrame* mutableParent = const_cast<OpenSim::PhysicalOffsetFrame*>(parentPOF);
                mutableParent->setOffsetTransform(newXform);

                for (int i = 0, len = selection->numCoordinates(); i < len; ++i)
                {
                    OpenSim::Coordinate const& c = selection->get_coordinates(i);
                    uim.updUiModel().removeCoordinateEdit(c);
                }

                uim.setDirty(true);
                uim.commit("rezeroed joint");
            }
            osc::DrawTooltipIfItemHovered("Re-zero the joint", "Given the joint's current geometry due to joint defaults, coordinate defaults, and any coordinate edits made in the coordinate editor, this will reorient the joint's parent (if it's an offset frame) to match the child's transformation. Afterwards, it will then resets all of the joints coordinates to zero. This effectively sets the 'zero point' of the joint (i.e. the geometry when all coordinates are zero) to match whatever the current geometry is.");
            ImGui::NextColumn();
        }
    }

    // BEWARE: broke
    {
        ImGui::Text("add offset frame");
        ImGui::NextColumn();

        if (ImGui::Button("parent"))
        {
            auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
            pf->setParentFrame(selection->getParentFrame());
            uim.updSelectedAs<OpenSim::Joint>()->addFrame(pf.release());
            uim.commit("added parent offset frame");
        }
        ImGui::SameLine();
        if (ImGui::Button("child"))
        {
            auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
            pf->setParentFrame(selection->getChildFrame());
            uim.updSelectedAs<OpenSim::Joint>()->addFrame(pf.release());
            uim.commit("added child offset frame");
        }
        ImGui::NextColumn();
    }

    ImGui::Columns();
}

// draw contextual actions (buttons, sliders) for a selected joint
static void DrawHCFContextualActions(osc::UndoableUiModel& uim)
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

    // HACK: if it has no parameters, give it some. The HuntCrossleyForce implementation effectively
    // does this internally anyway to satisfy its own API (e.g. `getStaticFriction` requires that
    // the HuntCrossleyForce has a parameter)
    if (hcf->get_contact_parameters().getSize() == 0)
    {
        uim.updSelectedAs<OpenSim::HuntCrossleyForce>()->updContactParametersSet().adoptAndAppend(new OpenSim::HuntCrossleyForce::ContactParameters());
        // no commit msg: it's a hack
    }

    OpenSim::HuntCrossleyForce::ContactParameters const& params = hcf->get_contact_parameters()[0];

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

        OpenSim::ContactGeometry const* added =
            osc::SelectComponentPopup<OpenSim::ContactGeometry>{}.draw("select contact geometry", uim.getModel());

        if (added)
        {
            uim.updSelectedAs<OpenSim::HuntCrossleyForce>()->updContactParametersSet()[0].updGeometry().appendValue(added->getName());
            uim.commit("added contact geometry");
        }
    }

    ImGui::NextColumn();
    ImGui::Columns();

    // render standard, easy to render, props of the contact params
    {
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
            uim.setDirty(true);
            maybe_updater->updater(const_cast<OpenSim::AbstractProperty&>(maybe_updater->prop));
            uim.commit("edited property");
        }
    }
}

// draw contextual actions (buttons, sliders) for a selected path actuator
static void DrawPathActuatorContextualParams(osc::UndoableUiModel& uim)
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
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(
            "Add a new path point, attached to an OpenSim::PhysicalFrame in the model, to the end of the sequence of path points in this OpenSim::PathActuator");
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

    // handle popup
    {
        OpenSim::PhysicalFrame const* pf = osc::Select1PFPopup{}.draw(modalName, uim.getModel());
        if (pf) {
            int n = pa->getGeometryPath().getPathPointSet().getSize();
            char buf[128];
            std::snprintf(buf, sizeof(buf), "%s-P%i", pa->getName().c_str(), n + 1);
            std::string name{buf};
            SimTK::Vec3 pos{0.0f, 0.0f, 0.0f};

            uim.updSelectedAs<OpenSim::PathActuator>()->addNewPathPoint(name, *pf, pos);
            uim.commit("added path point to path actuator");
        }
    }

    ImGui::NextColumn();
    ImGui::Columns();
}

static void DrawModelContextualActions(osc::UndoableUiModel& uum)
{
    OpenSim::Model const* m = uum.getSelectedAs<OpenSim::Model>();

    if (!m)
    {
        return;
    }

    ImGui::Columns(2);
    ImGui::Text("show frames");
    ImGui::NextColumn();
    bool showingFrames = m->get_ModelVisualPreferences().get_ModelDisplayHints().get_show_frames();

    if (ImGui::Button(showingFrames ? "hide" : "show"))
    {
        uum.updSelectedAs<OpenSim::Model>()->upd_ModelVisualPreferences().upd_ModelDisplayHints().set_show_frames(!showingFrames);
        uum.commit("toggled frame visibility");
    }
    ImGui::NextColumn();
    ImGui::Columns();
}

// draw socket editor for current selection
static void DrawSocketEditor(osc::ReassignSocketPopup& reassignSocketPopup,
                             osc::UndoableUiModel& uim)
{
    OpenSim::Component const* selected = uim.getSelected();

    if (!selected)
    {
        ImGui::TextUnformatted("cannot draw socket editor: selection is blank (shouldn't be)");
        return;
    }

    std::vector<std::string> socknames = const_cast<OpenSim::Component*>(selected)->getSocketNames();

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

            OpenSim::Object const& existing = socket.getConnecteeAsObject();
            try
            {
                uim.updSelected()->updSocket(sn).connect(*connectee);
                reassignSocketPopup.clear();
                ImGui::CloseCurrentPopup();
                uim.commit("reassigned socket");
            }
            catch (std::exception const& ex)
            {
                reassignSocketPopup.setError(ex.what());
                uim.updSelected()->updSocket(sn).connect(existing);
            }
        }

        ImGui::NextColumn();
    }
    ImGui::Columns();
}

// draw breadcrumbs for current selection
//
// eg: Model > Joint > PhysicalFrame
static void DrawSelectionBreadcrumbs(osc::UndoableUiModel& uim)
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
        ImGui::Dummy(ImVec2{indent, 0.0f});
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

    ImGui::Dummy(ImVec2{indent, 0.0f});
    ImGui::SameLine();
    ImGui::TextUnformatted((*(lst.end() - 1))->getName().c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("(%s)", (*(lst.end() - 1))->getConcreteClassName().c_str());
}


static void DrawSelectOwnerMenu(osc::MainEditorState& st,
                                OpenSim::Component const& selected)
{
    if (ImGui::BeginMenu("Select Owner"))
    {
        std::shared_ptr<osc::UndoableUiModel> editedModel = st.editedModel();

        OpenSim::Component const* c = &selected;
        editedModel->setHovered(nullptr);
        while (c->hasOwner())
        {
            c = &c->getOwner();

            char buf[128];
            std::snprintf(buf, sizeof(buf), "%s (%s)", c->getName().c_str(), c->getConcreteClassName().c_str());

            if (ImGui::MenuItem(buf))
            {
                editedModel->setSelected(c);
            }
            if (ImGui::IsItemHovered())
            {
                editedModel->setHovered(c);
            }
        }
        ImGui::EndMenu();
    }
}

static void DrawOutputWithSubfieldsMenu(osc::MainEditorState& st,
                                        OpenSim::AbstractOutput const& o)
{
    int supportedSubfields = osc::GetSupportedSubfields(o);

    // can plot suboutputs
    if (ImGui::BeginMenu(("  " + o.getName()).c_str()))
    {
        for (osc::OutputSubfield f : osc::GetAllSupportedOutputSubfields())
        {
            if (static_cast<int>(f) & supportedSubfields)
            {
                if (ImGui::MenuItem(GetOutputSubfieldLabel(f)))
                {
                    st.addUserOutputExtractor(osc::OutputExtractor{osc::ComponentOutputExtractor{o, f}});
                }
            }
        }
        ImGui::EndMenu();
    }

    if (ImGui::IsItemHovered())
    {
        DrawOutputTooltip(o);
    }
}

static void DrawOutputWithNoSubfieldsMenuItem(osc::MainEditorState& st,
                                              OpenSim::AbstractOutput const& o)
{
    // can only plot top-level of output

    if (ImGui::MenuItem(("  " + o.getName()).c_str()))
    {
       st.addUserOutputExtractor(osc::OutputExtractor{osc::ComponentOutputExtractor{o}});
    }

    if (ImGui::IsItemHovered())
    {
        DrawOutputTooltip(o);
    }
}

static void DrawRequestOutputMenuOrMenuItem(osc::MainEditorState& st,
                                            OpenSim::AbstractOutput const& o)
{
    if (osc::GetSupportedSubfields(o) == static_cast<int>(osc::OutputSubfield::None))
    {
        DrawOutputWithNoSubfieldsMenuItem(st, o);
    }
    else
    {
        DrawOutputWithSubfieldsMenu(st, o);
    }
}

static void DrawRequestOutputsMenu(osc::MainEditorState& st,
                                   OpenSim::Component const& c)
{
    if (ImGui::BeginMenu("Request Outputs"))
    {
        osc::DrawHelpMarker("Request that these outputs are plotted whenever a simulation is ran. The outputs will appear in the 'outputs' tab on the simulator screen");

        // iterate from the selected component upwards to the root
        int imguiId = 0;
        OpenSim::Component const* p = &c;
        while (p)
        {
            ImGui::PushID(imguiId++);

            ImGui::Dummy({0.0f, 2.0f});
            ImGui::TextDisabled("%s (%s)", p->getName().c_str(), p->getConcreteClassName().c_str());
            ImGui::Separator();

            if (p->getNumOutputs() == 0)
            {
                ImGui::TextDisabled("  (has no outputs)");
            }
            else
            {
                for (auto const& [name, output] : p->getOutputs())
                {
                    DrawRequestOutputMenuOrMenuItem(st, *output);
                }
            }

            ImGui::PopID();

            p = p->hasOwner() ? &p->getOwner() : nullptr;
        }

        ImGui::EndMenu();
    }
}

// draw right-click context menu for the 3D viewer
static void Draw3DViewerContextMenu(osc::MainEditorState& st,
                                    OpenSim::Component const& selected)
{
    ImGui::TextDisabled("%s (%s)", selected.getName().c_str(), selected.getConcreteClassName().c_str());
    ImGui::Separator();
    ImGui::Dummy({0.0f, 3.0f});

    DrawSelectOwnerMenu(st, selected);
    DrawRequestOutputsMenu(st, selected);
}

// draw a single 3D model viewer
static bool Draw3DViewer(osc::MainEditorState& st,
                         osc::UiModelViewer& viewer,
                         char const* name)
{
    bool isOpen = true;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
    bool shown = ImGui::Begin(name, &isOpen, ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar();

    if (!isOpen)
    {
        ImGui::End();
        return false;
    }

    if (!shown)
    {
        ImGui::End();
        return true;
    }

    std::shared_ptr<osc::UndoableUiModel> editedModel = st.editedModel();

    auto resp = viewer.draw(editedModel->getUiModel());
    ImGui::End();

    // update hover
    if (resp.isMousedOver && resp.hovertestResult != editedModel->getHovered())
    {
        editedModel->setHovered(resp.hovertestResult);
    }

    // if left-clicked, update selection
    if (resp.isMousedOver && resp.isLeftClicked)
    {
        editedModel->setSelected(resp.hovertestResult);
    }

    // if hovered, draw hover tooltip
    if (resp.isMousedOver && resp.hovertestResult)
    {
        DrawComponentHoverTooltip(*resp.hovertestResult, resp.mouse3DLocation);
    }

    // if right-clicked, draw context menu
    {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "%s_contextmenu", name);

        if (resp.isMousedOver && resp.hovertestResult && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        {
            editedModel->setSelected(resp.hovertestResult);
            ImGui::OpenPopup(buf);
        }

        OpenSim::Component const* selected = editedModel->getSelected();

        if (selected && ImGui::BeginPopup(buf))
        {
            Draw3DViewerContextMenu(st, *selected);
            ImGui::EndPopup();
        }
    }

    return true;
}

// draw all user-enabled 3D model viewers
static void Draw3DViewers(osc::MainEditorState& st)
{
    for (int i = 0; i < st.getNumViewers(); ++i)
    {
        osc::UiModelViewer& viewer = st.updViewer(i);

        char buf[64];
        std::snprintf(buf, sizeof(buf), "viewer%i", i);

        bool isOpen = Draw3DViewer(st, viewer, buf);

        if (!isOpen)
        {
            st.removeViewer(i);
            --i;
        }
    }
}

static std::string GetDocumentName(osc::UndoableUiModel const& uim)
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

static std::string GetRecommendedTitle(osc::UndoableUiModel const& uim)
{
    std::string s = GetDocumentName(uim);
    if (!uim.isUpToDateWithFilesystem())
    {
        s += '*';
    }
    return s;
}

// editor (internal) screen state
class osc::ModelEditorScreen::Impl final {
public:

    explicit Impl(std::shared_ptr<MainEditorState> _st) :
        m_Mes{std::move(_st)}
    {
    }

    void onMount()
    {
        App::cur().makeMainEventLoopWaiting();
        App::cur().setMainWindowSubTitle(GetRecommendedTitle(*m_Mes->editedModel()));
        osc::ImGuiInit();
    }

    void onUnmount()
    {
        osc::ImGuiShutdown();
        App::cur().unsetMainWindowSubTitle();
        App::cur().makeMainEventLoopPolling();
    }

    void tick()
    {
        if (m_FileChangePoller.changeWasDetected(m_Mes->editedModel()->getModel().getInputFileName()))
        {
            onBackingFileChanged();
        }

        App::cur().setMainWindowSubTitle(GetRecommendedTitle(*m_Mes->editedModel()));
    }

    void draw()
    {
        App::cur().clearScreen({0.0f, 0.0f, 0.0f, 0.0f});
        osc::ImGuiNewFrame();
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
        drawGUARDED();
        osc::ImGuiRender();
    }

    void onEvent(SDL_Event const& e)
    {
        if (e.type == SDL_QUIT)
        {
            onQuitEvent(e.quit);
        }
        else if (osc::ImGuiOnEvent(e))
        {
            m_ResetPerFrame.shouldRequestRedraw = true;
        }
        else if (e.type == SDL_KEYDOWN)
        {
            onKeydown(e.key);
        }
        else if (e.type == SDL_DROPFILE)
        {
            onDropEvent(e.drop);
        }
    }

private:

    void onDropEvent(SDL_DropEvent const& e)
    {
        if (e.file != nullptr && CStrEndsWith(e.file, ".sto"))
        {
            try
            {
                std::filesystem::path p{e.file};
                std::unique_ptr<OpenSim::Model> cpy =
                    std::make_unique<OpenSim::Model>(m_Mes->editedModel()->getModel());
                cpy->buildSystem();
                cpy->initializeState();
                m_Mes->addSimulation(Simulation{StoFileSimulation{std::move(cpy), p}});
                osc::App::cur().requestTransition<osc::SimulatorScreen>(m_Mes);
            }
            catch (std::exception const& ex)
            {
                log::error("encountered error while trying to load an STO file against the model: %s", ex.what());
            }
        }
    }

    void onQuitEvent(SDL_QuitEvent const&)
    {
        if (m_Mes->editedModel()->isUpToDateWithFilesystem())
        {
            App::cur().requestQuit();
        }
        else
        {
            SaveChangesPopupConfig cfg;
            cfg.onUserClickedSave = [this]()
            {
                if (actionSaveModel(m_Mes))
                {
                    App::cur().requestQuit();
                    return true;
                }
                else
                {
                    return false;
                }
            };
            cfg.onUserClickedDontSave = []()
            {
                App::cur().requestQuit();
                return true;
            };
            m_MaybeSaveChangesPopup = SaveChangesPopup{std::move(cfg)};
            m_MaybeSaveChangesPopup->open();
        }
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
                    ActionRedoCurrentlyEditedModel(*m_Mes);
                    return true;
                }
                return false;
            }

            switch (e.keysym.sym) {
            case SDLK_z:  // Ctrl+Z: undo focused model
                ActionUndoCurrentlyEditedModel(*m_Mes);
                return true;
            case SDLK_r:  // Ctrl+R: start a new simulation from focused model
                ActionStartSimulationFromEditedModel(*m_Mes);
                osc::App::cur().requestTransition<osc::SimulatorScreen>(m_Mes);
                return true;
            case SDLK_a:  // Ctrl+A: clear selection
                ActionClearSelectionFromEditedModel(*m_Mes);
                return true;
            case SDLK_e:  // Ctrl+E: show simulation screen
                osc::App::cur().requestTransition<osc::SimulatorScreen>(std::move(m_Mes));
                return true;
            }

            return false;
        }

        switch (e.keysym.sym) {
        case SDLK_BACKSPACE:
        case SDLK_DELETE:  // BACKSPACE/DELETE: delete selection
            ActionTryDeleteSelectionFromEditedModel(*m_Mes->editedModel());
            return true;
        }

        return false;
    }

    // handle what happens when the underlying model file changes
    void onBackingFileChanged()
    {
        try
        {
            std::shared_ptr<osc::UndoableUiModel> editedModel = m_Mes->editedModel();

            osc::log::info("file change detected: loading updated file");
            auto p = std::make_unique<OpenSim::Model>(editedModel->getModel().getInputFileName());
            osc::log::info("loaded updated file");
            editedModel->setModel(std::move(p));
            editedModel->setUpToDateWithFilesystem();
            editedModel->commit("reloaded model from filesystem");
        }
        catch (std::exception const& ex)
        {
            osc::log::error("error occurred while trying to automatically load a model file:");
            osc::log::error(ex.what());
            osc::log::error("the file will not be loaded into osc (you won't see the change in the UI)");
        }
    }

    // draw contextual actions for selection
    void drawContextualActions()
    {
        std::shared_ptr<osc::UndoableUiModel> editedModel = m_Mes->editedModel();

        if (!editedModel->hasSelected())
        {
            ImGui::TextUnformatted("cannot draw contextual actions: selection is blank (shouldn't be)");
            return;
        }

        ImGui::Columns(2);
        ImGui::TextUnformatted("isolate in visualizer");
        ImGui::NextColumn();

        if (editedModel->getSelected() != editedModel->getIsolated())
        {
            if (ImGui::Button("isolate"))
            {
                editedModel->setIsolated(editedModel->getSelected());
            }
        }
        else
        {
            if (ImGui::Button("clear isolation"))
            {
                editedModel->setIsolated(nullptr);
            }
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(
                "Only show this component in the visualizer\n\nThis can be disabled from the Edit menu (Edit -> Clear Isolation)");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        ImGui::NextColumn();


        ImGui::TextUnformatted("copy abspath");
        ImGui::NextColumn();
        if (ImGui::Button("copy"))
        {
            osc::SetClipboardText(editedModel->getSelected()->getAbsolutePathString().c_str());
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(
                "Copy the absolute path to this component to your clipboard.\n\n(This is handy if you are separately using absolute component paths to (e.g.) manipulate the model in a script or something)");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        ImGui::NextColumn();

        ImGui::Columns();

        if (editedModel->selectionIsType<OpenSim::Model>())
        {
            DrawModelContextualActions(*editedModel);
        }
        else if (editedModel->selectionDerivesFrom<OpenSim::PhysicalFrame>())
        {
            DrawPhysicalFrameContextualActions(m_AttachGeomPopup, *editedModel);
        }
        else if (editedModel->selectionDerivesFrom<OpenSim::Joint>())
        {
            DrawJointContextualActions(*editedModel);
        }
        else if (editedModel->selectionIsType<OpenSim::HuntCrossleyForce>())
        {
            DrawHCFContextualActions(*editedModel);
        }
        else if (editedModel->selectionDerivesFrom<OpenSim::PathActuator>())
        {
            DrawPathActuatorContextualParams(*editedModel);
        }
    }

    void drawSelectionEditor()
    {
        std::shared_ptr<osc::UndoableUiModel> editedModel = m_Mes->editedModel();

        if (!editedModel->hasSelected())
        {
            ImGui::TextUnformatted("(nothing selected)");
            return;
        }

        ImGui::PushID(editedModel->getSelected());

        ImGui::Dummy({0.0f, 1.0f});
        ImGui::TextUnformatted("hierarchy:");
        ImGui::SameLine();
        osc::DrawHelpMarker("Where the selected component is in the model's component hierarchy");
        ImGui::Separator();
        DrawSelectionBreadcrumbs(*editedModel);

        // contextual actions
        ImGui::Dummy({0.0f, 5.0f});
        ImGui::TextUnformatted("contextual actions:");
        ImGui::SameLine();
        osc::DrawHelpMarker("Actions that are specific to the type of OpenSim::Component that is currently selected");
        ImGui::Separator();
        drawContextualActions();

        // a contextual action may have changed this
        if (!editedModel->hasSelected())
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
            DrawTopLevelMembersEditor(*editedModel);
        }

        // top-level member edits may have changed this
        if (!editedModel->getSelected())
        {
            return;
        }

        // property editors
        {
            auto maybeUpdater = m_ObjectPropsEditor.draw(*editedModel->getSelected());
            if (maybeUpdater)
            {
                editedModel->setDirty(true);
                maybeUpdater->updater(const_cast<OpenSim::AbstractProperty&>(maybeUpdater->prop));
                editedModel->commit("edited component property");
            }
        }

        // socket editor
        ImGui::Dummy({0.0f, 5.0f});
        ImGui::TextUnformatted("sockets:");
        ImGui::SameLine();
        osc::DrawHelpMarker("What components this component is connected to.\n\nIn OpenSim, a Socket formalizes the dependency between a Component and another object (typically another Component) without owning that object. While Components can be composites (of multiple components) they often depend on unrelated objects/components that are defined and owned elsewhere. The object that satisfies the requirements of the Socket we term the 'connectee'. When a Socket is satisfied by a connectee we have a successful 'connection' or is said to be connected.");
        ImGui::Separator();
        DrawSocketEditor(m_ReassignSocketPopup, *editedModel);

        ImGui::PopID();
    }

    void drawMainMenuEditTab()
    {
        std::shared_ptr<osc::UndoableUiModel> editedModel = m_Mes->editedModel();

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem(ICON_FA_UNDO " Undo", "Ctrl+Z", false, editedModel->canUndo()))
            {
                ActionUndoCurrentlyEditedModel(*m_Mes);
            }

            if (ImGui::MenuItem(ICON_FA_REDO " Redo", "Ctrl+Shift+Z", false, editedModel->canRedo()))
            {
                ActionRedoCurrentlyEditedModel(*m_Mes);
            }

            if (ImGui::MenuItem(ICON_FA_EYE_SLASH " Clear Isolation", nullptr, false, editedModel->getIsolated()))
            {
                editedModel->setIsolated(nullptr);
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted("Clear currently isolation setting. This is effectively the opposite of 'Isolate'ing a component.");
                if (!editedModel->getIsolated())
                {
                    ImGui::TextDisabled("\n(disabled because nothing is currently isolated)");
                }
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            float scaleFactor = editedModel->getFixupScaleFactor();

            if (ImGui::InputFloat("set scale factor", &scaleFactor))
            {
                editedModel->setFixupScaleFactor(scaleFactor);
            }

            if (ImGui::MenuItem("autoscale scale factor"))
            {
                float sf = osc::GetRecommendedScaleFactor(*editedModel);
                editedModel->setFixupScaleFactor(sf);
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::Text("Try to autoscale the model's scale factor based on the current dimensions of the model");
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            bool showingFrames = editedModel->getModel().get_ModelVisualPreferences().get_ModelDisplayHints().get_show_frames();
            if (ImGui::MenuItem(showingFrames ? "hide frames" : "show frames"))
            {
                editedModel->updModel().upd_ModelVisualPreferences().upd_ModelDisplayHints().set_show_frames(!showingFrames);
                editedModel->commit("edited frame visibility");
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::Text("Set the model's display properties to display physical frames");
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            bool modelHasBackingFile = osc::HasInputFileName(editedModel->getModel());

            if (ImGui::MenuItem(ICON_FA_FOLDER " Open .osim's parent directory", nullptr, false, modelHasBackingFile))
            {
                std::filesystem::path p{editedModel->getModel().getInputFileName()};
                osc::OpenPathInOSDefaultApplication(p.parent_path());
            }

            if (ImGui::MenuItem(ICON_FA_LINK " Open .osim in external editor", nullptr, false, modelHasBackingFile))
            {
                osc::OpenPathInOSDefaultApplication(editedModel->getModel().getInputFileName());
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted("Open the .osim file currently being edited in an external text editor. The editor that's used depends on your operating system's default for opening .osim files.");
                if (!osc::HasInputFileName(editedModel->getModel()))
                {
                    ImGui::TextDisabled("\n(disabled because the currently-edited model has no backing file)");
                }
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            ImGui::EndMenu();
        }
    }

    void drawMainMenuSimulateTab()
    {
        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem(ICON_FA_PLAY " Simulate", "Ctrl+R"))
            {
                StartSimulatingEditedModel(*m_Mes);
                osc::App::cur().requestTransition<osc::SimulatorScreen>(m_Mes);
                m_ResetPerFrame.subpanelRequestedEarlyExit = true;
            }

            if (ImGui::MenuItem(ICON_FA_EDIT " Edit simulation settings"))
            {
                m_ResetPerFrame.editSimParamsRequested = true;
            }

            if (ImGui::MenuItem("Disable all wrapping surfaces"))
            {
                ActionDisableAllWrappingSurfaces(*m_Mes);
            }

            if (ImGui::MenuItem("Enable all wrapping surfaces"))
            {
                ActionEnableAllWrappingSurfaces(*m_Mes);
            }

            ImGui::EndMenu();
        }
    }

    void drawMainMenu()
    {
        if (ImGui::BeginMainMenuBar())
        {
            m_MainMenuFileTab.draw(m_Mes);
            drawMainMenuEditTab();
            drawMainMenuSimulateTab();
            m_MainMenuWindowTab.draw(*m_Mes);
            m_MainMenuAboutTab.draw();

            ImGui::Dummy({2.0f, 0.0f});
            if (ImGui::Button(ICON_FA_LIST_ALT " Switch to simulator (Ctrl+E)"))
            {
                osc::App::cur().requestTransition<osc::SimulatorScreen>(std::move(m_Mes));
                ImGui::EndMainMenuBar();
                m_ResetPerFrame.subpanelRequestedEarlyExit = true;
                return;
            }

            // "switch to simulator" menu button
            ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
            if (ImGui::Button(ICON_FA_PLAY " Simulate (Ctrl+R)"))
            {
                StartSimulatingEditedModel(*m_Mes);
                osc::App::cur().requestTransition<osc::SimulatorScreen>(std::move(m_Mes));
                ImGui::PopStyleColor();
                ImGui::EndMainMenuBar();
                m_ResetPerFrame.subpanelRequestedEarlyExit = true;
                return;
            }
            ImGui::PopStyleColor();

            if (ImGui::Button(ICON_FA_EDIT " Edit simulation settings"))
            {
                m_ResetPerFrame.editSimParamsRequested = true;
            }

            ImGui::EndMainMenuBar();
        }
    }

    void drawUNGUARDED()
    {
        if (m_ResetPerFrame.shouldRequestRedraw)
        {
            osc::App::cur().requestRedraw();
        }

        m_ResetPerFrame = {};

        // draw main menu
        {
            drawMainMenu();
        }

        // check for early exit request
        //
        // (the main menu may have requested a screen transition)
        if (m_ResetPerFrame.subpanelRequestedEarlyExit)
        {
            return;
        }

        // draw 3D viewers (if any)
        {
            Draw3DViewers(*m_Mes);
        }

        // draw editor actions panel
        //
        // contains top-level actions (e.g. "add body")
        if (m_Mes->getUserPanelPrefs().actions)
        {
            if (ImGui::Begin("Actions", &m_Mes->updUserPanelPrefs().actions, ImGuiWindowFlags_MenuBar))
            {
                m_ModelActionsMenuBar.draw();
            }
            ImGui::End();
        }

        // draw hierarchy viewer
        if (m_Mes->getUserPanelPrefs().hierarchy)
        {
            if (ImGui::Begin("Hierarchy", &m_Mes->updUserPanelPrefs().hierarchy))
            {
                std::shared_ptr<osc::UndoableUiModel> editedModel = m_Mes->editedModel();

                auto resp = m_ComponentHierarchy.draw(
                    &editedModel->getModel().getRoot(),
                    editedModel->getSelected(),
                    editedModel->getHovered());

                if (resp.type == osc::ComponentHierarchy::SelectionChanged)
                {
                    editedModel->setSelected(resp.ptr);
                }
                else if (resp.type == osc::ComponentHierarchy::HoverChanged)
                {
                    editedModel->setHovered(resp.ptr);
                }
            }
            ImGui::End();
        }

        // draw property editor
        if (m_Mes->getUserPanelPrefs().propertyEditor)
        {
            if (ImGui::Begin("Edit Props", &m_Mes->updUserPanelPrefs().propertyEditor))
            {
                drawSelectionEditor();
            }
            ImGui::End();
        }

        // draw application log
        if (m_Mes->getUserPanelPrefs().log)
        {
            if (ImGui::Begin("Log", &m_Mes->updUserPanelPrefs().log, ImGuiWindowFlags_MenuBar))
            {
                m_LogViewer.draw();
            }
            ImGui::End();
        }

        // draw coordinate editor
        if (m_Mes->getUserPanelPrefs().coordinateEditor)
        {
            if (ImGui::Begin("Coordinate Editor", &m_Mes->updUserPanelPrefs().coordinateEditor))
            {
                m_CoordEditor.draw();
            }
        }

        // draw sim params editor popup (if applicable)
        {
            if (m_ResetPerFrame.editSimParamsRequested)
            {
                ImGui::OpenPopup("simulation parameters");
            }

            osc::ParamBlockEditorPopup{}.draw("simulation parameters", m_Mes->updSimulationParams());
        }

        if (m_MaybeSaveChangesPopup)
        {
            m_MaybeSaveChangesPopup->draw();
        }
    }

    void drawGUARDED()
    {
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
                App::cur().requestTransition<ErrorScreen>(ex);
            }
            else
            {
                try
                {
                    m_Mes->editedModel()->rollback();
                    log::error("model rollback succeeded");
                    m_ExceptionThrownLastFrame = true;
                }
                catch (std::exception const& ex2)
                {
                    App::cur().requestTransition<ErrorScreen>(ex2);
                }
            }

            // try to put ImGui into a clean state
            osc::ImGuiShutdown();
            osc::ImGuiInit();
            osc::ImGuiNewFrame();
        }
    }

    // top-level state this screen can handle
    std::shared_ptr<MainEditorState> m_Mes;

    // polls changes to a file
    FileChangePoller m_FileChangePoller{std::chrono::milliseconds{1000}, m_Mes->editedModel()->getModel().getInputFileName()};

    // UI widgets/popups
    MainMenuFileTab m_MainMenuFileTab;
    MainMenuWindowTab m_MainMenuWindowTab;
    MainMenuAboutTab m_MainMenuAboutTab;
    ObjectPropertiesEditor m_ObjectPropsEditor;
    ReassignSocketPopup m_ReassignSocketPopup;
    Select2PFsPopup m_Select2PFsPopup;
    LogViewer m_LogViewer;
    ComponentHierarchy m_ComponentHierarchy;
    ModelActionsMenuBar m_ModelActionsMenuBar{m_Mes->editedModel()};
    CoordinateEditor m_CoordEditor{m_Mes->editedModel()};
    SelectGeometryPopup m_AttachGeomPopup{"select geometry to add"};
    std::optional<SaveChangesPopup> m_MaybeSaveChangesPopup;

    // state that is reset at the start of each frame
    struct {
        bool editSimParamsRequested = false;
        bool subpanelRequestedEarlyExit = false;
        bool shouldRequestRedraw = false;
    } m_ResetPerFrame;

    // flag that's set+reset each frame to prevent continual
    // throwing
    bool m_ExceptionThrownLastFrame = false;
};


// public API

osc::ModelEditorScreen::ModelEditorScreen(std::shared_ptr<MainEditorState> st) :
    m_Impl{new Impl {std::move(st)}}
{
}

osc::ModelEditorScreen::~ModelEditorScreen() noexcept = default;

void osc::ModelEditorScreen::onMount()
{
    m_Impl->onMount();
}

void osc::ModelEditorScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::ModelEditorScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::ModelEditorScreen::tick(float)
{
    m_Impl->tick();
}

void osc::ModelEditorScreen::draw()
{
    m_Impl->draw();
}
