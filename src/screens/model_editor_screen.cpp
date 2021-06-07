#include "model_editor_screen.hpp"

#include "src/3d/3d.hpp"
#include "src/application.hpp"
#include "src/config.hpp"
#include "src/log.hpp"
#include "src/opensim_bindings/simulation.hpp"
#include "src/opensim_bindings/opensim_helpers.hpp"
#include "src/opensim_bindings/type_registry.hpp"
#include "src/main_editor_state.hpp"
#include "src/screens/show_model_screen.hpp"
#include "src/screens/splash_screen.hpp"
#include "src/screens/simulator_screen.hpp"
#include "src/ui/add_body_popup.hpp"
#include "src/ui/attach_geometry_popup.hpp"
#include "src/ui/component_details.hpp"
#include "src/ui/component_hierarchy.hpp"
#include "src/ui/fd_params_editor_popup.hpp"
#include "src/ui/help_marker.hpp"
#include "src/ui/log_viewer.hpp"
#include "src/ui/main_menu.hpp"
#include "src/ui/model_actions.hpp"
#include "src/ui/component_3d_viewer.hpp"
#include "src/ui/properties_editor.hpp"
#include "src/ui/reassign_socket_popup.hpp"
#include "src/ui/select_1_pf_popup.hpp"
#include "src/ui/select_2_pfs_popup.hpp"
#include "src/ui/select_component_popup.hpp"
#include "src/utils/circular_buffer.hpp"
#include "src/utils/file_change_poller.hpp"
#include "src/utils/scope_guard.hpp"
#include "src/utils/sdl_wrapper.hpp"
#include "src/utils/os.hpp"

#include <OpenSim/Common/AbstractProperty.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ModelDisplayHints.h>
#include <OpenSim/Common/Property.h>
#include <OpenSim/Common/PropertyObjArray.h>
#include <OpenSim/Common/Set.h>
#include <OpenSim/Simulation/Model/BodySet.h>
#include <OpenSim/Simulation/Model/BushingForce.h>
#include <OpenSim/Simulation/Model/ContactHalfSpace.h>
#include <OpenSim/Simulation/Model/ContactSphere.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/JointSet.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/Model/PointToPointSpring.h>
#include <OpenSim/Simulation/SimbodyEngine/BallJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PointOnLineConstraint.h>
#include <OpenSim/Simulation/SimbodyEngine/UniversalJoint.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SimTKcommon.h>
#include <SimTKcommon/Mechanics.h>
#include <SimTKcommon/SmallMatrix.h>
#include <imgui.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <typeinfo>
#include <vector>
#include <array>

namespace fs = std::filesystem;
using namespace osc;
using namespace osc::ui;
using std::literals::operator""s;
using std::literals::operator""ms;

template<typename T>
static T const* find_ancestor(OpenSim::Component const* c) {
    while (c) {
        T const* p = dynamic_cast<T const*>(c);
        if (p) {
            return p;
        }
        c = c->hasOwner() ? &c->getOwner() : nullptr;
    }
    return nullptr;
}

static Component_3d_viewer create_viewer() {
    return Component_3d_viewer{Component3DViewerFlags_Default | Component3DViewerFlags_DrawFrames};
}

struct Model_editor_screen::Impl final {
    // top-level state this screen can handle
    std::shared_ptr<Main_editor_state> st;

    // 3d viewers
    //
    // user can be viewing up to 4 of these, if they want
    std::array<std::optional<Component_3d_viewer>, 4> viewers = {
        create_viewer(),
        std::nullopt,
        std::nullopt,
        std::nullopt,
    };

    // internal state of any sub-panels the editor screen draws
    struct {
        ui::main_menu::file_tab::State main_menu_tab;
        ui::add_body_popup::State abm;
        ui::properties_editor::State properties_editor;
        ui::reassign_socket::State reassign_socket;
        ui::attach_geometry_popup::State attach_geometry_modal;
        ui::select_2_pfs::State select_2_pfs;
        ui::model_actions::State model_actions_panel;
        ui::log_viewer::State log_viewer;
    } ui;

    // which windows are currently showing
    struct {
        bool hierarchy = true;
        bool selection_details = true;
        bool property_editor = true;
        bool log = true;
        bool actions = true;
    } showing;

    // state that is reset at the start of each frame
    struct {
        bool edit_sim_params_requested = false;
        bool subpanel_requested_early_exit = false;
    } reset_per_frame;

    // poller that checks (with debouncing) when model being edited has changed on the filesystem
    File_change_poller file_poller{1000ms, st->model().getInputFileName()};

    explicit Impl(std::shared_ptr<Main_editor_state> _st) : st{std::move(_st)} {
    }
};

static void draw_top_level_editor(Undoable_ui_model& st) {
    if (!st.selection()) {
        ImGui::Text("cannot draw top level editor: nothing selected?");
        return;
    }
    OpenSim::Component& selection = *st.selection();

    ImGui::Columns(2);

    ImGui::TextUnformatted("name");
    ImGui::NextColumn();

    char nambuf[128];
    nambuf[sizeof(nambuf) - 1] = '\0';
    std::strncpy(nambuf, selection.getName().c_str(), sizeof(nambuf) - 1);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
    if (ImGui::InputText("##nameditor", nambuf, sizeof(nambuf), ImGuiInputTextFlags_EnterReturnsTrue)) {

        if (std::strlen(nambuf) > 0) {
            st.before_modifying_model();
            selection.setName(nambuf);
            st.after_modifying_model();
        }
    }
    ImGui::NextColumn();

    ImGui::Columns();
}

static void draw_frame_contextual_actions(Model_editor_screen::Impl& impl, OpenSim::PhysicalFrame& selection) {
    ImGui::Columns(2);

    ImGui::TextUnformatted("geometry");
    ImGui::SameLine();
    help_marker::draw("Geometry that is attached to this physical frame. Multiple pieces of geometry can be attached to the frame");
    ImGui::NextColumn();

    static constexpr char const* modal_name = "select geometry to add";

    if (ImGui::Button("add geometry")) {
        ImGui::OpenPopup(modal_name);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted("Add geometry to this component. Geometry can be removed by selecting it in the hierarchy editor and pressing DELETE");
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

    if (auto attached = attach_geometry_popup::draw(impl.ui.attach_geometry_modal, modal_name); attached) {
        impl.st->before_modifying_model();
        selection.attachGeometry(attached.release());
        impl.st->after_modifying_model();
    }
    ImGui::NextColumn();

    ImGui::TextUnformatted("offset frame");
    ImGui::NextColumn();
    if (ImGui::Button("add offset frame")) {
        auto pof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        pof->setName(selection.getName() + "_offsetframe");
        pof->setParentFrame(selection);

        impl.st->before_modifying_model();
        auto pofptr = pof.get();
        selection.addComponent(pof.release());
        impl.st->set_selection(pofptr);
        impl.st->after_modifying_model();
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

static void copy_common_joint_properties(OpenSim::Joint const& src, OpenSim::Joint& dest) {
    dest.setName(src.getName());

    // copy owned frames
    dest.updProperty_frames().assign(src.getProperty_frames());

    // copy, or reference, the parent based on whether the source owns it
    {
        OpenSim::PhysicalFrame const& src_parent = src.getParentFrame();
        bool parent_assigned = false;
        for (int i = 0; i < src.getProperty_frames().size(); ++i) {
            if (&src.get_frames(i) == &src_parent) {
                // the source's parent is also owned by the source, so we need to
                // ensure the destination refers to its own (cloned, above) copy
                dest.connectSocket_parent_frame(dest.get_frames(i));
                parent_assigned = true;
                break;
            }
        }
        if (!parent_assigned) {
            // the source's parent is a reference to some frame that the source
            // doesn't, itself, own, so the destination should just also refer
            // to the same (not-owned) frame
            dest.connectSocket_parent_frame(src_parent);
        }
    }

    // copy, or reference, the child based on whether the source owns it
    {
        OpenSim::PhysicalFrame const& src_child = src.getChildFrame();
        bool child_assigned = false;
        for (int i = 0; i < src.getProperty_frames().size(); ++i) {
            if (&src.get_frames(i) == &src_child) {
                // the source's child is also owned by the source, so we need to
                // ensure the destination refers to its own (cloned, above) copy
                dest.connectSocket_child_frame(dest.get_frames(i));
                child_assigned = true;
                break;
            }
        }
        if (!child_assigned) {
            // the source's child is a reference to some frame that the source
            // doesn't, itself, own, so the destination should just also refer
            // to the same (not-owned) frame
            dest.connectSocket_child_frame(src_child);
        }
    }
}

static void draw_joint_type_switcher(Undoable_ui_model& st, OpenSim::Joint& selection) {
    auto const* parent_jointset =
        selection.hasOwner() ? dynamic_cast<OpenSim::JointSet const*>(&selection.getOwner()) : nullptr;

    if (!parent_jointset) {
        // it's a joint, but it's not owned by a JointSet, so the implementation cannot switch
        // the joint type
        return;
    }

    OpenSim::JointSet const& js = *parent_jointset;

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
    std::optional<size_t> maybe_type_idx = Joint_registry::index_of(selection);
    int type_idx = maybe_type_idx ? static_cast<int>(*maybe_type_idx) : -1;

    auto known_joint_names = Joint_registry::names();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
    if (ImGui::Combo(
            "##newjointtypeselector",
            &type_idx,
            known_joint_names.data(),
            static_cast<int>(known_joint_names.size())) &&
        type_idx >= 0) {

        // copy + fixup  a prototype of the user's selection
        std::unique_ptr<OpenSim::Joint> new_joint{Joint_registry::prototypes()[static_cast<size_t>(type_idx)]->clone()};
        copy_common_joint_properties(selection, *new_joint);

        // overwrite old joint in model
        //
        // note: this will invalidate the `selection` joint, because the
        // OpenSim::JointSet container will automatically kill it
        st.before_modifying_model();
        OpenSim::Joint* ptr = new_joint.get();
        const_cast<OpenSim::JointSet&>(js).set(idx, new_joint.release());
        st.declare_death_of(&selection);
        st.set_selection(ptr);
        st.after_modifying_model();
    }
    ImGui::NextColumn();
}

static void draw_joint_contextual_actions(Undoable_ui_model& st, OpenSim::Joint& selection) {

    ImGui::Columns(2);

    draw_joint_type_switcher(st, selection);

    // BEWARE: broke
    {
        ImGui::Text("add offset frame");
        ImGui::NextColumn();

        if (ImGui::Button("parent")) {
            auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
            pf->setParentFrame(selection.getParentFrame());

            st.before_modifying_model();
            selection.addFrame(pf.release());
            st.after_modifying_model();
        }
        ImGui::SameLine();
        if (ImGui::Button("child")) {
            auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
            pf->setParentFrame(selection.getChildFrame());

            st.before_modifying_model();
            selection.addFrame(pf.release());
            st.after_modifying_model();
        }
        ImGui::NextColumn();
    }

    ImGui::Columns();
}

static void draw_hcf_contextual_actions(Undoable_ui_model& uim, OpenSim::HuntCrossleyForce& selection) {
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
    ui::help_marker::draw(
        "Add OpenSim::ContactGeometry to this OpenSim::HuntCrossleyForce.\n\nCollisions are evaluated for all OpenSim::ContactGeometry attached to the OpenSim::HuntCrossleyForce. E.g. if you want an OpenSim::ContactSphere component to collide with an OpenSim::ContactHalfSpace component during a simulation then you should add both of those components to this force");
    ImGui::NextColumn();

    // allow user to add geom
    {
        if (ImGui::Button("add contact geometry")) {
            ImGui::OpenPopup("select contact geometry");
        }

        ui::select_component::State s;
        OpenSim::ContactGeometry const* added =
            ui::select_component::draw<OpenSim::ContactGeometry>(s, "select contact geometry", uim.model());

        if (added) {
            uim.before_modifying_model();
            params.updGeometry().appendValue(added->getName());
            uim.after_modifying_model();
        }
    }

    ImGui::NextColumn();
    ImGui::Columns();

    // render standard, easy to render, props of the contact params
    {
        std::array<int, 6> easy_to_handle_props = {
            params.PropertyIndex_geometry,
            params.PropertyIndex_stiffness,
            params.PropertyIndex_dissipation,
            params.PropertyIndex_static_friction,
            params.PropertyIndex_dynamic_friction,
            params.PropertyIndex_viscous_friction,
        };

        properties_editor::State st;
        auto maybe_updater = properties_editor::draw(st, params, easy_to_handle_props);

        if (maybe_updater) {
            uim.before_modifying_model();
            maybe_updater->updater(const_cast<OpenSim::AbstractProperty&>(maybe_updater->prop));
            uim.after_modifying_model();
        }
    }
}

static void draw_pa_contextual_actions(Undoable_ui_model& uim, OpenSim::PathActuator& selection) {
    ImGui::Columns(2);

    char const* modal_name = "select physical frame";

    ImGui::TextUnformatted("add path point to end");
    ImGui::NextColumn();

    if (ImGui::Button("add")) {
        ImGui::OpenPopup(modal_name);
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
        OpenSim::PhysicalFrame const* pf = ui::select_1_pf::draw(modal_name, uim.model());
        if (pf) {
            int n = selection.getGeometryPath().getPathPointSet().getSize();
            char buf[128];
            std::snprintf(buf, sizeof(buf), "%s-P%i", selection.getName().c_str(), n + 1);
            std::string name{buf};
            SimTK::Vec3 pos{0.0f, 0.0f, 0.0f};

            uim.before_modifying_model();
            selection.addNewPathPoint(name, *pf, pos);
            uim.after_modifying_model();
        }
    }

    ImGui::NextColumn();
    ImGui::Columns();
}

static void draw_contextual_actions(Model_editor_screen::Impl& impl) {
    if (!impl.st->selection()) {
        ImGui::TextUnformatted("cannot draw contextual actions: selection is blank (shouldn't be)");
        return;
    }

    ImGui::Columns(2);
    ImGui::TextUnformatted("isolate in visualizer");
    ImGui::NextColumn();

    if (impl.st->selection() != impl.st->isolated()) {
        if (ImGui::Button("isolate")) {
            impl.st->before_modifying_model();
            impl.st->set_isolated(impl.st->selection());
            impl.st->after_modifying_model();
        }
    } else {
        if (ImGui::Button("un-isolate")) {
            impl.st->before_modifying_model();
            impl.st->set_isolated(nullptr);
            impl.st->after_modifying_model();
        }
    }

    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(
            "Only show this component in the visualizer\n\nThis can be disabled from the Edit menu (Edit -> Show all components)");
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    ImGui::NextColumn();
    ImGui::Columns();

    if (auto* frame = dynamic_cast<OpenSim::PhysicalFrame*>(impl.st->selection()); frame) {
        draw_frame_contextual_actions(impl, *frame);
    } else if (auto* joint = dynamic_cast<OpenSim::Joint*>(impl.st->selection()); joint) {
        draw_joint_contextual_actions(impl.st->edited_model, *joint);
    } else if (auto* hcf = dynamic_cast<OpenSim::HuntCrossleyForce*>(impl.st->selection()); hcf) {
        draw_hcf_contextual_actions(impl.st->edited_model, *hcf);
    } else if (auto* pa = dynamic_cast<OpenSim::PathActuator*>(impl.st->selection()); pa) {
        draw_pa_contextual_actions(impl.st->edited_model, *pa);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.5f, 0.5f, 0.5f, 1.0f});
        ImGui::Text(
            "    (OpenSim::%s has no contextual actions)", impl.st->selection()->getConcreteClassName().c_str());
        ImGui::PopStyleColor();
    }
}

static void draw_socket_editor(Model_editor_screen::Impl& impl) {

    if (!impl.st->selection()) {
        ImGui::TextUnformatted("cannot draw socket editor: selection is blank (shouldn't be)");
        return;
    }

    OpenSim::Component& selection = *impl.st->selection();

    std::vector<std::string> socknames = selection.getSocketNames();

    if (socknames.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.5f, 0.5f, 0.5f, 1.0f});
        ImGui::Text("    (OpenSim::%s has no sockets)", impl.st->selection()->getConcreteClassName().c_str());
        ImGui::PopStyleColor();
        return;
    }

    // else: it has sockets with names, list each socket and provide the user
    //       with the ability to reassign the socket's connectee

    ImGui::Columns(2);
    for (std::string const& sn : socknames) {
        ImGui::TextUnformatted(sn.c_str());
        ImGui::NextColumn();

        OpenSim::AbstractSocket const& socket = selection.getSocket(sn);
        std::string sockname = socket.getConnecteePath();
        std::string popupname = std::string{"reassign"} + sockname;

        if (ImGui::Button(sockname.c_str())) {
            ImGui::OpenPopup(popupname.c_str());
        }

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::Text(
                "%s\n\nClick to reassign this socket's connectee",
                socket.getConnecteeAsObject().getConcreteClassName().c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        if (OpenSim::Object const* connectee =
                ui::reassign_socket::draw(impl.ui.reassign_socket, popupname.c_str(), impl.st->model(), socket);
            connectee) {

            ImGui::CloseCurrentPopup();

            OpenSim::Object const& existing = socket.getConnecteeAsObject();
            try {
                impl.st->before_modifying_model();
                selection.updSocket(sn).connect(*connectee);
                impl.ui.reassign_socket.search[0] = '\0';
                impl.ui.reassign_socket.error.clear();
                ImGui::CloseCurrentPopup();
            } catch (std::exception const& ex) {
                impl.ui.reassign_socket.error = ex.what();
                selection.updSocket(sn).connect(existing);
            }
            impl.st->after_modifying_model();
        }

        ImGui::NextColumn();
    }
    ImGui::Columns();
}

static void draw_selection_breadcrumbs(Undoable_ui_model& uim) {
    if (!uim.selection()) {
        return;  // nothing selected
    }

    auto lst = osc::path_to(*uim.selection());

    if (lst.empty()) {
        return;  // this shouldn't happen, but you never know...
    }

    float indent = 0.0f;

    for (auto it = lst.begin(); it != lst.end() - 1; ++it) {
        ImGui::Dummy(ImVec2{indent, 0.0f});
        ImGui::SameLine();
        if (ImGui::Button((*it)->getName().c_str())) {
            uim.set_selection(const_cast<OpenSim::Component*>(*it));
        }
        if (ImGui::IsItemHovered()) {
            uim.set_hovered(const_cast<OpenSim::Component*>(*it));
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

static void draw_selection_editor(Model_editor_screen::Impl& impl) {
    if (!impl.st->selection()) {
        ImGui::TextUnformatted("(nothing selected)");
        return;
    }

    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    ImGui::TextUnformatted("hierarchy:");
    ImGui::SameLine();
    ui::help_marker::draw("Where the selected component is in the model's component hierarchy");
    ImGui::Separator();
    draw_selection_breadcrumbs(impl.st->edited_model);

    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    ImGui::TextUnformatted("top-level attributes:");
    ImGui::SameLine();
    ui::help_marker::draw("Top-level properties on the OpenSim::Component itself");
    ImGui::Separator();
    draw_top_level_editor(impl.st->edited_model);

    // contextual actions
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    ImGui::TextUnformatted("contextual actions:");
    ImGui::SameLine();
    ui::help_marker::draw("Actions that are specific to the type of OpenSim::Component that is currently selected");
    ImGui::Separator();
    draw_contextual_actions(impl);

    // a contextual action may have changed this
    if (!impl.st->selection()) {
        return;
    }

    // property editor
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    ImGui::TextUnformatted("properties:");
    ImGui::SameLine();
    ui::help_marker::draw(
        "Properties of the selected OpenSim::Component. These are declared in the Component's implementation.");
    ImGui::Separator();
    {
        auto maybe_updater = properties_editor::draw(impl.ui.properties_editor, *impl.st->selection());
        if (maybe_updater) {
            impl.st->before_modifying_model();
            maybe_updater->updater(const_cast<OpenSim::AbstractProperty&>(maybe_updater->prop));
            impl.st->after_modifying_model();
        }
    }

    // socket editor
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    ImGui::TextUnformatted("sockets:");
    ImGui::SameLine();
    ui::help_marker::draw(
        "What components this component is connected to.\n\nIn OpenSim, a Socket formalizes the dependency between a Component and another object (typically another Component) without owning that object. While Components can be composites (of multiple components) they often depend on unrelated objects/components that are defined and owned elsewhere. The object that satisfies the requirements of the Socket we term the 'connectee'. When a Socket is satisfied by a connectee we have a successful 'connection' or is said to be connected.");
    ImGui::Separator();
    draw_socket_editor(impl);
}

template<typename T, typename TSetBase = OpenSim::Object>
static void delete_item_from_set_in_model(OpenSim::Set<T, TSetBase>& set, T* item) {
    for (int i = 0; i < set.getSize(); ++i) {
        if (&set.get(i) == item) {
            set.remove(i);
            return;
        }
    }
}

static void action_delete_selection_from_model(osc::Undoable_ui_model& uim) {
    OpenSim::Component* selected = uim.selection();

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

        uim.before_modifying_model();
        delete_item_from_set_in_model(*js, static_cast<OpenSim::Joint*>(selected));
        uim.declare_death_of(selected);
        uim.after_modifying_model();
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

        // TODO: iterate over `PathWrap`s in the model and disconnect them from the
        // GeometryPath that uses them: otherwise, the wrapping in the model is going
        // to be invalid
        /*
        impl.before_modify_model();
        delete_item_from_set_in_model(*wos, static_cast<OpenSim::WrapObject*>(selected));
        impl.set_selection(nullptr);
        impl.set_hover(nullptr);
        impl.after_modify_model();
        */
    } else if (auto* cs = dynamic_cast<OpenSim::ControllerSet*>(owner); cs) {
        // delete an OpenSim::Controller from its owning OpenSim::ControllerSet

        uim.before_modifying_model();
        delete_item_from_set_in_model(*cs, static_cast<OpenSim::Controller*>(selected));
        uim.declare_death_of(selected);
        uim.after_modifying_model();
    } else if (auto* conss = dynamic_cast<OpenSim::ConstraintSet*>(owner); cs) {
        // delete an OpenSim::Constraint from its owning OpenSim::ConstraintSet

        uim.before_modifying_model();
        delete_item_from_set_in_model(*conss, static_cast<OpenSim::Constraint*>(selected));
        uim.declare_death_of(selected);
        uim.after_modifying_model();
    } else if (auto* fs = dynamic_cast<OpenSim::ForceSet*>(owner); fs) {
        // delete an OpenSim::Force from its owning OpenSim::ForceSet

        uim.before_modifying_model();
        delete_item_from_set_in_model(*fs, static_cast<OpenSim::Force*>(selected));
        uim.declare_death_of(selected);
        uim.after_modifying_model();
    } else if (auto* ms = dynamic_cast<OpenSim::MarkerSet*>(owner); ms) {
        // delete an OpenSim::Marker from its owning OpenSim::MarkerSet

        uim.before_modifying_model();
        delete_item_from_set_in_model(*ms, static_cast<OpenSim::Marker*>(selected));
        uim.declare_death_of(selected);
        uim.after_modifying_model();
    } else if (auto* cgs = dynamic_cast<OpenSim::ContactGeometrySet*>(owner); cgs) {
        // delete an OpenSim::ContactGeometry from its owning OpenSim::ContactGeometrySet

        uim.before_modifying_model();
        delete_item_from_set_in_model(*cgs, static_cast<OpenSim::ContactGeometry*>(selected));
        uim.declare_death_of(selected);
        uim.after_modifying_model();
    } else if (auto* ps = dynamic_cast<OpenSim::ProbeSet*>(owner); ps) {
        // delete an OpenSim::Probe from its owning OpenSim::ProbeSet

        uim.before_modifying_model();
        delete_item_from_set_in_model(*ps, static_cast<OpenSim::Probe*>(selected));
        uim.declare_death_of(selected);
        uim.after_modifying_model();
    } else if (auto const* geom = find_ancestor<OpenSim::Geometry>(selected); geom) {
        // delete an OpenSim::Geometry from its owning OpenSim::Frame

        if (auto const* frame = find_ancestor<OpenSim::Frame>(geom); frame) {
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

            uim.before_modifying_model();
            prop.assign(*copy);
            uim.declare_death_of(selected);
            uim.after_modifying_model();
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
                uim.before_modifying_model();
                gp->deletePathPoint(uim.state(), idx);
                uim.declare_death_of(selected);
                uim.after_modifying_model();
            }
        }
    }
}

static void action_undo(Model_editor_screen::Impl& impl) {
    if (impl.st->can_undo()) {
        impl.st->do_undo();
    }
}

static void action_redo(Model_editor_screen::Impl& impl) {
    if (impl.st->can_redo()) {
        impl.st->do_redo();
    }
}

static void action_disable_all_wrapping_surfs(Model_editor_screen::Impl& impl) {
    OpenSim::Model& m = impl.st->model();
    impl.st->before_modifying_model();
    for (OpenSim::WrapObjectSet& wos : m.updComponentList<OpenSim::WrapObjectSet>()) {
        for (int i = 0; i < wos.getSize(); ++i) {
            OpenSim::WrapObject& wo = wos[i];
            wo.set_active(false);
            wo.upd_Appearance().set_visible(false);
        }
    }
    impl.st->after_modifying_model();
}

static void action_enable_all_wrapping_surfs(Model_editor_screen::Impl& impl) {
    OpenSim::Model& m = impl.st->model();
    impl.st->before_modifying_model();
    for (OpenSim::WrapObjectSet& wos : m.updComponentList<OpenSim::WrapObjectSet>()) {
        for (int i = 0; i < wos.getSize(); ++i) {
            OpenSim::WrapObject& wo = wos[i];
            wo.set_active(true);
            wo.upd_Appearance().set_visible(true);
        }
    }
    impl.st->after_modifying_model();
}

static bool has_backing_file(OpenSim::Model const& m) {
    return m.getInputFileName() != "Unassigned";
}

static void draw_main_menu_actions_tab(osc::Model_editor_screen::Impl& impl) {
    if (ImGui::BeginMenu("Edit")) {

        if (ImGui::MenuItem("Undo", "Ctrl+Z", false, impl.st->can_undo())) {
            action_undo(impl);
        }

        if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z", false, impl.st->can_redo())) {
            action_redo(impl);
        }

        if (ImGui::MenuItem("Clear Selection", "Ctrl+A")) {
            impl.st->set_selection(nullptr);
        }

        if (ImGui::MenuItem("Show all components")) {
            impl.st->set_isolated(nullptr);
        }

        if (ImGui::BeginMenu("Utilities")) {
            if (ImGui::MenuItem("Disable all wrapping surfaces")) {
                action_disable_all_wrapping_surfs(impl);
            }

            if (ImGui::MenuItem("Enable all wrapping surfaces")) {
                action_enable_all_wrapping_surfs(impl);
            }

            if (ImGui::MenuItem("Open Model in external editor", nullptr, false, has_backing_file(impl.st->model()))) {
                open_path_in_default_application(impl.st->model().getInputFileName());
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }
}

static bool on_keydown(osc::Model_editor_screen::Impl& impl, SDL_KeyboardEvent const& e) {
    if (e.keysym.mod & KMOD_CTRL) {
        // CTRL

        if (e.keysym.mod & KMOD_SHIFT) {
            // CTRL+SHIFT

            switch (e.keysym.sym) {
            case SDLK_s:
                ui::main_menu::action_save_as(impl.st->model());
                return true;
            case SDLK_z:
                action_redo(impl);
                return false;
            }
            return false;
        }

        switch (e.keysym.sym) {
        case SDLK_n:
            ui::main_menu::action_new_model(impl.st);
            return true;
        case SDLK_o:
            ui::main_menu::action_open_model(impl.st);
            return true;
        case SDLK_s:
            ui::main_menu::action_save(impl.st->model());
            return true;
        case SDLK_q:
            Application::current().request_quit_application();
            return true;
        case SDLK_z:
            action_undo(impl);
            return true;
        case SDLK_r:
            // Ctrl + r
            impl.st->start_simulating_edited_model();
            Application::current().request_screen_transition<Simulator_screen>(std::move(impl.st));
            return true;
        case SDLK_a:
            impl.st->set_selection(nullptr);
            return true;
        }

        return false;
    }

    switch (e.keysym.sym) {
    case SDLK_DELETE:
        action_delete_selection_from_model(impl.st->edited_model);
        return true;
    case SDLK_F12:
        ui::main_menu::action_save_as(impl.st->model());
        return true;
    }

    return false;
}

static void on_model_backing_file_changed(osc::Model_editor_screen::Impl& impl) {
    try {
        log::info("file change detected: loading updated file");
        auto p = std::make_unique<OpenSim::Model>(impl.st->model().getInputFileName());
        log::info("loaded updated file");
        impl.st->set_model(std::move(p));
    } catch (std::exception const& ex) {
        log::error("error occurred while trying to automatically load a model file:");
        log::error(ex.what());
        log::error("the file will not be loaded into osc (you won't see the change in the UI)");
    }
}

static void tick(osc::Model_editor_screen::Impl& impl) {
    if (impl.file_poller.change_detected(impl.st->model().getInputFileName())) {
        on_model_backing_file_changed(impl);
    }
}

static bool on_event(osc::Model_editor_screen::Impl& impl, SDL_Event const& e) {
    bool handled = false;

    if (e.type == SDL_KEYDOWN) {
        handled = on_keydown(impl, e.key);
    }

    // if the screen didn't handle the event, forward it into the 3D viewers
    for (auto& viewer : impl.viewers) {
        if (!handled && viewer && viewer->is_moused_over()) {
            handled = viewer->on_event(e);
        }
    }

    return handled;
}

// draw editor screen main menu
//
// this is the bar that appears at the top of the screen
static void draw_main_menu(osc::Model_editor_screen::Impl& impl) {
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }

    // draw "file" tab
    ui::main_menu::file_tab::draw(impl.ui.main_menu_tab, impl.st);

    // draw "actions" tab
    draw_main_menu_actions_tab(impl);

    // draw "window" tab
    if (ImGui::BeginMenu("Window")) {

        ImGui::MenuItem("Actions", nullptr, &impl.showing.actions);
        ImGui::MenuItem("Hierarchy", nullptr, &impl.showing.hierarchy);
        ImGui::MenuItem("Log", nullptr, &impl.showing.log);
        ImGui::MenuItem("Property Editor", nullptr, &impl.showing.property_editor);
        ImGui::MenuItem("Selection Details", nullptr, &impl.showing.selection_details);

        for (size_t i = 0; i < impl.viewers.size(); ++i) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "viewer%zu", i);

            bool is_enabled = impl.viewers[i].has_value();
            if (ImGui::MenuItem(buf, nullptr, &is_enabled)) {
                if (is_enabled) {
                    // was enabled by user click
                    impl.viewers[i] = create_viewer();
                } else {
                    // was disabled by user click
                    impl.viewers[i] = std::nullopt;
                }
            }
        }

        ImGui::EndMenu();
    }

    // draw "about" tab
    ui::main_menu::about_tab::draw();

    ImGui::Dummy(ImVec2{2.0f, 0.0f});
    if (ImGui::Button("Show simulations")) {
        Application::current().request_screen_transition<Simulator_screen>(std::move(impl.st));
        ImGui::EndMainMenuBar();
        impl.reset_per_frame.subpanel_requested_early_exit = true;
        return;
    }

    // "switch to simulator" menu button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.0f, 0.6f, 0.0f, 1.0f});
    if (ImGui::Button("Simulate (Ctrl+R)")) {
        impl.st->start_simulating_edited_model();
        Application::current().request_screen_transition<Simulator_screen>(std::move(impl.st));
        ImGui::PopStyleColor();
        ImGui::EndMainMenuBar();
        impl.reset_per_frame.subpanel_requested_early_exit = true;
        return;
    }
    ImGui::PopStyleColor();

    if (ImGui::Button("Edit sim settings")) {
        impl.reset_per_frame.edit_sim_params_requested = true;
    }

    ImGui::EndMainMenuBar();
}

static void draw_3d_hover_tooltips(OpenSim::Component const& hovered) {
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

static void draw_3d_selection_context_menu(
        osc::Model_editor_screen::Impl& impl,
        OpenSim::Component const& selected) {

    ImGui::TextDisabled("%s (%s)", selected.getName().c_str(), selected.getConcreteClassName().c_str());
    ImGui::Separator();
    ImGui::Dummy(ImVec2{0.0f, 3.0f});

    if (ImGui::BeginMenu("Select Owner")) {
        OpenSim::Component const* c = &selected;
        impl.st->set_hovered(nullptr);
        while (c->hasOwner()) {
            c = &c->getOwner();

            char buf[128];
            std::snprintf(buf, sizeof(buf), "%s (%s)", c->getName().c_str(), c->getConcreteClassName().c_str());

            if (ImGui::MenuItem(buf)) {
                impl.st->set_selection(const_cast<OpenSim::Component*>(c));
            }
            if (ImGui::IsItemHovered()) {
                impl.st->set_hovered(const_cast<OpenSim::Component*>(c));
            }
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Request Outputs")) {
        help_marker::draw("Request that these outputs are plotted whenever a simulation is ran. The outputs will appear in the 'outputs' tab on the simulator screen");

        OpenSim::Component const* c = &selected;
        while (c) {
            ImGui::Dummy(ImVec2{0.0f, 2.0f});
            ImGui::TextDisabled("%s (%s)", c->getName().c_str(), c->getConcreteClassName().c_str());
            ImGui::Separator();
            for (auto const& o : c->getOutputs()) {
                char buf[256];
                std::snprintf(buf, sizeof(buf), "  %s", o.second->getName().c_str());

                if (ImGui::MenuItem(buf)) {
                   impl.st->desired_outputs.emplace_back(c->getAbsolutePath().toString(), o.second->getName());
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Output Type = %s", o.second->getTypeName().c_str());
                    ImGui::EndTooltip();
                }
            }
            if (c->getNumOutputs() == 0) {
                ImGui::TextDisabled("  (has no outputs)");
            }
            c = c->hasOwner() ? &c->getOwner() : nullptr;
        }
        ImGui::EndMenu();
    }
}

// draw a single 3D model viewer
static void draw_3d_viewer(
        osc::Model_editor_screen::Impl& impl,
        Component_3d_viewer& viewer,
        char const* name) {

    Component3DViewerResponse resp;

    if (impl.st->isolated()) {
        resp = viewer.draw(
            name,
            *impl.st->isolated(),
            impl.st->model().getDisplayHints(),
            impl.st->state(),
            impl.st->selection(),
            impl.st->hovered());
    } else {
        resp = viewer.draw(
            name,
            impl.st->model(),
            impl.st->state(),
            impl.st->selection(),
            impl.st->hovered());
    }

    // update hover
    if (resp.is_moused_over && resp.hovertest_result != impl.st->hovered()) {
        impl.st->set_hovered(const_cast<OpenSim::Component*>(resp.hovertest_result));
    }

    // if left-clicked, update selection
    if (resp.is_moused_over && resp.is_left_clicked) {
        impl.st->set_selection(const_cast<OpenSim::Component*>(resp.hovertest_result));
    }

    // if hovered, draw hover tooltip
    if (resp.is_moused_over && resp.hovertest_result) {
        draw_3d_hover_tooltips(*resp.hovertest_result);
    }

    // if right-clicked, draw context menu
    {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "%s_contextmenu", name);
        if (resp.is_moused_over && resp.hovertest_result && resp.is_right_clicked) {
            impl.st->set_selection(const_cast<OpenSim::Component*>(resp.hovertest_result));
            ImGui::OpenPopup(buf);
        }
        if (impl.st->selection() && ImGui::BeginPopup(buf)) {
            draw_3d_selection_context_menu(impl, *impl.st->selection());
            ImGui::EndPopup();
        }
    }
}

// draw all user-enabled 3D model viewers
static void draw_3d_viewers(osc::Model_editor_screen::Impl& impl) {
    for (size_t i = 0; i < impl.viewers.size(); ++i) {
        std::optional<Component_3d_viewer>& maybe_viewer = impl.viewers[i];

        if (!maybe_viewer) {
            continue;
        }

        Component_3d_viewer& viewer = *maybe_viewer;

        char buf[64];
        std::snprintf(buf, sizeof(buf), "viewer%zu", i);

        draw_3d_viewer(impl, viewer, buf);
    }
}

// top-level draw function for the editor screen
static void draw(osc::Model_editor_screen::Impl& impl) {
    impl.reset_per_frame = {};

    // draw main menu
    {
        draw_main_menu(impl);
    }

    // check for early exit request
    //
    // (the main menu may have requested a screen transition)
    if (impl.reset_per_frame.subpanel_requested_early_exit) {
        return;
    }

    // draw 3D viewers (if any)
    {
        draw_3d_viewers(impl);
    }

    // draw editor actions panel
    //
    // contains top-level actions (e.g. "add body")
    if (impl.showing.actions) {
        if (ImGui::Begin("Actions", nullptr, ImGuiWindowFlags_MenuBar)) {
            auto on_set_selection = [&](OpenSim::Component* c) { impl.st->set_selection(c); };
            auto before_modify_model = [&]() { impl.st->before_modifying_model(); };
            auto after_modify_model = [&]() { impl.st->after_modifying_model(); };
            ui::model_actions::draw(
                impl.ui.model_actions_panel, impl.st->model(), on_set_selection, before_modify_model, after_modify_model);
        }
        ImGui::End();
    }

    // draw hierarchy viewer
    if (impl.showing.hierarchy) {
        if (ImGui::Begin("Hierarchy", &impl.showing.hierarchy)) {
            auto resp = ui::component_hierarchy::draw(
                &impl.st->model().getRoot(),
                impl.st->selection(),
                impl.st->hovered());

            if (resp.type == component_hierarchy::SelectionChanged) {
                impl.st->set_selection(const_cast<OpenSim::Component*>(resp.ptr));
            } else if (resp.type == component_hierarchy::HoverChanged) {
                impl.st->set_hovered(const_cast<OpenSim::Component*>(resp.ptr));
            }
        }
        ImGui::End();
    }

    // draw selection details
    if (impl.showing.selection_details) {
        if (ImGui::Begin("Selection", &impl.showing.selection_details)) {
            auto resp = component_details::draw(impl.st->state(), impl.st->selection());

            if (resp.type == component_details::SelectionChanged) {
                impl.st->set_selection(const_cast<OpenSim::Component*>(resp.ptr));
            }
        }
        ImGui::End();
    }

    // draw property editor
    if (impl.showing.property_editor) {
        if (ImGui::Begin("Edit Props", &impl.showing.property_editor)) {
            draw_selection_editor(impl);
        }
        ImGui::End();
    }

    // draw application log
    if (impl.showing.log) {
        if (ImGui::Begin("Log", &impl.showing.log, ImGuiWindowFlags_MenuBar)) {
            ui::log_viewer::draw(impl.ui.log_viewer);
        }
        ImGui::End();
    }

    // draw sim params editor popup (if applicable)
    {
        if (impl.reset_per_frame.edit_sim_params_requested) {
            ImGui::OpenPopup("simulation parameters");
        }

        osc::ui::fd_params_editor_popup::draw("simulation parameters", impl.st->sim_params);
    }

    if (impl.reset_per_frame.subpanel_requested_early_exit) {
        return;
    }

    // garbage-collect any models damaged by in-UI modifications (if applicable)
    {
        impl.st->clear_any_damaged_models();
    }
}

// Model_editor_screen interface

Model_editor_screen::Model_editor_screen() : 
    impl{new Impl(std::make_unique<Main_editor_state>())} {
}

Model_editor_screen::Model_editor_screen(std::unique_ptr<OpenSim::Model> _model) : 
    impl{new Impl(std::make_unique<Main_editor_state>(std::move(_model)))} {
}

Model_editor_screen::Model_editor_screen(std::shared_ptr<Main_editor_state> st) :
    impl{new Impl {std::move(st)}} {
}

Model_editor_screen::~Model_editor_screen() noexcept {
    delete impl;
}

bool Model_editor_screen::on_event(SDL_Event const& e) {
    return ::on_event(*impl, e);
}

void osc::Model_editor_screen::tick(float) {
    ::tick(*impl);
}

void osc::Model_editor_screen::draw() {
    ::draw(*impl);
}
