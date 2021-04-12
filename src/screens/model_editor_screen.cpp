#include "model_editor_screen.hpp"

#include "src/3d/gpu_cache.hpp"
#include "src/application.hpp"
#include "src/config.hpp"
#include "src/log.hpp"
#include "src/opensim_bindings/fd_simulation.hpp"
#include "src/opensim_bindings/opensim_helpers.hpp"
#include "src/opensim_bindings/type_registry.hpp"
#include "src/screens/show_model_screen.hpp"
#include "src/screens/splash_screen.hpp"
#include "src/utils/circular_buffer.hpp"
#include "src/utils/file_change_poller.hpp"
#include "src/utils/scope_guard.hpp"
#include "src/utils/sdl_wrapper.hpp"
#include "src/widgets/add_body_modal.hpp"
#include "src/widgets/add_joint_modal.hpp"
#include "src/widgets/attach_geometry_modal.hpp"
#include "src/widgets/component_hierarchy_widget.hpp"
#include "src/widgets/component_selection_widget.hpp"
#include "src/widgets/help_marker.hpp"
#include "src/widgets/log_viewer_widget.hpp"
#include "src/widgets/main_menu_about_tab.hpp"
#include "src/widgets/main_menu_file_tab.hpp"
#include "src/widgets/model_actions_panel.hpp"
#include "src/widgets/model_viewer_widget.hpp"
#include "src/widgets/properties_editor.hpp"
#include "src/widgets/reassign_socket_modal.hpp"
#include "src/widgets/select_2_pfs_modal.hpp"

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

namespace fs = std::filesystem;
using namespace osc;
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

// bundles together a model+state with pointers pointing into them, with support for
// relocate-ability
struct Model_ui_state final {
    static std::unique_ptr<OpenSim::Model> copy_model(OpenSim::Model const& model) {
        auto copy = std::make_unique<OpenSim::Model>(model);
        copy->finalizeFromProperties();
        copy->finalizeConnections();
        return copy;
    }

    static SimTK::State init_fresh_system_and_state(OpenSim::Model& model) {
        model.finalizeFromProperties();
        model.finalizeConnections();
        SimTK::State rv = model.initSystem();
        model.realizePosition(rv);
        return rv;
    }

    // relocate a component pointer to point into `model`, rather into whatever
    // model it used to point into
    static OpenSim::Component*
        relocate_component_pointer_to_new_model(OpenSim::Model const& model, OpenSim::Component* ptr) {
        if (!ptr) {
            return nullptr;
        }

        try {
            return const_cast<OpenSim::Component*>(model.findComponent(ptr->getAbsolutePath()));
        } catch (OpenSim::Exception const&) {
            // finding fails with exception when ambiguous (fml)
            return nullptr;
        }
    }

    std::unique_ptr<OpenSim::Model> model = nullptr;
    SimTK::State state = {};
    OpenSim::Component* selected_component = nullptr;
    OpenSim::Component* hovered_component = nullptr;
    OpenSim::Component* isolated_component = nullptr;

    Model_ui_state() = default;

    Model_ui_state(std::unique_ptr<OpenSim::Model> _model) :
        model{std::move(_model)},
        state{init_fresh_system_and_state(*model)},
        selected_component{nullptr},
        hovered_component{nullptr},
        isolated_component{nullptr} {
    }

    Model_ui_state(Model_ui_state const& other) :
        model{copy_model(*other.model)},
        state{init_fresh_system_and_state(*model)},
        selected_component{relocate_component_pointer_to_new_model(*model, other.selected_component)},
        hovered_component{relocate_component_pointer_to_new_model(*model, other.hovered_component)},
        isolated_component{relocate_component_pointer_to_new_model(*model, other.isolated_component)} {
    }

    Model_ui_state(Model_ui_state&& tmp) :
        model{std::move(tmp.model)},
        state{std::move(tmp.state)},
        selected_component{std::move(tmp.selected_component)},
        hovered_component{std::move(tmp.hovered_component)},
        isolated_component{std::move(tmp.isolated_component)} {
    }

    friend void swap(Model_ui_state& a, Model_ui_state& b) {
        std::swap(a.model, b.model);
        std::swap(a.state, b.state);
        std::swap(a.selected_component, b.selected_component);
        std::swap(a.hovered_component, b.hovered_component);
        std::swap(a.isolated_component, b.isolated_component);
    }

    Model_ui_state& operator=(Model_ui_state other) {
        swap(*this, other);
        return *this;
    }

    Model_ui_state& operator=(std::unique_ptr<OpenSim::Model> ptr) {
        if (model == ptr) {
            return *this;
        }

        std::swap(model, ptr);
        state = init_fresh_system_and_state(*model);
        selected_component = relocate_component_pointer_to_new_model(*model, selected_component);
        hovered_component = relocate_component_pointer_to_new_model(*model, hovered_component);
        isolated_component = relocate_component_pointer_to_new_model(*model, isolated_component);

        return *this;
    }

    // beware: can throw, because the model might've been put into an invalid
    // state by the modification
    void on_model_modified() {
        state = init_fresh_system_and_state(*model);
        selected_component = relocate_component_pointer_to_new_model(*model, selected_component);
        hovered_component = relocate_component_pointer_to_new_model(*model, hovered_component);
        isolated_component = relocate_component_pointer_to_new_model(*model, isolated_component);
    }
};

struct Undo_redo_entry final {
    std::chrono::system_clock::time_point time;
    Model_ui_state state;

    Undo_redo_entry() = default;

    template<typename... Args>
    Undo_redo_entry(std::chrono::system_clock::time_point t, Args&&... args) :
        time{std::move(t)},
        state{std::forward<Args>(args)...} {
    }

    template<typename... Args>
    Undo_redo_entry(Args&&... args) : time{std::chrono::system_clock::now()}, state{std::forward<Args>(args)...} {
    }
};

struct Model_editor_screen::Impl final {
    // model + state being edited by the user
    Model_ui_state ms;

    // cache of meshes, textures, etc. currently held on the GPU
    Gpu_cache gpu_cache;

    // sequence of undo/redo states that the user can transition the screen to
    Circular_buffer<Undo_redo_entry, 32> undo;
    Circular_buffer<Undo_redo_entry, 32> redo;

    // state for a 3D model viewer
    Model_viewer_widget model_viewer{gpu_cache, ModelViewerWidgetFlags_Default | ModelViewerWidgetFlags_DrawFrames};

    // state of any sub-panels the editor screen draws
    struct {
        Main_menu_file_tab_state main_menu_tab;
        Added_body_modal_state abm;
        std::array<Add_joint_modal, 4> add_joint_modals = {
            Add_joint_modal::create<OpenSim::FreeJoint>("Add FreeJoint"),
            Add_joint_modal::create<OpenSim::PinJoint>("Add PinJoint"),
            Add_joint_modal::create<OpenSim::UniversalJoint>("Add UniversalJoint"),
            Add_joint_modal::create<OpenSim::BallJoint>("Add BallJoint")};
        Properties_editor_state properties_editor;
        Reassign_socket_modal_state reassign_socket;
        Attach_geometry_modal_state attach_geometry_modal;
        Select_2_pfs_modal_state select_2_pfs;
        Model_actions_panel_state model_actions_panel;
        Log_viewer_widget_state log_viewer;
    } ui;

    // poller that checks (with debouncing) when model being edited has changed on the filesystem
    File_change_poller file_poller{1000ms, ms.model->getInputFileName()};

    // set to true if the UI has recovered from a disaster (probably an exception)
    bool recovered_from_disaster = false;

    // minimum time that must pass before the next undo state is recorded (when requested)
    //
    // prevents tiny changes that happen in quick succession from flooding the undo buffer
    static constexpr std::chrono::seconds undo_recording_debounce = 5s;

    // Absolute path to Component in the current Model that should be renderered in isolation

    void before_modify_model() {
        log::debug("starting model modification");
        attempt_new_undo_push();
    }

    void after_modify_model() {
        log::debug("ended model modification");
        try_update_model_system_and_state_with_rollback();
    }

    [[nodiscard]] OpenSim::Model& model() noexcept {
        return *ms.model;
    }

    void before_modify_selection() {
        before_modify_model();
    }

    void after_modify_selection() {
        after_modify_model();
    }

    void set_selection(OpenSim::Component* c) {
        ms.selected_component = c;
    }

    void set_isolated(OpenSim::Component* c) {
        ms.isolated_component = c;
    }

    [[nodiscard]] OpenSim::Component* selection() noexcept {
        return ms.selected_component;
    }

    [[nodiscard]] OpenSim::Component* hover() noexcept {
        return ms.hovered_component;
    }

    [[nodiscard]] OpenSim::Component* isolated() noexcept {
        return ms.isolated_component;
    }

    void set_hover(OpenSim::Component* c) {
        ms.hovered_component = c;
    }

    [[nodiscard]] bool can_undo() const noexcept {
        return !undo.empty();
    }

    [[nodiscard]] bool can_redo() const noexcept {
        return !redo.empty();
    }

    void attempt_new_undo_push() {
        auto now = std::chrono::system_clock::now();

        bool should_undo = undo.empty() || undo.back().time + undo_recording_debounce <= now;

        if (should_undo) {
            undo.emplace_back(now, ms);
            redo.clear();
        }
    }

    void do_redo() {
        if (redo.empty()) {
            return;
        }

        undo.emplace_back(std::move(ms));
        ms = std::move(redo.try_pop_back()).value().state;
    }

    void do_undo() {
        if (undo.empty()) {
            return;
        }

        redo.emplace_back(std::move(ms));
        ms = std::move(undo.try_pop_back()).value().state;
    }

    void try_update_model_system_and_state_with_rollback() noexcept {
        try {
            ms.on_model_modified();
        } catch (std::exception const& ex) {
            log::error("exception thrown when initializing updated model: %s", ex.what());
            log::error("attempting to rollback to earlier (pre-modification) version of the model");
            if (!undo.empty()) {
                do_undo();

                // NOTE: REDO now contains the broken model
                //
                // logically, REDO should be cleared right now. However, it can't be, because this
                // function call can happen midway through a drawcall. Parts of that drawcall might
                // still be holding onto pointers to the broken model.
                //
                // The "clean" solution to this is to throw an exception when realizePosition etc.
                // fail. However, that exception will unwind midway through a drawcall and end up
                // causing all kinds of state damage (e.g. ImGui will almost certainly be left in
                // invalid state)
                //
                // This "dirty" solution is to keep the broken model around until the frame has
                // finished drawing. That way, any stack-allocated pointers to the broken model
                // will be valid (as in, non-segfaulting) and we can *hopefully* limp along until
                // the drawcall is complete *and then* nuke the REDO container after the drawcall
                // when we know that there's no stale pointers hiding in a stack somewhere.
                recovered_from_disaster = true;
            } else {
                log::error("cannot perform undo: no undo history available: crashing application");
                std::terminate();
            }
        }
    }

    SimTK::State const& get_state() {
        return ms.state;
    }

    void set_model(std::unique_ptr<OpenSim::Model> model) {
        attempt_new_undo_push();
        ms = std::move(model);
    }

    Impl(std::unique_ptr<OpenSim::Model> _model) : ms{std::move(_model)} {
    }
};

static void draw_top_level_editor(Model_editor_screen::Impl& impl) {
    if (!impl.selection()) {
        ImGui::Text("cannot draw top level editor: nothing selected?");
        return;
    }

    OpenSim::Component& selection = *impl.selection();

    ImGui::Columns(2);

    ImGui::Text("name");
    ImGui::NextColumn();

    char nambuf[128];
    nambuf[sizeof(nambuf) - 1] = '\0';
    std::strncpy(nambuf, selection.getName().c_str(), sizeof(nambuf) - 1);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
    if (ImGui::InputText("##nameditor", nambuf, sizeof(nambuf), ImGuiInputTextFlags_EnterReturnsTrue)) {

        if (std::strlen(nambuf) > 0) {
            impl.before_modify_selection();
            selection.setName(nambuf);
            impl.after_modify_selection();
        }
    }
    ImGui::NextColumn();

    ImGui::Columns();
}

static void draw_frame_contextual_actions(Model_editor_screen::Impl& impl, OpenSim::PhysicalFrame& selection) {
    ImGui::Columns(2);

    ImGui::Text("geometry");
    ImGui::NextColumn();

    static constexpr char const* modal_name = "attach geometry";

    if (selection.getProperty_attached_geometry().empty()) {
        if (ImGui::Button("add geometry")) {
            ImGui::OpenPopup(modal_name);
        }
    } else {
        std::string name;
        if (selection.getProperty_attached_geometry().size() > 1) {
            name = "multiple";
        } else if (auto const* mesh = dynamic_cast<OpenSim::Mesh const*>(&selection.get_attached_geometry(0)); mesh) {
            name = mesh->get_mesh_file();
        } else {
            name = selection.get_attached_geometry(0).getConcreteClassName();
        }

        if (ImGui::Button(name.c_str())) {
            ImGui::OpenPopup(modal_name);
        }
    }

    auto on_mesh_add = [&impl, &selection](std::unique_ptr<OpenSim::Mesh> m) {
        impl.before_modify_selection();
        selection.updProperty_attached_geometry().clear();
        selection.attachGeometry(m.release());
        impl.after_modify_selection();
    };

    draw_attach_geom_modal_if_opened(impl.ui.attach_geometry_modal, modal_name, on_mesh_add);
    ImGui::NextColumn();

    ImGui::Text("offset frame");
    ImGui::NextColumn();
    if (ImGui::Button("add offset frame")) {
        auto pof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        pof->setName(selection.getName() + "_frame");
        pof->setParentFrame(selection);

        impl.before_modify_selection();
        selection.addComponent(pof.release());
        impl.after_modify_selection();
    }
    ImGui::NextColumn();

    ImGui::Columns(1);
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

static void draw_joint_type_switcher(Model_editor_screen::Impl& impl, OpenSim::Joint& selection) {
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

    ImGui::Text("joint type");
    ImGui::NextColumn();

    // look the Joint up in the type registry so we know where it should be in the ImGui::Combo
    std::optional<size_t> maybe_type_idx = joint::index_of(selection);
    int type_idx = maybe_type_idx ? static_cast<int>(*maybe_type_idx) : -1;

    auto known_joint_names = joint::names();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
    if (ImGui::Combo(
            "##newjointtypeselector",
            &type_idx,
            known_joint_names.data(),
            static_cast<int>(known_joint_names.size())) &&
        type_idx >= 0) {

        // copy + fixup  a prototype of the user's selection
        std::unique_ptr<OpenSim::Joint> new_joint{joint::prototypes()[static_cast<size_t>(type_idx)]->clone()};
        auto ptr = new_joint.get();
        copy_common_joint_properties(selection, *new_joint);

        // overwrite old joint in model
        impl.before_modify_model();
        const_cast<OpenSim::JointSet&>(js).set(idx, new_joint.release());
        impl.set_selection(ptr);
        impl.after_modify_model();
    }
    ImGui::NextColumn();
}

static void draw_joint_contextual_actions(Model_editor_screen::Impl& impl, OpenSim::Joint& selection) {

    ImGui::Columns(2);

    draw_joint_type_switcher(impl, selection);

    // BEWARE: broke
    {
        ImGui::Text("add offset frame");
        ImGui::NextColumn();

        if (ImGui::Button("parent")) {
            auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
            pf->setParentFrame(selection.getParentFrame());

            impl.before_modify_selection();
            selection.addFrame(pf.release());
            impl.after_modify_selection();
        }
        ImGui::SameLine();
        if (ImGui::Button("child")) {
            auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
            pf->setParentFrame(selection.getChildFrame());

            impl.before_modify_selection();
            selection.addFrame(pf.release());
            impl.after_modify_selection();
        }
        ImGui::NextColumn();
    }

    ImGui::Columns();
}

static void draw_hcf_contextual_actions(Model_editor_screen::Impl& impl, OpenSim::HuntCrossleyForce& selection) {
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

    // TODO: this is work in progress

    // the `geometry` property contains the *names* of contact geometry in the model that
    // the contact force should use.
    {
        OpenSim::Property<std::string> const& geoms = params.getGeometry();
        for (int i = 0; i < geoms.size(); ++i) {
            Property_editor_state st{};
        }
    }

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

        Properties_editor_state st{};
        draw_properties_editor_for_props_with_indices(
            st,
            params,
            easy_to_handle_props.data(),
            easy_to_handle_props.size(),
            [&impl]() { impl.before_modify_model(); },
            [&impl]() { impl.after_modify_model(); });
    }
}

static void draw_contextual_actions(Model_editor_screen::Impl& impl) {
    if (!impl.selection()) {
        ImGui::Text("cannot draw contextual actions: selection is blank (shouldn't be)");
        return;
    }

    ImGui::Columns(2);
    ImGui::Text("isolate in visualizer");
    ImGui::NextColumn();
    if (impl.selection() != impl.isolated()) {
        if (ImGui::Button("isolate")) {
            impl.set_isolated(impl.selection());
        }
    } else {
        if (ImGui::Button("un-isolate")) {
            impl.set_isolated(nullptr);
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

    if (auto* frame = dynamic_cast<OpenSim::PhysicalFrame*>(impl.selection()); frame) {
        draw_frame_contextual_actions(impl, *frame);
    } else if (auto* joint = dynamic_cast<OpenSim::Joint*>(impl.selection()); joint) {
        draw_joint_contextual_actions(impl, *joint);
    } else if (auto* hcf = dynamic_cast<OpenSim::HuntCrossleyForce*>(impl.selection()); hcf) {
        draw_hcf_contextual_actions(impl, *hcf);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.5f, 0.5f, 0.5f, 1.0f});
        ImGui::Text("    (OpenSim::%s has no contextual actions)", impl.selection()->getConcreteClassName().c_str());
        ImGui::PopStyleColor();
    }
}

static void draw_socket_editor(Model_editor_screen::Impl& impl) {

    if (!impl.selection()) {
        ImGui::Text("cannot draw socket editor: selection is blank (shouldn't be)");
        return;
    }
    OpenSim::Component& selection = *impl.selection();

    std::vector<std::string> socknames = selection.getSocketNames();

    if (socknames.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.5f, 0.5f, 0.5f, 1.0f});
        ImGui::Text("    (OpenSim::%s has no sockets)", impl.selection()->getConcreteClassName().c_str());
        ImGui::PopStyleColor();
        return;
    }

    ImGui::Columns(2);
    for (std::string const& sn : socknames) {
        ImGui::Text("%s", sn.c_str());
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

        auto on_conectee_change = [&](auto const& new_conectee) {
            impl.before_modify_selection();
            selection.updSocket(sn).connect(new_conectee);
            impl.after_modify_selection();
        };

        draw_reassign_socket_modal(
            impl.ui.reassign_socket, popupname.c_str(), impl.model(), socket, on_conectee_change);

        ImGui::NextColumn();
    }
    ImGui::Columns();
}

static void draw_selection_breadcrumbs(Model_editor_screen::Impl& impl) {
    if (!impl.selection()) {
        return;  // nothing selected
    }

    auto lst = osc::path_to(*impl.selection());

    if (lst.empty()) {
        return;  // this shouldn't happen, but you never know...
    }

    float indent = 0.0f;

    for (auto it = lst.begin(); it != lst.end() - 1; ++it) {
        ImGui::Dummy(ImVec2{indent, 0.0f});
        ImGui::SameLine();
        if (ImGui::Button((*it)->getName().c_str())) {
            impl.set_selection(const_cast<OpenSim::Component*>(*it));
        }
        if (ImGui::IsItemHovered()) {
            impl.set_hover(const_cast<OpenSim::Component*>(*it));
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::Text("OpenSim::%s", (*it)->getConcreteClassName().c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        indent += 15.0f;
    }

    ImGui::Dummy(ImVec2{indent, 0.0f});
    ImGui::SameLine();
    ImGui::Text("%s", (*(lst.end() - 1))->getName().c_str());
}

static void draw_selection_editor(Model_editor_screen::Impl& impl) {
    if (!impl.selection()) {
        ImGui::Text("(nothing selected)");
        return;
    }

    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    ImGui::Text("hierarchy:");
    ImGui::SameLine();
    draw_help_marker("Where the selected component is in the model's component hierarchy");
    ImGui::Separator();
    draw_selection_breadcrumbs(impl);

    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    ImGui::Text("top-level attributes:");
    ImGui::SameLine();
    draw_help_marker("Top-level properties on the OpenSim::Component itself");
    ImGui::Separator();
    draw_top_level_editor(impl);

    // contextual actions
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    ImGui::Text("contextual actions:");
    ImGui::SameLine();
    draw_help_marker("Actions that are specific to the type of OpenSim::Component that is currently selected");
    ImGui::Separator();
    draw_contextual_actions(impl);

    // a contextual action may have changed this
    if (!impl.selection()) {
        return;
    }

    // property editor
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    ImGui::Text("properties:");
    ImGui::SameLine();
    draw_help_marker(
        "Properties of the selected OpenSim::Component. These are declared in the Component's implementation.");
    ImGui::Separator();
    {
        auto before_property_edited = [&]() { impl.before_modify_selection(); };
        auto after_property_edited = [&]() { impl.after_modify_selection(); };
        draw_properties_editor(
            impl.ui.properties_editor, *impl.selection(), before_property_edited, after_property_edited);
    }

    // socket editor
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    ImGui::Text("sockets:");
    ImGui::SameLine();
    draw_help_marker(
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

static void action_delete_selection_from_model(osc::Model_editor_screen::Impl& impl) {
    OpenSim::Component* selected = impl.selection();

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

    // try delete OpenSim::Joint from owning OpenSim::JointSet
    if (auto* js = dynamic_cast<OpenSim::JointSet*>(owner); js) {
        impl.before_modify_model();
        delete_item_from_set_in_model(*js, static_cast<OpenSim::Joint*>(selected));
        impl.set_selection(nullptr);
        impl.set_hover(nullptr);
        impl.after_modify_model();
        return;
    }

    // try delete OpenSim::Body from owning OpenSim::BodySet
    if (auto* bs = dynamic_cast<OpenSim::BodySet*>(owner); bs) {
        log::error(
            "cannot delete %s: deleting OpenSim::Body is not supported: it segfaults in the OpenSim API",
            selected->getName().c_str());
        return;

        // segfaults:
        // delete_item_from_set_in_model(impl, *bs, static_cast<OpenSim::Body*>(selected));
    }

    // try delete OpenSim::WrapObject from owning OpenSim::WrapObjectSet
    if (auto* wos = dynamic_cast<OpenSim::WrapObjectSet*>(owner); wos) {

        log::error(
            "cannot delete %s: deleting an OpenSim::WrapObject is not supported: faults in the OpenSim API until after AK's connection checking addition",
            selected->getName().c_str());
        return;

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
    }

    // try delete Controller from ControllerSet
    if (auto* cs = dynamic_cast<OpenSim::ControllerSet*>(owner); cs) {
        impl.before_modify_model();
        delete_item_from_set_in_model(*cs, static_cast<OpenSim::Controller*>(selected));
        impl.set_selection(nullptr);
        impl.set_hover(nullptr);
        impl.after_modify_model();
        return;
    }

    // try delete Constraint from ConstraintSet
    if (auto* cs = dynamic_cast<OpenSim::ConstraintSet*>(owner); cs) {
        impl.before_modify_model();
        delete_item_from_set_in_model(*cs, static_cast<OpenSim::Constraint*>(selected));
        impl.set_selection(nullptr);
        impl.set_hover(nullptr);
        impl.after_modify_model();
        return;
    }

    // try delete Force from ForceSet
    if (auto* fs = dynamic_cast<OpenSim::ForceSet*>(owner); fs) {
        impl.before_modify_model();
        delete_item_from_set_in_model(*fs, static_cast<OpenSim::Force*>(selected));
        impl.set_selection(nullptr);
        impl.set_hover(nullptr);
        impl.after_modify_model();
        return;
    }

    // try delete Marker from MarkerSet
    if (auto* ms = dynamic_cast<OpenSim::MarkerSet*>(owner); ms) {
        impl.before_modify_model();
        delete_item_from_set_in_model(*ms, static_cast<OpenSim::Marker*>(selected));
        impl.set_selection(nullptr);
        impl.set_hover(nullptr);
        impl.after_modify_model();
        return;
    }

    // try delete Marker from MarkerSet
    if (auto* ms = dynamic_cast<OpenSim::MarkerSet*>(owner); ms) {
        impl.before_modify_model();
        delete_item_from_set_in_model(*ms, static_cast<OpenSim::Marker*>(selected));
        impl.set_selection(nullptr);
        impl.set_hover(nullptr);
        impl.after_modify_model();
        return;
    }

    // try delete ContactGeometry from ContactGeometrySet
    if (auto* cgs = dynamic_cast<OpenSim::ContactGeometrySet*>(owner); cgs) {
        impl.before_modify_model();
        delete_item_from_set_in_model(*cgs, static_cast<OpenSim::ContactGeometry*>(selected));
        impl.set_selection(nullptr);
        impl.set_hover(nullptr);
        impl.after_modify_model();
        return;
    }

    // try delete Probe from ProbeSet
    if (auto* ps = dynamic_cast<OpenSim::ProbeSet*>(owner); ps) {
        impl.before_modify_model();
        delete_item_from_set_in_model(*ps, static_cast<OpenSim::Probe*>(selected));
        impl.set_selection(nullptr);
        impl.set_hover(nullptr);
        impl.after_modify_model();
        return;
    }

    // try delete OpenSim::Geometry from owning OpenSim::Frame
    if (auto const* geom = find_ancestor<OpenSim::Geometry>(selected); geom) {
        // it's some child of geometry (probably a mesh), try and find its owning
        // frame and clear the frame of all geometry
        if (auto const* frame = find_ancestor<OpenSim::Frame>(geom); frame) {
            log::info("cannot delete one piece of geometry: must delete them all (API limitation)");
            impl.before_modify_model();
            const_cast<OpenSim::Frame*>(frame)->updProperty_attached_geometry().clear();
            impl.set_selection(nullptr);
            impl.after_modify_model();
        }

        return;
    }
}

static void action_undo(Model_editor_screen::Impl& impl) {
    if (!impl.can_undo()) {
        return;
    }
    impl.do_undo();
}

static void action_redo(Model_editor_screen::Impl& impl) {
    if (!impl.can_redo()) {
        return;
    }
    impl.do_redo();
}

static void action_switch_to_simulator(Model_editor_screen::Impl& impl) {
    auto copy = std::make_unique<OpenSim::Model>(impl.model());
    Application::current().request_screen_transition<Show_model_screen>(std::move(copy));
}

static void action_disable_all_wrapping_surfs(Model_editor_screen::Impl& impl) {
    OpenSim::Model& m = impl.model();
    impl.before_modify_model();
    for (OpenSim::WrapObjectSet& wos : m.updComponentList<OpenSim::WrapObjectSet>()) {
        for (int i = 0; i < wos.getSize(); ++i) {
            OpenSim::WrapObject& wo = wos[i];
            wo.set_active(false);
            wo.upd_Appearance().set_visible(false);
        }
    }
    impl.after_modify_model();
}

static void action_enable_all_wrapping_surfs(Model_editor_screen::Impl& impl) {
    OpenSim::Model& m = impl.model();
    impl.before_modify_model();
    for (OpenSim::WrapObjectSet& wos : m.updComponentList<OpenSim::WrapObjectSet>()) {
        for (int i = 0; i < wos.getSize(); ++i) {
            OpenSim::WrapObject& wo = wos[i];
            wo.set_active(true);
            wo.upd_Appearance().set_visible(true);
        }
    }
    impl.after_modify_model();
}

static void draw_main_menu_actions_tab(osc::Model_editor_screen::Impl& impl) {
    if (ImGui::BeginMenu("Edit")) {

        if (ImGui::MenuItem("Undo", "Ctrl+Z", false, impl.can_undo())) {
            action_undo(impl);
        }

        if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z", false, impl.can_redo())) {
            action_redo(impl);
        }

        if (ImGui::MenuItem("Switch to simulator", "Ctrl+R")) {
            action_switch_to_simulator(impl);
        }

        if (ImGui::MenuItem("Clear Selection", "Ctrl+A")) {
            impl.set_selection(nullptr);
        }

        if (ImGui::MenuItem("Show all components")) {
            impl.set_isolated(nullptr);
        }

        if (ImGui::BeginMenu("Utilities")) {
            if (ImGui::MenuItem("Disable all wrapping surfaces")) {
                action_disable_all_wrapping_surfs(impl);
            }

            if (ImGui::MenuItem("Enable all wrapping surfaces")) {
                action_enable_all_wrapping_surfs(impl);
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
                main_menu_save_as(impl.model());
                return true;
            case SDLK_z:
                action_redo(impl);
                return false;
            }
            return false;
        }

        switch (e.keysym.sym) {
        case SDLK_n:
            main_menu_new();
            return true;
        case SDLK_o:
            main_menu_open();
            return true;
        case SDLK_s:
            main_menu_save(impl.model());
            return true;
        case SDLK_q:
            Application::current().request_quit_application();
            return true;
        case SDLK_z:
            action_undo(impl);
            return true;
        case SDLK_r:
            action_switch_to_simulator(impl);
            return true;
        case SDLK_a:
            impl.set_selection(nullptr);
            return true;
        }

        return false;
    }

    switch (e.keysym.sym) {
    case SDLK_DELETE:
        action_delete_selection_from_model(impl);
        return true;
    case SDLK_F12:
        main_menu_save_as(impl.model());
        return true;
    }

    return false;
}

static void on_model_backing_file_changed(osc::Model_editor_screen::Impl& impl) {
    log::info("file change detected: loading updated file");
    std::unique_ptr<OpenSim::Model> p;
    try {
        p = std::make_unique<OpenSim::Model>(impl.model().getInputFileName());
        log::info("loaded updated file");
    } catch (std::exception const& ex) {
        log::error("error occurred while trying to automatically load a model file:");
        log::error(ex.what());
        log::error("the file will not be loaded into osc (you won't see the change in the UI)");
    }

    if (p) {
        impl.set_model(std::move(p));
    }
}

// Model_editor_screen public interface

Model_editor_screen::Model_editor_screen() : impl{new Impl{std::make_unique<OpenSim::Model>()}} {
}

Model_editor_screen::Model_editor_screen(std::unique_ptr<OpenSim::Model> _model) : impl{new Impl{std::move(_model)}} {
}

Model_editor_screen::~Model_editor_screen() noexcept {
    delete impl;
}

bool Model_editor_screen::on_event(SDL_Event const& e) {
    bool handled = false;

    if (e.type == SDL_KEYDOWN) {
        handled = on_keydown(*impl, e.key);
    }

    // if the screen didn't handle the event, forward it into the 3D viewer
    if (!handled) {
        handled = impl->model_viewer.on_event(e);
    }

    return handled;
}

void osc::Model_editor_screen::tick() {
    if (impl->file_poller.change_detected(impl->model().getInputFileName())) {
        on_model_backing_file_changed(*impl);
    }
}

void osc::Model_editor_screen::draw() {
    // draw main menu
    if (ImGui::BeginMainMenuBar()) {
        draw_main_menu_file_tab(impl->ui.main_menu_tab, &impl->model());
        draw_main_menu_actions_tab(*impl);
        draw_main_menu_about_tab();

        // colored "switch to simulator" menu button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.0f, 0.6f, 0.0f, 1.0f});
        if (ImGui::Button("Switch to simulator (Ctrl+R)")) {
            action_switch_to_simulator(*impl);
        }
        ImGui::PopStyleColor();

        ImGui::EndMainMenuBar();
    }

    // draw editor actions panel
    {
        auto on_set_selection = [&](OpenSim::Component* c) { impl->set_selection(c); };
        auto before_modify_model = [&]() { impl->before_modify_model(); };
        auto after_modify_model = [&]() { impl->after_modify_model(); };
        draw_model_actions_panel(
            impl->ui.model_actions_panel, impl->model(), on_set_selection, before_modify_model, after_modify_model);
    }

    // draw 3D model viewer
    {
        auto on_selection_change = [this](OpenSim::Component const* new_selection) {
            impl->set_selection(const_cast<OpenSim::Component*>(new_selection));
        };

        auto on_hover_change = [this](OpenSim::Component const* new_hover) {
            impl->set_hover(const_cast<OpenSim::Component*>(new_hover));
        };

        if (impl->isolated()) {
            impl->model_viewer.draw(
                "render",
                *impl->isolated(),
                impl->model().getDisplayHints(),
                impl->get_state(),
                impl->selection(),
                impl->hover(),
                on_selection_change,
                on_hover_change);
        } else {
            impl->model_viewer.draw(
                "render",
                impl->model(),
                impl->get_state(),
                impl->selection(),
                impl->hover(),
                on_selection_change,
                on_hover_change);
        }
    }

    // draw model hierarchy viewer
    if (ImGui::Begin("Hierarchy")) {
        auto on_select = [this](auto const* c) { impl->set_selection(const_cast<OpenSim::Component*>(c)); };
        auto on_hover = [this](auto const* c) { impl->set_hover(const_cast<OpenSim::Component*>(c)); };
        draw_component_hierarchy_widget(
            &impl->model().getRoot(), impl->selection(), impl->hover(), on_select, on_hover);
    }
    ImGui::End();

    // draw selection viewer
    if (ImGui::Begin("Selection")) {
        auto on_selection_changed = [this](auto const* c) { impl->set_selection(const_cast<OpenSim::Component*>(c)); };
        draw_component_selection_widget(impl->get_state(), impl->selection(), on_selection_changed);
    }
    ImGui::End();

    // draw property editor panel
    if (ImGui::Begin("Edit Props")) {
        draw_selection_editor(*impl);
    }
    ImGui::End();

    // draw log viewer
    draw_log_viewer_widget(impl->ui.log_viewer, "Log");

    if (impl->recovered_from_disaster) {
        impl->redo.clear();
        impl->recovered_from_disaster = false;
    }
}
