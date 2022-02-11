#include "ModelEditorScreen.hpp"

#include "src/3D/Gl.hpp"
#include "src/OpenSimBindings/FileChangePoller.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/Screens/ErrorScreen.hpp"
#include "src/Screens/SimulatorScreen.hpp"
#include "src/UI/AddBodyPopup.hpp"
#include "src/UI/AttachGeometryPopup.hpp"
#include "src/UI/CoordinateEditor.hpp"
#include "src/UI/ComponentDetails.hpp"
#include "src/UI/ComponentHierarchy.hpp"
#include "src/UI/FdParamsEditorPopup.hpp"
#include "src/UI/MainMenu.hpp"
#include "src/UI/ModelActionsMenuBar.hpp"
#include "src/UI/LogViewer.hpp"
#include "src/UI/PropertyEditors.hpp"
#include "src/UI/ReassignSocketPopup.hpp"
#include "src/UI/SelectComponentPopup.hpp"
#include "src/UI/Select1PFPopup.hpp"
#include "src/UI/Select2PFsPopup.hpp"
#include "src/Utils/ScopeGuard.hpp"
#include "src/Utils/ImGuiHelpers.hpp"
#include "src/App.hpp"
#include "src/Log.hpp"
#include "src/MainEditorState.hpp"
#include "src/os.hpp"
#include "src/Styling.hpp"

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

using namespace osc;

namespace {

    // returns the first ancestor of `c` that has type `T`
    template<typename T>
    [[nodiscard]] T const* findAncestorWithType(OpenSim::Component const* c) {
        while (c) {
            T const* p = dynamic_cast<T const*>(c);
            if (p) {
                return p;
            }
            c = c->hasOwner() ? &c->getOwner() : nullptr;
        }
        return nullptr;
    }

    // returns true if the model has a backing file
    [[nodiscard]] bool hasBackingFile(OpenSim::Model const& m) {
        return m.getInputFileName() != "Unassigned";
    }

    // copy common joint properties from a `src` to `dest`
    //
    // e.g. names, coordinate names, etc.
    void copyCommonJointProperties(OpenSim::Joint const& src, OpenSim::Joint& dest) {
        dest.setName(src.getName());

        // copy owned frames
        dest.updProperty_frames().assign(src.getProperty_frames());

        // copy, or reference, the parent based on whether the source owns it
        {
            OpenSim::PhysicalFrame const& srcParent = src.getParentFrame();
            bool parentAssigned = false;
            for (int i = 0; i < src.getProperty_frames().size(); ++i) {
                if (&src.get_frames(i) == &srcParent) {
                    // the source's parent is also owned by the source, so we need to
                    // ensure the destination refers to its own (cloned, above) copy
                    dest.connectSocket_parent_frame(dest.get_frames(i));
                    parentAssigned = true;
                    break;
                }
            }
            if (!parentAssigned) {
                // the source's parent is a reference to some frame that the source
                // doesn't, itself, own, so the destination should just also refer
                // to the same (not-owned) frame
                dest.connectSocket_parent_frame(srcParent);
            }
        }

        // copy, or reference, the child based on whether the source owns it
        {
            OpenSim::PhysicalFrame const& srcChild = src.getChildFrame();
            bool childAssigned = false;
            for (int i = 0; i < src.getProperty_frames().size(); ++i) {
                if (&src.get_frames(i) == &srcChild) {
                    // the source's child is also owned by the source, so we need to
                    // ensure the destination refers to its own (cloned, above) copy
                    dest.connectSocket_child_frame(dest.get_frames(i));
                    childAssigned = true;
                    break;
                }
            }
            if (!childAssigned) {
                // the source's child is a reference to some frame that the source
                // doesn't, itself, own, so the destination should just also refer
                // to the same (not-owned) frame
                dest.connectSocket_child_frame(srcChild);
            }
        }
    }

    // delete an item from an OpenSim::Set
    template<typename T, typename TSetBase = OpenSim::Object>
    void deleteItemFromSet(OpenSim::Set<T, TSetBase>& set, T const* item) {
        for (int i = 0; i < set.getSize(); ++i) {
            if (&set.get(i) == item) {
                set.remove(i);
                return;
            }
        }
    }

    // draw component information as a hover tooltip
    void drawComponentHoverTooltip(OpenSim::Component const& hovered, glm::vec3 const& pos) {
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

    // try to delete an undoable-model's current selection
    //
    // "try", because some things are difficult to delete from OpenSim models
    void actionTryDeleteSelectionFromEditedModel(UndoableUiModel& uim) {
        OpenSim::Component* selected = uim.updSelected();

        if (!selected) {
            return;  // nothing selected, so nothing can be deleted
        }

        if (!selected->hasOwner()) {
            // the selected item isn't owned by anything, so it can't be deleted from its
            // owner's hierarchy
            return;
        }

        OpenSim::Component* owner = const_cast<OpenSim::Component*>(&selected->getOwner());

        // else: an OpenSim::Component is selected and we need to figure out how to remove it
        // from its parent
        //
        // this is uglier than it should be because OpenSim doesn't have a uniform approach for
        // storing Components in the model hierarchy. Some Components might be in specialized sets,
        // some Components might be in std::vectors, some might be solo children, etc.
        //
        // the challenge is knowing what component is selected, what kind of parent it's contained
        // within, and how that particular component type can be safely deleted from that particular
        // parent type without leaving the overall Model in an invalid state

        if (auto* js = dynamic_cast<OpenSim::JointSet*>(owner); js) {
            // delete an OpenSim::Joint from its owning OpenSim::JointSet

            deleteItemFromSet(*js, static_cast<OpenSim::Joint*>(selected));
            uim.setDirty(true);
            uim.declareDeathOf(selected);
        } else if (auto* bs = dynamic_cast<OpenSim::BodySet*>(owner); bs) {
            // delete an OpenSim::Body from its owning OpenSim::BodySet

            log::error(
                "cannot delete %s: deleting OpenSim::Body is not supported: it segfaults in the OpenSim API",
                selected->getName().c_str());

            // segfaults:
            // uim.before_modifying_model();
            // delete_item_from_set_in_model(*bs, static_cast<OpenSim::Body*>(selected));
            // uim.model().clearConnections();
            // uim.declare_death_of(selected);
            // uim.after_modifying_model();
        } else if (auto* wos = dynamic_cast<OpenSim::WrapObjectSet*>(owner); wos) {
            // delete an OpenSim::WrapObject from its owning OpenSim::WrapObjectSet

            log::error(
                "cannot delete %s: deleting an OpenSim::WrapObject is not supported: faults in the OpenSim API until after AK's connection checking addition",
                selected->getName().c_str());

            // also, this implementation needs to iterate over all pathwraps in the model
            // and disconnect them from the GeometryPath that uses them; otherwise, the model
            // will explode
        } else if (auto* cs = dynamic_cast<OpenSim::ControllerSet*>(owner); cs) {
            // delete an OpenSim::Controller from its owning OpenSim::ControllerSet

            uim.setDirty(true);
            deleteItemFromSet(*cs, static_cast<OpenSim::Controller*>(selected));            
            uim.declareDeathOf(selected);
        } else if (auto* conss = dynamic_cast<OpenSim::ConstraintSet*>(owner); cs) {
            // delete an OpenSim::Constraint from its owning OpenSim::ConstraintSet

            uim.setDirty(true);
            deleteItemFromSet(*conss, static_cast<OpenSim::Constraint*>(selected));
            uim.declareDeathOf(selected);
        } else if (auto* fs = dynamic_cast<OpenSim::ForceSet*>(owner); fs) {
            // delete an OpenSim::Force from its owning OpenSim::ForceSet

            uim.setDirty(true);
            deleteItemFromSet(*fs, static_cast<OpenSim::Force*>(selected));
            uim.declareDeathOf(selected);
        } else if (auto* ms = dynamic_cast<OpenSim::MarkerSet*>(owner); ms) {
            // delete an OpenSim::Marker from its owning OpenSim::MarkerSet

            uim.setDirty(true);
            deleteItemFromSet(*ms, static_cast<OpenSim::Marker*>(selected));
            uim.declareDeathOf(selected);
        } else if (auto* cgs = dynamic_cast<OpenSim::ContactGeometrySet*>(owner); cgs) {
            // delete an OpenSim::ContactGeometry from its owning OpenSim::ContactGeometrySet

            uim.setDirty(true);
            deleteItemFromSet(*cgs, static_cast<OpenSim::ContactGeometry*>(selected));
            uim.declareDeathOf(selected);
        } else if (auto* ps = dynamic_cast<OpenSim::ProbeSet*>(owner); ps) {
            // delete an OpenSim::Probe from its owning OpenSim::ProbeSet

            uim.setDirty(true);
            deleteItemFromSet(*ps, static_cast<OpenSim::Probe*>(selected));
            uim.declareDeathOf(selected);
        } else if (auto const* geom = findAncestorWithType<OpenSim::Geometry>(selected); geom) {
            // delete an OpenSim::Geometry from its owning OpenSim::Frame

            if (auto const* frame = findAncestorWithType<OpenSim::Frame>(geom); frame) {
                // its owner is a frame, which holds the geometry in a list property

                // make a copy of the property containing the geometry and
                // only copy over the not-deleted geometry into the copy
                //
                // this is necessary because OpenSim::Property doesn't seem
                // to support list element deletion, but does support full
                // assignment

                uim.setDirty(true);

                auto& mframe = const_cast<OpenSim::Frame&>(*frame);
                OpenSim::ObjectProperty<OpenSim::Geometry>& prop =
                    static_cast<OpenSim::ObjectProperty<OpenSim::Geometry>&>(mframe.updProperty_attached_geometry());

                std::unique_ptr<OpenSim::ObjectProperty<OpenSim::Geometry>> copy{prop.clone()};
                copy->clear();
                for (int i = 0; i < prop.size(); ++i) {
                    OpenSim::Geometry& g = prop[i];
                    if (&g != geom) {
                        copy->adoptAndAppendValue(g.clone());
                    }
                }

                prop.assign(*copy);
                uim.declareDeathOf(selected);
            }
        } else if (auto* pp = dynamic_cast<OpenSim::PathPoint*>(selected); pp) {
            if (auto* gp = dynamic_cast<OpenSim::GeometryPath*>(owner); gp) {

                OpenSim::PathPointSet const& pps = gp->getPathPointSet();
                int idx = -1;
                for (int i = 0; i < pps.getSize(); ++i) {
                    if (&pps.get(i) == pp) {
                        idx = i;
                    }
                }

                if (idx != -1) {
                    uim.setDirty(true);
                    gp->deletePathPoint(uim.getState(), idx);
                    uim.declareDeathOf(selected);
                }
            }
        }

        uim.updateIfDirty();
    }

    // draw an editor for top-level selected Component members (e.g. name)
    void drawTopLevelMembersEditor(UndoableUiModel& st) {
        OpenSim::Component const* selection = st.getSelected();

        if (!selection) {
            ImGui::Text("cannot draw top level editor: nothing selected?");
            return;
        }

        ImGui::Columns(2);

        ImGui::TextUnformatted("name");
        ImGui::NextColumn();

        char nambuf[128];
        nambuf[sizeof(nambuf) - 1] = '\0';
        std::strncpy(nambuf, selection->getName().c_str(), sizeof(nambuf) - 1);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
        if (ImGui::InputText("##nameditor", nambuf, sizeof(nambuf), ImGuiInputTextFlags_EnterReturnsTrue)) {

            if (std::strlen(nambuf) > 0) {
                st.updSelected()->setName(nambuf);
            }
        }
        ImGui::NextColumn();

        ImGui::Columns();
    }

    // draw UI element that lets user change a model joint's type
    void drawSelectionJointTypeSwitcher(UndoableUiModel& st) {
        OpenSim::Joint const* selection = st.getSelectedAs<OpenSim::Joint>();

        if (!selection) {
            return;
        }

        auto const* parentJointset =
            selection->hasOwner() ? dynamic_cast<OpenSim::JointSet const*>(&selection->getOwner()) : nullptr;

        if (!parentJointset) {
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
        std::optional<size_t> maybeTypeIndex = JointRegistry::indexOf(*selection);
        int typeIndex = maybeTypeIndex ? static_cast<int>(*maybeTypeIndex) : -1;

        auto jointNames = JointRegistry::nameCStrings();

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
        if (ImGui::Combo(
                "##newjointtypeselector",
                &typeIndex,
                jointNames.data(),
                static_cast<int>(jointNames.size())) &&
            typeIndex >= 0) {

            // copy + fixup  a prototype of the user's selection
            std::unique_ptr<OpenSim::Joint> newJoint{JointRegistry::prototypes()[static_cast<size_t>(typeIndex)]->clone()};
            copyCommonJointProperties(*selection, *newJoint);

            // overwrite old joint in model
            //
            // note: this will invalidate the `selection` joint, because the
            // OpenSim::JointSet container will automatically kill it
            OpenSim::Joint* ptr = newJoint.get();
            st.setDirty(true);
            const_cast<OpenSim::JointSet&>(js).set(idx, newJoint.release());
            st.declareDeathOf(selection);
            st.setSelected(ptr);
        }
        ImGui::NextColumn();
    }


    // try to undo currently edited model to earlier state
    void actionUndoCurrentlyEditedModel(MainEditorState& mes) {
        if (mes.editedModel.canUndo()) {
            mes.editedModel.doUndo();
        }
    }

    // try to redo currently edited model to later state
    void actionRedoCurrentlyEditedModel(MainEditorState& mes) {
        if (mes.editedModel.canRedo()) {
            mes.editedModel.doRedo();
        }
    }

    // disable all wrapping surfaces in the current model
    void actionDisableAllWrappingSurfaces(MainEditorState& mes) {
        UndoableUiModel& uim = mes.editedModel;

        OpenSim::Model& m = uim.updModel();
        for (OpenSim::WrapObjectSet& wos : m.updComponentList<OpenSim::WrapObjectSet>()) {
            for (int i = 0; i < wos.getSize(); ++i) {
                OpenSim::WrapObject& wo = wos[i];
                wo.set_active(false);
                wo.upd_Appearance().set_visible(false);
            }
        }
    }

    // enable all wrapping surfaces in the current model
    void actionEnableAllWrappingSurfaces(MainEditorState& mes) {
        UndoableUiModel& uim = mes.editedModel;

        OpenSim::Model& m = uim.updModel();
        for (OpenSim::WrapObjectSet& wos : m.updComponentList<OpenSim::WrapObjectSet>()) {
            for (int i = 0; i < wos.getSize(); ++i) {
                OpenSim::WrapObject& wo = wos[i];
                wo.set_active(true);
                wo.upd_Appearance().set_visible(true);
            }
        }
    }

    // try to start a new simulation from the currently-edited model
    void actionStartSimulationFromEditedModel(MainEditorState& mes) {
        mes.startSimulatingEditedModel();
    }

    void actionClearSelectionFromEditedModel(MainEditorState& mes) {
        mes.editedModel.setSelected(nullptr);
    }
}

// editor (internal) screen state
struct ModelEditorScreen::Impl final {

    // top-level state this screen can handle
    std::shared_ptr<MainEditorState> st;

    // polls changes to a file
    FileChangePoller filePoller;

    // internal state of any sub-panels the editor screen draws
    struct {
        MainMenuFileTab mmFileTab;
        MainMenuWindowTab mmWindowTab;
        MainMenuAboutTab mmAboutTab;
        AddBodyPopup addBodyPopup;
        ObjectPropertiesEditor propertiesEditor;
        ReassignSocketPopup reassignSocketPopup;
        AttachGeometryPopup attachGeometryPopup;
        Select2PFsPopup select2PFsPopup;
        ModelActionsMenuBar modelActions;
        LogViewer logViewer;
        CoordinateEditor coordEditor;
        ComponentHierarchy componentHierarchy;
    } ui;

    // state that is reset at the start of each frame
    struct {
        bool editSimParamsRequested = false;
        bool subpanelRequestedEarlyExit = false;
        bool shouldRequestRedraw = false;
    } resetPerFrame;

    explicit Impl(std::shared_ptr<MainEditorState> _st) :
        st{std::move(_st)},
        filePoller{std::chrono::milliseconds{1000}, st->editedModel.getModel().getInputFileName()} {
    }
};

namespace {

    // handle what happens when a user presses a key
    bool modelEditorOnKeydown(ModelEditorScreen::Impl& impl, SDL_KeyboardEvent const& e) {

        if (e.keysym.mod & KMOD_CTRL) {  // Ctrl
            if (e.keysym.mod & KMOD_SHIFT) {  // Ctrl+Shift
                switch (e.keysym.sym) {
                case SDLK_z:  // Ctrl+Shift+Z : undo focused model
                    actionRedoCurrentlyEditedModel(*impl.st);
                    return true;
                }
                return false;
            }

            switch (e.keysym.sym) {
            case SDLK_z:  // Ctrl+Z: undo focused model
                actionUndoCurrentlyEditedModel(*impl.st);
                return true;
            case SDLK_r:  // Ctrl+R: start a new simulation from focused model
                actionStartSimulationFromEditedModel(*impl.st);
                App::cur().requestTransition<SimulatorScreen>(impl.st);
                return true;
            case SDLK_a:  // Ctrl+A: clear selection
                actionClearSelectionFromEditedModel(*impl.st);
                return true;
            case SDLK_e:  // Ctrl+E: show simulation screen
                App::cur().requestTransition<SimulatorScreen>(std::move(impl.st));
                return true;
            }

            return false;
        }

        switch (e.keysym.sym) {
        case SDLK_DELETE:  // DELETE: delete selection
            actionTryDeleteSelectionFromEditedModel(impl.st->editedModel);
            return true;
        }

        return false;
    }

    // handle what happens when the underlying model file changes
    void modelEditorOnBackingFileChanged(ModelEditorScreen::Impl& impl) {
        try {
            log::info("file change detected: loading updated file");
            auto p = std::make_unique<OpenSim::Model>(impl.st->getModel().getInputFileName());
            log::info("loaded updated file");
            impl.st->setModel(std::move(p));
            impl.st->editedModel.setUpToDateWithFilesystem();
        } catch (std::exception const& ex) {
            log::error("error occurred while trying to automatically load a model file:");
            log::error(ex.what());
            log::error("the file will not be loaded into osc (you won't see the change in the UI)");
        }
    }

    // draw contextual actions (buttons, sliders) for a selected physical frame
    void drawPhysicalFrameContextualActions(AttachGeometryPopup& attachGeomPopup, UndoableUiModel& uim) {
        OpenSim::PhysicalFrame const* selection = uim.getSelectedAs<OpenSim::PhysicalFrame>();

        ImGui::Columns(2);

        ImGui::TextUnformatted("geometry");
        ImGui::SameLine();
        DrawHelpMarker("Geometry that is attached to this physical frame. Multiple pieces of geometry can be attached to the frame");
        ImGui::NextColumn();

        static constexpr char const* modalName = "select geometry to add";

        if (ImGui::Button("add geometry")) {
            ImGui::OpenPopup(modalName);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("Add geometry to this component. Geometry can be removed by selecting it in the hierarchy editor and pressing DELETE");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        if (auto attached = attachGeomPopup.draw(modalName); attached) {
            uim.updSelectedAs<OpenSim::PhysicalFrame>()->attachGeometry(attached.release());
        }
        ImGui::NextColumn();

        ImGui::TextUnformatted("offset frame");
        ImGui::NextColumn();
        if (ImGui::Button("add offset frame")) {
            auto pof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
            pof->setName(selection->getName() + "_offsetframe");
            pof->setParentFrame(*selection);

            auto pofptr = pof.get();
            uim.updSelectedAs<OpenSim::PhysicalFrame>()->addComponent(pof.release());
            uim.setSelected(pofptr);
        }
        if (ImGui::IsItemHovered()) {
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
    void drawJointContextualActions(UndoableUiModel& uim)
    {
        OpenSim::Joint const* selection = uim.getSelectedAs<OpenSim::Joint>();

        if (!selection)
        {
            return;
        }

        ImGui::Columns(2);

        drawSelectionJointTypeSwitcher(uim);

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
                }
                DrawTooltipIfItemHovered("Re-zero the joint", "Given the joint's current geometry due to joint defaults, coordinate defaults, and any coordinate edits made in the coordinate editor, this will reorient the joint's parent (if it's an offset frame) to match the child's transformation. Afterwards, it will then resets all of the joints coordinates to zero. This effectively sets the 'zero point' of the joint (i.e. the geometry when all coordinates are zero) to match whatever the current geometry is.");
                ImGui::NextColumn();
            }
        }

        // BEWARE: broke
        {
            ImGui::Text("add offset frame");
            ImGui::NextColumn();

            if (ImGui::Button("parent")) {
                auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
                pf->setParentFrame(selection->getParentFrame());
                uim.updSelectedAs<OpenSim::Joint>()->addFrame(pf.release());
            }
            ImGui::SameLine();
            if (ImGui::Button("child")) {
                auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
                pf->setParentFrame(selection->getChildFrame());
                uim.updSelectedAs<OpenSim::Joint>()->addFrame(pf.release());
            }
            ImGui::NextColumn();
        }

        ImGui::Columns();
    }

    // draw contextual actions (buttons, sliders) for a selected joint
    void drawHCFContextualActions(UndoableUiModel& uim) {
        OpenSim::HuntCrossleyForce const* hcf = uim.getSelectedAs<OpenSim::HuntCrossleyForce>();

        if (!hcf) {
            return;
        }


        if (hcf->get_contact_parameters().getSize() > 1) {
            ImGui::Text("cannot edit: has more than one HuntCrossleyForce::Parameter");
            return;
        }

        // HACK: if it has no parameters, give it some. The HuntCrossleyForce implementation effectively
        // does this internally anyway to satisfy its own API (e.g. `getStaticFriction` requires that
        // the HuntCrossleyForce has a parameter)
        if (hcf->get_contact_parameters().getSize() == 0) {
            uim.updSelectedAs<OpenSim::HuntCrossleyForce>()->updContactParametersSet().adoptAndAppend(new OpenSim::HuntCrossleyForce::ContactParameters());
        }

        OpenSim::HuntCrossleyForce::ContactParameters const& params = hcf->get_contact_parameters()[0];

        ImGui::Columns(2);
        ImGui::TextUnformatted("add contact geometry");
        ImGui::SameLine();
        DrawHelpMarker("Add OpenSim::ContactGeometry to this OpenSim::HuntCrossleyForce.\n\nCollisions are evaluated for all OpenSim::ContactGeometry attached to the OpenSim::HuntCrossleyForce. E.g. if you want an OpenSim::ContactSphere component to collide with an OpenSim::ContactHalfSpace component during a simulation then you should add both of those components to this force");
        ImGui::NextColumn();

        // allow user to add geom
        {
            if (ImGui::Button("add contact geometry")) {
                ImGui::OpenPopup("select contact geometry");
            }

            OpenSim::ContactGeometry const* added =
                SelectComponentPopup<OpenSim::ContactGeometry>{}.draw("select contact geometry", uim.getModel());

            if (added) {
                uim.updSelectedAs<OpenSim::HuntCrossleyForce>()->updContactParametersSet()[0].updGeometry().appendValue(added->getName());
            }
        }

        ImGui::NextColumn();
        ImGui::Columns();

        // render standard, easy to render, props of the contact params
        {
            auto easyToHandleProps = std::array<int, 6>{
                params.PropertyIndex_geometry,
                params.PropertyIndex_stiffness,
                params.PropertyIndex_dissipation,
                params.PropertyIndex_static_friction,
                params.PropertyIndex_dynamic_friction,
                params.PropertyIndex_viscous_friction,
            };

            ObjectPropertiesEditor st;
            auto maybe_updater = st.draw(params, easyToHandleProps);

            if (maybe_updater) {
                uim.setDirty(true);
                maybe_updater->updater(const_cast<OpenSim::AbstractProperty&>(maybe_updater->prop));
            }
        }


        // render readonly contact geometry list with the option for deletion

    }

    // draw contextual actions (buttons, sliders) for a selected path actuator
    void drawPathActuatorContextualParams(UndoableUiModel& uim) {
        OpenSim::PathActuator const* pa = uim.getSelectedAs<OpenSim::PathActuator>();

        if (!pa) {
            return;
        }

        ImGui::Columns(2);

        char const* modalName = "select physical frame";

        ImGui::TextUnformatted("add path point to end");
        ImGui::NextColumn();

        if (ImGui::Button("add")) {
            ImGui::OpenPopup(modalName);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(
                "Add a new path point, attached to an OpenSim::PhysicalFrame in the model, to the end of the sequence of path points in this OpenSim::PathActuator");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        // handle popup
        {
            OpenSim::PhysicalFrame const* pf = Select1PFPopup{}.draw(modalName, uim.getModel());
            if (pf) {
                int n = pa->getGeometryPath().getPathPointSet().getSize();
                char buf[128];
                std::snprintf(buf, sizeof(buf), "%s-P%i", pa->getName().c_str(), n + 1);
                std::string name{buf};
                SimTK::Vec3 pos{0.0f, 0.0f, 0.0f};

                uim.updSelectedAs<OpenSim::PathActuator>()->addNewPathPoint(name, *pf, pos);
            }
        }

        ImGui::NextColumn();
        ImGui::Columns();
    }

    void drawModelContextualActions(UndoableUiModel& uum) {
        OpenSim::Model const* m = uum.getSelectedAs<OpenSim::Model>();

        if (!m) {
            return;
        }

        ImGui::Columns(2);
        ImGui::Text("show frames");
        ImGui::NextColumn();
        bool showingFrames = m->get_ModelVisualPreferences().get_ModelDisplayHints().get_show_frames();
        if (ImGui::Button(showingFrames ? "hide" : "show")) {
            uum.updSelectedAs<OpenSim::Model>()->upd_ModelVisualPreferences().upd_ModelDisplayHints().set_show_frames(!showingFrames);
        }
        ImGui::NextColumn();
        ImGui::Columns();
    }

    // draw contextual actions for selection
    void drawContextualActions(ModelEditorScreen::Impl& impl, UndoableUiModel& uim) {

        if (!uim.hasSelected()) {
            ImGui::TextUnformatted("cannot draw contextual actions: selection is blank (shouldn't be)");
            return;
        }

        ImGui::Columns(2);
        ImGui::TextUnformatted("isolate in visualizer");
        ImGui::NextColumn();

        if (uim.getSelected() != uim.getIsolated()) {
            if (ImGui::Button("isolate")) {
                uim.setIsolated(uim.getSelected());
            }
        } else {
            if (ImGui::Button("clear isolation")) {
                uim.setIsolated(nullptr);
            }
        }

        if (ImGui::IsItemHovered()) {
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
        if (ImGui::Button("copy")) {
            SetClipboardText(uim.getSelected()->getAbsolutePathString().c_str());
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(
                "Copy the absolute path to this component to your clipboard.\n\n(This is handy if you are separately using absolute component paths to (e.g.) manipulate the model in a script or something)");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        ImGui::NextColumn();

        ImGui::Columns();

        if (uim.selectionIsType<OpenSim::Model>()) {
            drawModelContextualActions(uim);
        } else if (uim.selectionDerivesFrom<OpenSim::PhysicalFrame>()) {
            drawPhysicalFrameContextualActions(impl.ui.attachGeometryPopup, uim);
        } else if (uim.selectionDerivesFrom<OpenSim::Joint>()) {
            drawJointContextualActions(uim);
        } else if (uim.selectionIsType<OpenSim::HuntCrossleyForce>()) {
            drawHCFContextualActions(uim);
        } else if (uim.selectionDerivesFrom<OpenSim::PathActuator>()) {
            drawPathActuatorContextualParams(uim);
        }
    }


    // draw socket editor for current selection
    void drawSocketEditor(ReassignSocketPopup& reassignSocketPopup, UndoableUiModel& uim) {
        OpenSim::Component const* selected = uim.getSelected();

        if (!selected) {
            ImGui::TextUnformatted("cannot draw socket editor: selection is blank (shouldn't be)");
        }

        std::vector<std::string> socknames = const_cast<OpenSim::Component*>(selected)->getSocketNames();

        if (socknames.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, OSC_GREYED_RGBA);
            ImGui::Text("    (OpenSim::%s has no sockets)", selected->getConcreteClassName().c_str());
            ImGui::PopStyleColor();
            return;
        }

        // else: it has sockets with names, list each socket and provide the user
        //       with the ability to reassign the socket's connectee

        ImGui::Columns(2);
        for (std::string const& sn : socknames) {
            ImGui::TextUnformatted(sn.c_str());
            ImGui::NextColumn();

            OpenSim::AbstractSocket const& socket = selected->getSocket(sn);
            std::string sockname = socket.getConnecteePath();
            std::string popupname = std::string{"reassign"} + sockname;

            if (ImGui::Button(sockname.c_str())) {
                ImGui::OpenPopup(popupname.c_str());
            }

            if (ImGui::IsItemHovered()) {
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

            if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && dynamic_cast<OpenSim::Component const*>(&socket.getConnecteeAsObject())) {
                uim.setSelected(dynamic_cast<OpenSim::Component const*>(&socket.getConnecteeAsObject()));
                ImGui::NextColumn();
                break;  // don't continue to traverse the sockets, because the selection changed
            }

            if (OpenSim::Object const* connectee = reassignSocketPopup.draw(popupname.c_str(), uim.getModel(), socket); connectee) {

                ImGui::CloseCurrentPopup();

                OpenSim::Object const& existing = socket.getConnecteeAsObject();
                try {
                    uim.updSelected()->updSocket(sn).connect(*connectee);
                    reassignSocketPopup.search[0] = '\0';
                    reassignSocketPopup.error.clear();
                    ImGui::CloseCurrentPopup();
                } catch (std::exception const& ex) {
                    reassignSocketPopup.error = ex.what();
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
    void drawSelectionBreadcrumbs(UndoableUiModel& uim) {
        OpenSim::Component const* selection = uim.getSelected();

        if (!selection) {
            return;
        }

        auto lst = osc::path_to(*selection);

        if (lst.empty()) {
            return;  // this shouldn't happen, but you never know...
        }

        float indent = 0.0f;

        for (auto it = lst.begin(); it != lst.end() - 1; ++it) {
            ImGui::Dummy(ImVec2{indent, 0.0f});
            ImGui::SameLine();
            if (ImGui::Button((*it)->getName().c_str())) {
                uim.setSelected(*it);
            }
            if (ImGui::IsItemHovered()) {
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

    // draw editor for current selection
    void drawSelectionEditor(ModelEditorScreen::Impl& impl, UndoableUiModel& uim) {
        if (!uim.hasSelected()) {
            ImGui::TextUnformatted("(nothing selected)");
            return;
        }

        ImGui::Dummy(ImVec2(0.0f, 1.0f));
        ImGui::TextUnformatted("hierarchy:");
        ImGui::SameLine();
        DrawHelpMarker("Where the selected component is in the model's component hierarchy");
        ImGui::Separator();
        drawSelectionBreadcrumbs(uim);

        // contextual actions
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        ImGui::TextUnformatted("contextual actions:");
        ImGui::SameLine();
        DrawHelpMarker("Actions that are specific to the type of OpenSim::Component that is currently selected");
        ImGui::Separator();
        drawContextualActions(impl, uim);

        // a contextual action may have changed this
        if (!uim.hasSelected()) {
            return;
        }

        // property editors
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        ImGui::TextUnformatted("properties:");
        ImGui::SameLine();
        DrawHelpMarker("Properties of the selected OpenSim::Component. These are declared in the Component's implementation.");
        ImGui::Separator();

        // top-level property editors
        {
            drawTopLevelMembersEditor(uim);
        }

        // property editors
        {
            auto maybeUpdater = impl.ui.propertiesEditor.draw(*uim.getSelected());
            if (maybeUpdater) {
                uim.setDirty(true);
                maybeUpdater->updater(const_cast<OpenSim::AbstractProperty&>(maybeUpdater->prop));
            }
        }

        // socket editor
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        ImGui::TextUnformatted("sockets:");
        ImGui::SameLine();
        DrawHelpMarker("What components this component is connected to.\n\nIn OpenSim, a Socket formalizes the dependency between a Component and another object (typically another Component) without owning that object. While Components can be composites (of multiple components) they often depend on unrelated objects/components that are defined and owned elsewhere. The object that satisfies the requirements of the Socket we term the 'connectee'. When a Socket is satisfied by a connectee we have a successful 'connection' or is said to be connected.");
        ImGui::Separator();
        drawSocketEditor(impl.ui.reassignSocketPopup, uim);
    }

    // draw the "Actions" tab of the main (top) menu
    void drawMainMenuEditTab(ModelEditorScreen::Impl& impl) {

        UndoableUiModel& uim = impl.st->editedModel;

        if (ImGui::BeginMenu("Edit")) {

            if (ImGui::MenuItem(ICON_FA_UNDO " Undo", "Ctrl+Z", false, uim.canUndo())) {
                actionUndoCurrentlyEditedModel(*impl.st);
            }

            if (ImGui::MenuItem(ICON_FA_REDO " Redo", "Ctrl+Shift+Z", false, uim.canRedo())) {
                actionRedoCurrentlyEditedModel(*impl.st);
            }

            if (ImGui::MenuItem(ICON_FA_EYE_SLASH " Clear Isolation", nullptr, false, uim.getIsolated())) {
                uim.setIsolated(nullptr);
            }

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted("Clear currently isolation setting. This is effectively the opposite of 'Isolate'ing a component.");
                if (!uim.getIsolated()) {
                    ImGui::TextDisabled("\n(disabled because nothing is currently isolated)");
                }
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }


            float scaleFactor = impl.st->editedModel.getFixupScaleFactor();
            if (ImGui::InputFloat("set scale factor", &scaleFactor)) {
                impl.st->editedModel.setFixupScaleFactor(scaleFactor);
                impl.st->editedModel.updUiModel().updateIfDirty();
            }
            if (ImGui::MenuItem("autoscale scale factor")) {
                float sf = impl.st->editedModel.getUiModel().getRecommendedScaleFactor();
                impl.st->editedModel.updUiModel().setFixupScaleFactor(sf);
                impl.st->editedModel.updUiModel().updateIfDirty();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::Text("Try to autoscale the model's scale factor based on the current dimensions of the model");
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            bool showingFrames = impl.st->editedModel.getModel().get_ModelVisualPreferences().get_ModelDisplayHints().get_show_frames();
            if (ImGui::MenuItem(showingFrames ? "hide frames" : "show frames")) {
                impl.st->editedModel.updModel().upd_ModelVisualPreferences().upd_ModelDisplayHints().set_show_frames(!showingFrames);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::Text("Set the model's display properties to display physical frames");
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            bool modelHasBackingFile = hasBackingFile(impl.st->editedModel.getModel());
            if (ImGui::MenuItem(ICON_FA_FOLDER " Open .osim's parent directory", nullptr, false, modelHasBackingFile)) {
                std::filesystem::path p{uim.getModel().getInputFileName()};
                OpenPathInOSDefaultApplication(p.parent_path());
            }

            if (ImGui::MenuItem(ICON_FA_LINK " Open .osim in external editor", nullptr, false, modelHasBackingFile)) {
                OpenPathInOSDefaultApplication(uim.getModel().getInputFileName());
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted("Open the .osim file currently being edited in an external text editor. The editor that's used depends on your operating system's default for opening .osim files.");
                if (!hasBackingFile(uim.getModel())) {
                    ImGui::TextDisabled("\n(disabled because the currently-edited model has no backing file)");
                }
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            ImGui::EndMenu();
        }
    }

    void drawMainMenuSimulateTab(ModelEditorScreen::Impl& impl) {
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem(ICON_FA_PLAY " Simulate", "Ctrl+R")) {
                impl.st->startSimulatingEditedModel();
                App::cur().requestTransition<SimulatorScreen>(impl.st);
                impl.resetPerFrame.subpanelRequestedEarlyExit = true;
            }

            if (ImGui::MenuItem(ICON_FA_EDIT " Edit simulation settings")) {
                impl.resetPerFrame.editSimParamsRequested = true;
            }

            if (ImGui::MenuItem("Disable all wrapping surfaces")) {
                actionDisableAllWrappingSurfaces(*impl.st);
            }

            if (ImGui::MenuItem("Enable all wrapping surfaces")) {
                actionEnableAllWrappingSurfaces(*impl.st);
            }

            ImGui::EndMenu();
        }
    }

    // draws the screen's main menu
    void drawMainMenu(ModelEditorScreen::Impl& impl) {
        if (ImGui::BeginMainMenuBar()) {
            impl.ui.mmFileTab.draw(impl.st);
            drawMainMenuEditTab(impl);
            drawMainMenuSimulateTab(impl);
            impl.ui.mmWindowTab.draw(*impl.st);
            impl.ui.mmAboutTab.draw();


            ImGui::Dummy(ImVec2{2.0f, 0.0f});
            if (ImGui::Button(ICON_FA_LIST_ALT " Switch to simulator (Ctrl+E)")) {
                App::cur().requestTransition<SimulatorScreen>(std::move(impl.st));
                ImGui::EndMainMenuBar();
                impl.resetPerFrame.subpanelRequestedEarlyExit = true;
                return;
            }

            // "switch to simulator" menu button
            ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
            if (ImGui::Button(ICON_FA_PLAY " Simulate (Ctrl+R)")) {
                impl.st->startSimulatingEditedModel();
                App::cur().requestTransition<SimulatorScreen>(std::move(impl.st));
                ImGui::PopStyleColor();
                ImGui::EndMainMenuBar();
                impl.resetPerFrame.subpanelRequestedEarlyExit = true;
                return;
            }
            ImGui::PopStyleColor();

            if (ImGui::Button(ICON_FA_EDIT " Edit simulation settings")) {
                impl.resetPerFrame.editSimParamsRequested = true;
            }

            ImGui::EndMainMenuBar();
        }
    }

    // draw right-click context menu for the 3D viewer
    void draw3DViewerContextMenu(
            ModelEditorScreen::Impl& impl,
            OpenSim::Component const& selected) {

        ImGui::TextDisabled("%s (%s)", selected.getName().c_str(), selected.getConcreteClassName().c_str());
        ImGui::Separator();
        ImGui::Dummy(ImVec2{0.0f, 3.0f});

        if (ImGui::BeginMenu("Select Owner")) {
            OpenSim::Component const* c = &selected;
            impl.st->setHovered(nullptr);
            while (c->hasOwner()) {
                c = &c->getOwner();

                char buf[128];
                std::snprintf(buf, sizeof(buf), "%s (%s)", c->getName().c_str(), c->getConcreteClassName().c_str());

                if (ImGui::MenuItem(buf)) {
                    impl.st->setSelected(c);
                }
                if (ImGui::IsItemHovered()) {
                    impl.st->setHovered(c);
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Request Outputs")) {
            DrawHelpMarker("Request that these outputs are plotted whenever a simulation is ran. The outputs will appear in the 'outputs' tab on the simulator screen");

            OpenSim::Component const* c = &selected;
            int imguiId = 0;
            while (c) {
                ImGui::PushID(imguiId++);
                ImGui::Dummy(ImVec2{0.0f, 2.0f});
                ImGui::TextDisabled("%s (%s)", c->getName().c_str(), c->getConcreteClassName().c_str());
                ImGui::Separator();
                for (auto const& o : c->getOutputs()) {
                    char buf[256];
                    std::snprintf(buf, sizeof(buf), "  %s", o.second->getName().c_str());

                    auto const& suboutputs = getOutputSubfields(*o.second);
                    if (suboutputs.empty()) {
                        // can only plot top-level of output

                        if (ImGui::MenuItem(buf)) {
                           impl.st->desiredOutputs.emplace_back(*c, *o.second);
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();
                            ImGui::Text("Output Type = %s", o.second->getTypeName().c_str());
                            ImGui::EndTooltip();
                        }
                    } else {
                        // can plot suboutputs
                        if (ImGui::BeginMenu(buf)) {
                            for (PlottableOutputSubfield const& pos : suboutputs) {
                                if (ImGui::MenuItem(pos.name)) {
                                    impl.st->desiredOutputs.emplace_back(*c, *o.second, pos);
                                }
                            }
                            ImGui::EndMenu();
                        }

                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();
                            ImGui::Text("Output Type = %s", o.second->getTypeName().c_str());
                            ImGui::EndTooltip();
                        }
                    }
                }
                if (c->getNumOutputs() == 0) {
                    ImGui::TextDisabled("  (has no outputs)");
                }
                ImGui::PopID();
                c = c->hasOwner() ? &c->getOwner() : nullptr;
            }
            ImGui::EndMenu();
        }
    }

    // draw a single 3D model viewer
    bool draw3DViewer(
            ModelEditorScreen::Impl& impl,
            UiModelViewer& viewer,
            char const* name) {

        bool isOpen = true;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        bool shown = ImGui::Begin(name, &isOpen, ImGuiWindowFlags_MenuBar);
        ImGui::PopStyleVar();

        if (!isOpen) {
            ImGui::End();
            return false;
        }

        if (!shown) {
            ImGui::End();
            return true;
        }

        auto resp = viewer.draw(impl.st->editedModel.getUiModel());
        ImGui::End();

        // update hover
        if (resp.isMousedOver && resp.hovertestResult != impl.st->getHovered()) {
            impl.st->setHovered(resp.hovertestResult);
        }

        // if left-clicked, update selection
        if (resp.isMousedOver && resp.isLeftClicked) {
            impl.st->setSelected(resp.hovertestResult);
        }

        // if hovered, draw hover tooltip
        if (resp.isMousedOver && resp.hovertestResult) {
            drawComponentHoverTooltip(*resp.hovertestResult, resp.mouse3DLocation);
        }

        // if right-clicked, draw context menu
        {
            char buf[128];
            std::snprintf(buf, sizeof(buf), "%s_contextmenu", name);
            if (resp.isMousedOver && resp.hovertestResult && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                impl.st->setSelected(resp.hovertestResult);
                ImGui::OpenPopup(buf);
            }
            if (impl.st->editedModel.hasSelected() && ImGui::BeginPopup(buf)) {
                draw3DViewerContextMenu(impl, *impl.st->getSelected());
                ImGui::EndPopup();
            }
        }

        return true;
    }

    // draw all user-enabled 3D model viewers
    void draw3DViewers(ModelEditorScreen::Impl& impl) {
        for (size_t i = 0; i < impl.st->viewers.size(); ++i) {
            auto& maybe3DViewer = impl.st->viewers[i];

            if (!maybe3DViewer) {
                continue;
            }

            UiModelViewer& viewer = *maybe3DViewer;

            char buf[64];
            std::snprintf(buf, sizeof(buf), "viewer%zu", i);

            bool isOpen = draw3DViewer(impl, viewer, buf);

            if (!isOpen) {
                maybe3DViewer.reset();
            }
        }
    }

    // draw model editor screen
    //
    // can throw if the model is in an invalid state
    void modelEditorDrawUnguarded(ModelEditorScreen::Impl& impl) {
        if (impl.resetPerFrame.shouldRequestRedraw) {
            App::cur().requestRedraw();
        }

        impl.resetPerFrame = {};

        // draw main menu
        {
            drawMainMenu(impl);
        }

        // check for early exit request
        //
        // (the main menu may have requested a screen transition)
        if (impl.resetPerFrame.subpanelRequestedEarlyExit) {
            return;
        }

        // draw 3D viewers (if any)
        {
            draw3DViewers(impl);
        }

        // apply any updates made during this frame (can throw)
        impl.st->editedModel.updateIfDirty();

        // draw editor actions panel
        //
        // contains top-level actions (e.g. "add body")
        if (impl.st->showing.actions) {
            if (ImGui::Begin("Actions", nullptr, ImGuiWindowFlags_MenuBar)) {
                impl.ui.modelActions.draw(impl.st->editedModel.updUiModel());
            }
            ImGui::End();
        }

        // apply any updates made during this frame (can throw)
        impl.st->editedModel.updateIfDirty();

        // draw hierarchy viewer
        if (impl.st->showing.hierarchy) {
            if (ImGui::Begin("Hierarchy", &impl.st->showing.hierarchy)) {
                auto resp = impl.ui.componentHierarchy.draw(
                    &impl.st->editedModel.getModel().getRoot(),
                    impl.st->getSelected(),
                    impl.st->getHovered());

                if (resp.type == ComponentHierarchy::SelectionChanged) {
                    impl.st->setSelected(resp.ptr);
                } else if (resp.type == ComponentHierarchy::HoverChanged) {
                    impl.st->setHovered(resp.ptr);
                }
            }
            ImGui::End();
        }

        // apply any updates made during this frame (can throw)
        impl.st->editedModel.updateIfDirty();

        // draw property editor
        if (impl.st->showing.propertyEditor) {
            if (ImGui::Begin("Edit Props", &impl.st->showing.propertyEditor)) {
                drawSelectionEditor(impl, impl.st->editedModel);
            }
            ImGui::End();
        }

        // apply any updates made during this frame (can throw)
        impl.st->editedModel.updateIfDirty();

        // draw application log
        if (impl.st->showing.log) {
            if (ImGui::Begin("Log", &impl.st->showing.log, ImGuiWindowFlags_MenuBar)) {
                impl.ui.logViewer.draw();
            }
            ImGui::End();
        }

        // apply any updates made during this frame (can throw)
        impl.st->editedModel.updateIfDirty();

        if (impl.st->showing.coordinateEditor) {
            if (ImGui::Begin("Coordinate Editor")) {
                impl.ui.coordEditor.draw(impl.st->editedModel.updUiModel());
            }
        }

        // apply any updates made during this frame (can throw)
        impl.st->editedModel.updateIfDirty();

        // draw sim params editor popup (if applicable)
        {
            if (impl.resetPerFrame.editSimParamsRequested) {
                ImGui::OpenPopup("simulation parameters");
            }

            FdParamsEditorPopup{}.draw("simulation parameters", impl.st->simParams);
        }

        if (impl.resetPerFrame.subpanelRequestedEarlyExit) {
            return;
        }

        // apply any updates made during this frame (can throw)
        impl.st->editedModel.updateIfDirty();
    }
}

static std::string GetDocumentName(UndoableUiModel const& uim)
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

static std::string GetRecommendedTitle(UndoableUiModel const& uim)
{
    std::string s = GetDocumentName(uim);
    if (!uim.isUpToDateWithFilesystem())
    {
        s += '*';
    }
    return s;
}


// public API

osc::ModelEditorScreen::ModelEditorScreen(std::shared_ptr<MainEditorState> st) :
    m_Impl{new Impl {std::move(st)}} {
}

osc::ModelEditorScreen::~ModelEditorScreen() noexcept = default;

void osc::ModelEditorScreen::onMount()
{
    App::cur().makeMainEventLoopWaiting();
    App::cur().setMainWindowSubTitle(GetRecommendedTitle(m_Impl->st->editedModel));
    osc::ImGuiInit();
}

void osc::ModelEditorScreen::onUnmount()
{
    osc::ImGuiShutdown();
    App::cur().unsetMainWindowSubTitle();
    App::cur().makeMainEventLoopPolling();
}

void ModelEditorScreen::onEvent(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        m_Impl->resetPerFrame.shouldRequestRedraw = true;
        return;
    }

    if (e.type == SDL_KEYDOWN) {
        modelEditorOnKeydown(*m_Impl, e.key);
        return;
    }
}

void osc::ModelEditorScreen::tick(float) {
    if (m_Impl->filePoller.changeWasDetected(m_Impl->st->getModel().getInputFileName()))
    {
        modelEditorOnBackingFileChanged(*m_Impl);
    }

    App::cur().setMainWindowSubTitle(GetRecommendedTitle(m_Impl->st->editedModel));
}

void osc::ModelEditorScreen::draw() {
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    osc::ImGuiNewFrame();
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    try
    {
        ::modelEditorDrawUnguarded(*m_Impl);
    }
    catch (std::exception const& ex)
    {
        log::error("an OpenSim::Exception was thrown while drawing the editor");
        log::error("    message = %s", ex.what());
        log::error("OpenSim::Exceptions typically happen when the model is damaged or made invalid by an edit (e.g. setting a property to an invalid value)");

        try
        {
            m_Impl->st->editedModel.rollback();
            log::error("model rollback succeeded");
        }
        catch (std::exception const& ex2)
        {
            App::cur().requestTransition<ErrorScreen>(ex2);
        }

        // try to put ImGui into a clean state
        osc::ImGuiShutdown();
        osc::ImGuiInit();
        osc::ImGuiNewFrame();
    }

    osc::ImGuiRender();
}
