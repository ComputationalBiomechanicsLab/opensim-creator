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
#include "src/UI/HelpMarker.hpp"
#include "src/UI/MainMenu.hpp"
#include "src/UI/ModelActionsMenuBar.hpp"
#include "src/UI/LogViewer.hpp"
#include "src/UI/PropertyEditors.hpp"
#include "src/UI/ReassignSocketPopup.hpp"
#include "src/UI/SelectComponentPopup.hpp"
#include "src/UI/Select1PFPopup.hpp"
#include "src/UI/Select2PFsPopup.hpp"
#include "src/Utils/ScopeGuard.hpp"
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
    void deleteItemFromSet(OpenSim::Set<T, TSetBase>& set, T* item) {
        for (int i = 0; i < set.getSize(); ++i) {
            if (&set.get(i) == item) {
                set.remove(i);
                return;
            }
        }
    }

    // draw component information as a hover tooltip
    void drawComponentHoverTooltip(OpenSim::Component const& hovered) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() + 400.0f);

        ImGui::TextUnformatted(hovered.getName().c_str());
        ImGui::SameLine();
        ImGui::TextDisabled(" (%s)", hovered.getConcreteClassName().c_str());
        ImGui::Dummy(ImVec2{0.0f, 5.0f});
        ImGui::TextDisabled("(right-click for actions)");

        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

    // try to delete an undoable-model's current selection
    //
    // "try", because some things are difficult to delete from OpenSim models
    void actionTryDeleteSelectionFromEditedModel(UndoableUiModel& uim) {
        OpenSim::Component* selected = uim.getSelection();

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

            uim.beforeModifyingModel();
            deleteItemFromSet(*js, static_cast<OpenSim::Joint*>(selected));
            uim.declareDeathOf(selected);
            uim.afterModifyingModel();
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

            uim.beforeModifyingModel();
            deleteItemFromSet(*cs, static_cast<OpenSim::Controller*>(selected));
            uim.declareDeathOf(selected);
            uim.afterModifyingModel();
        } else if (auto* conss = dynamic_cast<OpenSim::ConstraintSet*>(owner); cs) {
            // delete an OpenSim::Constraint from its owning OpenSim::ConstraintSet

            uim.beforeModifyingModel();
            deleteItemFromSet(*conss, static_cast<OpenSim::Constraint*>(selected));
            uim.declareDeathOf(selected);
            uim.afterModifyingModel();
        } else if (auto* fs = dynamic_cast<OpenSim::ForceSet*>(owner); fs) {
            // delete an OpenSim::Force from its owning OpenSim::ForceSet

            uim.beforeModifyingModel();
            deleteItemFromSet(*fs, static_cast<OpenSim::Force*>(selected));
            uim.declareDeathOf(selected);
            uim.afterModifyingModel();
        } else if (auto* ms = dynamic_cast<OpenSim::MarkerSet*>(owner); ms) {
            // delete an OpenSim::Marker from its owning OpenSim::MarkerSet

            uim.beforeModifyingModel();
            deleteItemFromSet(*ms, static_cast<OpenSim::Marker*>(selected));
            uim.declareDeathOf(selected);
            uim.afterModifyingModel();
        } else if (auto* cgs = dynamic_cast<OpenSim::ContactGeometrySet*>(owner); cgs) {
            // delete an OpenSim::ContactGeometry from its owning OpenSim::ContactGeometrySet

            uim.beforeModifyingModel();
            deleteItemFromSet(*cgs, static_cast<OpenSim::ContactGeometry*>(selected));
            uim.declareDeathOf(selected);
            uim.afterModifyingModel();
        } else if (auto* ps = dynamic_cast<OpenSim::ProbeSet*>(owner); ps) {
            // delete an OpenSim::Probe from its owning OpenSim::ProbeSet

            uim.beforeModifyingModel();
            deleteItemFromSet(*ps, static_cast<OpenSim::Probe*>(selected));
            uim.declareDeathOf(selected);
            uim.afterModifyingModel();
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

                uim.beforeModifyingModel();
                prop.assign(*copy);
                uim.declareDeathOf(selected);
                uim.afterModifyingModel();
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
                    uim.beforeModifyingModel();
                    gp->deletePathPoint(uim.state(), idx);
                    uim.declareDeathOf(selected);
                    uim.afterModifyingModel();
                }
            }
        }
    }

    // draw an editor for top-level selected Component members (e.g. name)
    void drawTopLevelMembersEditor(UndoableUiModel& st) {
        if (!st.getSelection()) {
            ImGui::Text("cannot draw top level editor: nothing selected?");
            return;
        }
        OpenSim::Component& selection = *st.getSelection();

        ImGui::Columns(2);

        ImGui::TextUnformatted("name");
        ImGui::NextColumn();

        char nambuf[128];
        nambuf[sizeof(nambuf) - 1] = '\0';
        std::strncpy(nambuf, selection.getName().c_str(), sizeof(nambuf) - 1);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
        if (ImGui::InputText("##nameditor", nambuf, sizeof(nambuf), ImGuiInputTextFlags_EnterReturnsTrue)) {

            if (std::strlen(nambuf) > 0) {
                st.beforeModifyingModel();
                selection.setName(nambuf);
                st.afterModifyingModel();
            }
        }
        ImGui::NextColumn();

        ImGui::Columns();
    }

    // draw UI element that lets user change a model joint's type
    void drawJointTypeSwitcher(UndoableUiModel& st, OpenSim::Joint& selection) {
        auto const* parentJointset =
            selection.hasOwner() ? dynamic_cast<OpenSim::JointSet const*>(&selection.getOwner()) : nullptr;

        if (!parentJointset) {
            // it's a joint, but it's not owned by a JointSet, so the implementation cannot switch
            // the joint type
            return;
        }

        OpenSim::JointSet const& js = *parentJointset;

        int idx = -1;
        for (int i = 0; i < js.getSize(); ++i) {
            OpenSim::Joint const* j = &js[i];
            if (j == &selection) {
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
        std::optional<size_t> maybeTypeIndex = JointRegistry::indexOf(selection);
        int typeIndex = maybeTypeIndex ? static_cast<int>(*maybeTypeIndex) : -1;

        auto jointNames = JointRegistry::names();

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
        if (ImGui::Combo(
                "##newjointtypeselector",
                &typeIndex,
                jointNames.data(),
                static_cast<int>(jointNames.size())) &&
            typeIndex >= 0) {

            // copy + fixup  a prototype of the user's selection
            std::unique_ptr<OpenSim::Joint> newJoint{JointRegistry::prototypes()[static_cast<size_t>(typeIndex)]->clone()};
            copyCommonJointProperties(selection, *newJoint);

            // overwrite old joint in model
            //
            // note: this will invalidate the `selection` joint, because the
            // OpenSim::JointSet container will automatically kill it
            st.beforeModifyingModel();
            OpenSim::Joint* ptr = newJoint.get();
            const_cast<OpenSim::JointSet&>(js).set(idx, newJoint.release());
            st.declareDeathOf(&selection);
            st.setSelection(ptr);
            st.afterModifyingModel();
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

        OpenSim::Model& m = uim.model();

        uim.beforeModifyingModel();
        for (OpenSim::WrapObjectSet& wos : m.updComponentList<OpenSim::WrapObjectSet>()) {
            for (int i = 0; i < wos.getSize(); ++i) {
                OpenSim::WrapObject& wo = wos[i];
                wo.set_active(false);
                wo.upd_Appearance().set_visible(false);
            }
        }
        uim.afterModifyingModel();
    }

    // enable all wrapping surfaces in the current model
    void actionEnableAllWrappingSurfaces(MainEditorState& mes) {
        UndoableUiModel& uim = mes.editedModel;

        OpenSim::Model& m = uim.model();

        uim.beforeModifyingModel();
        for (OpenSim::WrapObjectSet& wos : m.updComponentList<OpenSim::WrapObjectSet>()) {
            for (int i = 0; i < wos.getSize(); ++i) {
                OpenSim::WrapObject& wo = wos[i];
                wo.set_active(true);
                wo.upd_Appearance().set_visible(true);
            }
        }
        uim.afterModifyingModel();
    }

    // try to start a new simulation from the currently-edited model
    void actionStartSimulationFromEditedModel(MainEditorState& mes) {
        mes.startSimulatingEditedModel();
    }

    void actionClearSelectionFromEditedModel(MainEditorState& mes) {
        mes.editedModel.setSelection(nullptr);
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
        filePoller{std::chrono::milliseconds{1000}, st->editedModel.model().getInputFileName()} {
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
            auto p = std::make_unique<OpenSim::Model>(impl.st->model().getInputFileName());
            log::info("loaded updated file");
            impl.st->setModel(std::move(p));
        } catch (std::exception const& ex) {
            log::error("error occurred while trying to automatically load a model file:");
            log::error(ex.what());
            log::error("the file will not be loaded into osc (you won't see the change in the UI)");
        }
    }

    // draw contextual actions (buttons, sliders) for a selected physical frame
    void drawPhysicalFrameContextualActions(
            ModelEditorScreen::Impl& impl,
            UndoableUiModel& uim,
            OpenSim::PhysicalFrame& selection) {

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

        if (auto attached = impl.ui.attachGeometryPopup.draw(modalName); attached) {
            uim.beforeModifyingModel();
            selection.attachGeometry(attached.release());
            uim.afterModifyingModel();
        }
        ImGui::NextColumn();

        ImGui::TextUnformatted("offset frame");
        ImGui::NextColumn();
        if (ImGui::Button("add offset frame")) {
            auto pof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
            pof->setName(selection.getName() + "_offsetframe");
            pof->setParentFrame(selection);

            uim.beforeModifyingModel();
            auto pofptr = pof.get();
            selection.addComponent(pof.release());
            uim.setSelection(pofptr);
            uim.afterModifyingModel();
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
    void drawJointContextualActions(
            UndoableUiModel& st,
            OpenSim::Joint& selection) {

        ImGui::Columns(2);

        drawJointTypeSwitcher(st, selection);

        // BEWARE: broke
        {
            ImGui::Text("add offset frame");
            ImGui::NextColumn();

            if (ImGui::Button("parent")) {
                auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
                pf->setParentFrame(selection.getParentFrame());

                st.beforeModifyingModel();
                selection.addFrame(pf.release());
                st.afterModifyingModel();
            }
            ImGui::SameLine();
            if (ImGui::Button("child")) {
                auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
                pf->setParentFrame(selection.getChildFrame());

                st.beforeModifyingModel();
                selection.addFrame(pf.release());
                st.afterModifyingModel();
            }
            ImGui::NextColumn();
        }

        ImGui::Columns();
    }

    // draw contextual actions (buttons, sliders) for a selected joint
    void drawHCFContextualActions(
        UndoableUiModel& uim,
        OpenSim::HuntCrossleyForce& selection) {

        if (selection.get_contact_parameters().getSize() > 1) {
            ImGui::Text("cannot edit: has more than one HuntCrossleyForce::Parameter");
            return;
        }

        // HACK: if it has no parameters, give it some. The HuntCrossleyForce implementation effectively
        // does this internally anyway to satisfy its own API (e.g. `getStaticFriction` requires that
        // the HuntCrossleyForce has a parameter)
        if (selection.get_contact_parameters().getSize() == 0) {
            selection.updContactParametersSet().adoptAndAppend(new OpenSim::HuntCrossleyForce::ContactParameters());
        }

        OpenSim::HuntCrossleyForce::ContactParameters& params = selection.upd_contact_parameters()[0];

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
                SelectComponentPopup<OpenSim::ContactGeometry>{}.draw("select contact geometry", uim.model());

            if (added) {
                uim.beforeModifyingModel();
                params.updGeometry().appendValue(added->getName());
                uim.afterModifyingModel();
            }
        }

        ImGui::NextColumn();
        ImGui::Columns();

        // render standard, easy to render, props of the contact params
        {
            std::array<int, 6> easyToHandleProps = {
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
                uim.beforeModifyingModel();
                maybe_updater->updater(const_cast<OpenSim::AbstractProperty&>(maybe_updater->prop));
                uim.afterModifyingModel();
            }
        }
    }

    // draw contextual actions (buttons, sliders) for a selected path actuator
    void drawPathActuatorContextualParams(
            UndoableUiModel& uim,
            OpenSim::PathActuator& selection) {

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
            OpenSim::PhysicalFrame const* pf = Select1PFPopup{}.draw(modalName, uim.model());
            if (pf) {
                int n = selection.getGeometryPath().getPathPointSet().getSize();
                char buf[128];
                std::snprintf(buf, sizeof(buf), "%s-P%i", selection.getName().c_str(), n + 1);
                std::string name{buf};
                SimTK::Vec3 pos{0.0f, 0.0f, 0.0f};

                uim.beforeModifyingModel();
                selection.addNewPathPoint(name, *pf, pos);
                uim.afterModifyingModel();
            }
        }

        ImGui::NextColumn();
        ImGui::Columns();
    }

    void drawModelContextualActions(ModelEditorScreen::Impl& impl, OpenSim::Model& selection) {
        ImGui::Columns(2);
        ImGui::Text("show frames");
        ImGui::NextColumn();
        bool showingFrames = selection.get_ModelVisualPreferences().get_ModelDisplayHints().get_show_frames();
        if (ImGui::Button(showingFrames ? "hide" : "show")) {
            impl.st->editedModel.beforeModifyingModel();
            selection.upd_ModelVisualPreferences().upd_ModelDisplayHints().set_show_frames(!showingFrames);
            impl.st->editedModel.afterModifyingModel();
        }
        ImGui::NextColumn();
        ImGui::Columns();
    }

    // draw contextual actions for selection
    void drawContextualActions(ModelEditorScreen::Impl& impl, UndoableUiModel& uim) {

        if (!uim.getSelection()) {
            ImGui::TextUnformatted("cannot draw contextual actions: selection is blank (shouldn't be)");
            return;
        }

        ImGui::Columns(2);
        ImGui::TextUnformatted("isolate in visualizer");
        ImGui::NextColumn();

        if (uim.getSelection() != uim.getIsolated()) {
            if (ImGui::Button("isolate")) {
                uim.beforeModifyingModel();
                uim.setIsolated(uim.getSelection());
                uim.afterModifyingModel();
            }
        } else {
            if (ImGui::Button("clear isolation")) {
                uim.beforeModifyingModel();
                uim.setIsolated(nullptr);
                uim.afterModifyingModel();
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
            SetClipboardText(uim.getSelection()->getAbsolutePathString().c_str());
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

        if (auto* model = dynamic_cast<OpenSim::Model*>(uim.getSelection()); model) {
            drawModelContextualActions(impl, *model);
        } else if (auto* frame = dynamic_cast<OpenSim::PhysicalFrame*>(uim.getSelection()); frame) {
            drawPhysicalFrameContextualActions(impl, uim, *frame);
        } else if (auto* joint = dynamic_cast<OpenSim::Joint*>(uim.getSelection()); joint) {
            drawJointContextualActions(uim, *joint);
        } else if (auto* hcf = dynamic_cast<OpenSim::HuntCrossleyForce*>(uim.getSelection()); hcf) {
            drawHCFContextualActions(uim, *hcf);
        } else if (auto* pa = dynamic_cast<OpenSim::PathActuator*>(uim.getSelection()); pa) {
            drawPathActuatorContextualParams(uim, *pa);
        }
    }


    // draw socket editor for current selection
    void drawSocketEditor(ModelEditorScreen::Impl& impl, UndoableUiModel& uim) {

        if (!uim.getSelection()) {
            ImGui::TextUnformatted("cannot draw socket editor: selection is blank (shouldn't be)");
            return;
        }

        std::vector<std::string> socknames = uim.getSelection()->getSocketNames();

        if (socknames.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, OSC_GREYED_RGBA);
            ImGui::Text("    (OpenSim::%s has no sockets)", uim.getSelection()->getConcreteClassName().c_str());
            ImGui::PopStyleColor();
            return;
        }

        // else: it has sockets with names, list each socket and provide the user
        //       with the ability to reassign the socket's connectee

        ImGui::Columns(2);
        for (std::string const& sn : socknames) {
            ImGui::TextUnformatted(sn.c_str());
            ImGui::NextColumn();

            OpenSim::AbstractSocket const& socket = uim.getSelection()->getSocket(sn);
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
                uim.setSelection(const_cast<OpenSim::Component*>(dynamic_cast<OpenSim::Component const*>(&socket.getConnecteeAsObject())));
                ImGui::NextColumn();
                break;  // don't continue to traverse the sockets, because the selection changed
            }

            if (OpenSim::Object const* connectee =
                    impl.ui.reassignSocketPopup.draw(popupname.c_str(), uim.model(), socket);
                connectee) {

                ImGui::CloseCurrentPopup();

                OpenSim::Object const& existing = socket.getConnecteeAsObject();
                uim.beforeModifyingModel();
                try {
                    uim.getSelection()->updSocket(sn).connect(*connectee);
                    impl.ui.reassignSocketPopup.search[0] = '\0';
                    impl.ui.reassignSocketPopup.error.clear();
                    ImGui::CloseCurrentPopup();
                } catch (std::exception const& ex) {
                    impl.ui.reassignSocketPopup.error = ex.what();
                    uim.getSelection()->updSocket(sn).connect(existing);
                }
                uim.afterModifyingModel();
            }

            ImGui::NextColumn();
        }
        ImGui::Columns();
    }

    // draw breadcrumbs for current selection
    //
    // eg: Model > Joint > PhysicalFrame
    void drawSelectionBreadcrumbs(UndoableUiModel& uim) {
        if (!uim.getSelection()) {
            return;  // nothing selected
        }

        auto lst = osc::path_to(*uim.getSelection());

        if (lst.empty()) {
            return;  // this shouldn't happen, but you never know...
        }

        float indent = 0.0f;

        for (auto it = lst.begin(); it != lst.end() - 1; ++it) {
            ImGui::Dummy(ImVec2{indent, 0.0f});
            ImGui::SameLine();
            if (ImGui::Button((*it)->getName().c_str())) {
                uim.setSelection(const_cast<OpenSim::Component*>(*it));
            }
            if (ImGui::IsItemHovered()) {
                uim.setHover(const_cast<OpenSim::Component*>(*it));
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
        if (!uim.getSelection()) {
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
        if (!uim.getSelection()) {
            return;
        }

        // property editor
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        ImGui::TextUnformatted("properties:");
        ImGui::SameLine();
        DrawHelpMarker("Properties of the selected OpenSim::Component. These are declared in the Component's implementation.");
        ImGui::Separator();
        drawTopLevelMembersEditor(uim);
        {
            auto maybeUpdater = impl.ui.propertiesEditor.draw(*uim.getSelection());
            if (maybeUpdater) {
                uim.beforeModifyingModel();
                maybeUpdater->updater(const_cast<OpenSim::AbstractProperty&>(maybeUpdater->prop));
                uim.afterModifyingModel();
            }
        }

        // socket editor
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        ImGui::TextUnformatted("sockets:");
        ImGui::SameLine();
        DrawHelpMarker("What components this component is connected to.\n\nIn OpenSim, a Socket formalizes the dependency between a Component and another object (typically another Component) without owning that object. While Components can be composites (of multiple components) they often depend on unrelated objects/components that are defined and owned elsewhere. The object that satisfies the requirements of the Socket we term the 'connectee'. When a Socket is satisfied by a connectee we have a successful 'connection' or is said to be connected.");
        ImGui::Separator();
        drawSocketEditor(impl, uim);
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


            float scaleFactor = impl.st->editedModel.current.fixupScaleFactor;
            if (ImGui::InputFloat("set scale factor", &scaleFactor)) {
                impl.st->editedModel.current.fixupScaleFactor = scaleFactor;
                impl.st->editedModel.current.onUiModelModified();
            }
            if (ImGui::MenuItem("autoscale scale factor")) {
                UiModel& uim2 = impl.st->editedModel.current;
                float sf = uim2.getRecommendedScaleFactor();
                uim2.setSceneScaleFactor(sf);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::Text("Try to autoscale the model's scale factor based on the current dimensions of the model");
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            bool showingFrames = impl.st->editedModel.model().get_ModelVisualPreferences().get_ModelDisplayHints().get_show_frames();
            if (ImGui::MenuItem(showingFrames ? "hide frames" : "show frames")) {
                impl.st->editedModel.beforeModifyingModel();
                impl.st->editedModel.model().upd_ModelVisualPreferences().upd_ModelDisplayHints().set_show_frames(!showingFrames);
                impl.st->editedModel.afterModifyingModel();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::Text("Set the model's display properties to display physical frames");
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            bool modelHasBackingFile = hasBackingFile(impl.st->editedModel.model());
            if (ImGui::MenuItem(ICON_FA_FOLDER " Open .osim's parent directory", nullptr, false, modelHasBackingFile)) {
                std::filesystem::path p{uim.model().getInputFileName()};
                OpenPathInOSDefaultApplication(p.parent_path());
            }

            if (ImGui::MenuItem(ICON_FA_LINK " Open .osim in external editor", nullptr, false, modelHasBackingFile)) {
                OpenPathInOSDefaultApplication(uim.model().getInputFileName());
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted("Open the .osim file currently being edited in an external text editor. The editor that's used depends on your operating system's default for opening .osim files.");
                if (!hasBackingFile(uim.model())) {
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
                    impl.st->setSelection(const_cast<OpenSim::Component*>(c));
                }
                if (ImGui::IsItemHovered()) {
                    impl.st->setHovered(const_cast<OpenSim::Component*>(c));
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

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});

        bool isOpen = true;
        bool shown = ImGui::Begin(name, &isOpen, ImGuiWindowFlags_MenuBar);

        if (!isOpen) {
            ImGui::End();
            return false;
        }

        if (!shown) {
            ImGui::End();
            return true;
        }

        auto resp = viewer.draw(impl.st->editedModel.current);
        ImGui::PopStyleVar();
        ImGui::End();

        // update hover
        if (resp.isMousedOver && resp.hovertestResult != impl.st->hovered()) {
            impl.st->setHovered(const_cast<OpenSim::Component*>(resp.hovertestResult));
        }

        // if left-clicked, update selection
        if (resp.isMousedOver && resp.isLeftClicked) {
            impl.st->setSelection(const_cast<OpenSim::Component*>(resp.hovertestResult));
        }

        // if hovered, draw hover tooltip
        if (resp.isMousedOver && resp.hovertestResult) {
            drawComponentHoverTooltip(*resp.hovertestResult);
        }

        // if right-clicked, draw context menu
        {
            char buf[128];
            std::snprintf(buf, sizeof(buf), "%s_contextmenu", name);
            if (resp.isMousedOver && resp.hovertestResult && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                impl.st->setSelection(const_cast<OpenSim::Component*>(resp.hovertestResult));
                ImGui::OpenPopup(buf);
            }
            if (impl.st->selection() && ImGui::BeginPopup(buf)) {
                draw3DViewerContextMenu(impl, *impl.st->selection());
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

        // draw editor actions panel
        //
        // contains top-level actions (e.g. "add body")
        if (impl.st->showing.actions) {
            if (ImGui::Begin("Actions", nullptr, ImGuiWindowFlags_MenuBar)) {
                auto onSetSelection = [&](OpenSim::Component* c) { impl.st->editedModel.setSelection(c); };
                auto onBeforeModifyModel = [&]() { impl.st->editedModel.beforeModifyingModel(); };
                auto onAfterModifyModel = [&]() { impl.st->editedModel.afterModifyingModel(); };
                impl.ui.modelActions.draw(impl.st->editedModel.model(), onSetSelection, onBeforeModifyModel, onAfterModifyModel);
            }
            ImGui::End();
        }

        // draw hierarchy viewer
        if (impl.st->showing.hierarchy) {
            if (ImGui::Begin("Hierarchy", &impl.st->showing.hierarchy)) {
                auto resp = impl.ui.componentHierarchy.draw(
                    &impl.st->editedModel.model().getRoot(),
                    impl.st->editedModel.getSelection(),
                    impl.st->editedModel.getHover());

                if (resp.type == ComponentHierarchy::SelectionChanged) {
                    impl.st->setSelection(const_cast<OpenSim::Component*>(resp.ptr));
                } else if (resp.type == ComponentHierarchy::HoverChanged) {
                    impl.st->setHovered(const_cast<OpenSim::Component*>(resp.ptr));
                }
            }
            ImGui::End();
        }

        // draw selection details
        if (impl.st->showing.selectionDetails) {
            if (ImGui::Begin("Selection", &impl.st->showing.selectionDetails)) {
                auto resp = ComponentDetails{}.draw(impl.st->editedModel.state(), impl.st->editedModel.getSelection());

                if (resp.type == ComponentDetails::SelectionChanged) {
                    impl.st->editedModel.setSelection(const_cast<OpenSim::Component*>(resp.ptr));
                }
            }
            ImGui::End();
        }

        // draw property editor
        if (impl.st->showing.propertyEditor) {
            if (ImGui::Begin("Edit Props", &impl.st->showing.propertyEditor)) {
                drawSelectionEditor(impl, impl.st->editedModel);
            }
            ImGui::End();
        }

        // draw application log
        if (impl.st->showing.log) {
            if (ImGui::Begin("Log", &impl.st->showing.log, ImGuiWindowFlags_MenuBar)) {
                impl.ui.logViewer.draw();
            }
            ImGui::End();
        }


        if (impl.st->showing.coordinateEditor) {
            if (ImGui::Begin("Coordinate Editor")) {
                impl.ui.coordEditor.draw(impl.st->editedModel.current);
            }
        }

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

        // garbage-collect any models damaged by in-UI modifications (if applicable)
        impl.st->clearAnyDamagedModels();
    }
}


// public API

osc::ModelEditorScreen::ModelEditorScreen(std::shared_ptr<MainEditorState> st) :
    m_Impl{new Impl {std::move(st)}} {
}

osc::ModelEditorScreen::ModelEditorScreen(ModelEditorScreen&&) noexcept = default;
osc::ModelEditorScreen& osc::ModelEditorScreen::operator=(ModelEditorScreen&&) noexcept = default;
osc::ModelEditorScreen::~ModelEditorScreen() noexcept = default;

void osc::ModelEditorScreen::onMount() {
    App::cur().makeMainEventLoopWaiting();
    osc::ImGuiInit();
}

void osc::ModelEditorScreen::onUnmount() {
    App::cur().makeMainEventLoopPolling();
    osc::ImGuiShutdown();
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
    if (m_Impl->filePoller.changeWasDetected(m_Impl->st->model().getInputFileName())) {
        modelEditorOnBackingFileChanged(*m_Impl);
    }
}

void osc::ModelEditorScreen::draw() {
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    osc::ImGuiNewFrame();
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);

    try {
        ::modelEditorDrawUnguarded(*m_Impl);
    } catch (std::exception const& ex) {
        log::error("an OpenSim::Exception was thrown while drawing the editor");
        log::error("    message = %s", ex.what());
        log::error("OpenSim::Exceptions typically happen when the model is damaged or made invalid by an edit (e.g. setting a property to an invalid value)");

        try {
            if (m_Impl->st->canUndo()) {
                log::error("the editor has an `undo` history for this model, so it will try to rollback to that");
                m_Impl->st->editedModel.forciblyRollbackToEarlierState();
                log::error("rollback succeeded");
            } else {
                throw;
            }
        } catch (std::exception const& ex2) {
            App::cur().requestTransition<ErrorScreen>(ex2);
        }

        // try to put ImGui into a clean state
        osc::ImGuiShutdown();
        osc::ImGuiInit();
        osc::ImGuiNewFrame();
    }

    osc::ImGuiRender();
}
