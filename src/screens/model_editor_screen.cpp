#include "model_editor_screen.hpp"

#include "src/3d/gpu_cache.hpp"
#include "src/application.hpp"
#include "src/config.hpp"
#include "src/log.hpp"
#include "src/opensim_bindings/fd_simulation.hpp"
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
using namespace osmv;
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

namespace {
    // bundles together:
    //
    // - model
    // - state
    // - selection
    // - hover
    //
    // into a single class that supports coherent copying, moving, assingment etc.
    //
    // enables snapshotting everything necessary to render a typical UI scene (just copy this) and
    // should automatically update/invalidate any pointers, states, etc. on each operation
    struct Model_ui_state final {
        static std::unique_ptr<OpenSim::Model> copy_model(OpenSim::Model const& model) {
            auto copy = std::make_unique<OpenSim::Model>(model);
            copy->finalizeFromProperties();
            return copy;
        }

        static SimTK::State init_fresh_system_and_state(OpenSim::Model& model) {
            SimTK::State rv = model.initSystem();
            model.realizePosition(rv);
            return rv;
        }

        // relocate a component pointer to point into `model`, rather into whatever
        // model it used to point into
        static OpenSim::Component* relocate_pointer(OpenSim::Model const& model, OpenSim::Component* ptr) {
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

        Model_ui_state() = default;

        Model_ui_state(std::unique_ptr<OpenSim::Model> _model) :
            model{std::move(_model)},
            state{init_fresh_system_and_state(*model)},
            selected_component{nullptr},
            hovered_component{nullptr} {
        }

        Model_ui_state(Model_ui_state const& other) :
            model{copy_model(*other.model)},
            state{init_fresh_system_and_state(*model)},
            selected_component{relocate_pointer(*model, other.selected_component)},
            hovered_component{relocate_pointer(*model, other.hovered_component)} {
        }

        Model_ui_state(Model_ui_state&& tmp) :
            model{std::move(tmp.model)},
            state{std::move(tmp.state)},
            selected_component{std::move(tmp.selected_component)},
            hovered_component{std::move(tmp.hovered_component)} {
        }

        friend void swap(Model_ui_state& a, Model_ui_state& b) {
            std::swap(a.model, b.model);
            std::swap(a.state, b.state);
            std::swap(a.selected_component, b.selected_component);
            std::swap(a.hovered_component, b.hovered_component);
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
            selected_component = relocate_pointer(*model, selected_component);
            hovered_component = relocate_pointer(*model, hovered_component);

            return *this;
        }

        // beware: can throw, because the model might've been put into an invalid
        // state by the modification
        void on_model_modified() {
            state = init_fresh_system_and_state(*model);
            selected_component = relocate_pointer(*model, selected_component);
            hovered_component = relocate_pointer(*model, hovered_component);
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
}

struct Model_editor_screen::Impl final {
private:
    Model_ui_state ms;
    Gpu_cache gpu_cache;
    Circular_buffer<Undo_redo_entry, 32> undo;
    Circular_buffer<Undo_redo_entry, 32> redo;
    bool recovered_from_disaster = false;

public:
    Model_viewer_widget model_viewer{gpu_cache, ModelViewerWidgetFlags_Default | ModelViewerWidgetFlags_DrawFrames};

    struct Panels {
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
        Log_viewer_widget_state log_viewer;
        Select_2_pfs_modal_state select_2_pfs;
        Model_actions_panel_state model_actions_panel;
    } ui;

    File_change_poller file_poller{1000ms, ""};

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

    [[nodiscard]] OpenSim::Component* selection() noexcept {
        return ms.selected_component;
    }

    [[nodiscard]] OpenSim::Component* hover() noexcept {
        return ms.hovered_component;
    }

    void set_hover(OpenSim::Component* c) {
        ms.hovered_component = c;
    }

    void perform_end_of_draw_steps() {
        if (recovered_from_disaster) {
            redo.clear();
        }
    }

    [[nodiscard]] bool can_undo() const noexcept {
        return !undo.empty();
    }

    [[nodiscard]] bool can_redo() const noexcept {
        return !redo.empty();
    }

    void attempt_new_undo_push() {
        auto now = std::chrono::system_clock::now();

        // debounce pushing undos to prevent it from being populated with hundreds of tiny
        // edits that can occur when (e.g.) a user is playing with a slider - the user's
        // probably still happy if they can reverse to <= `debounce_time` ago

        static constexpr std::chrono::seconds debounce = 5s;
        bool should_undo = undo.empty() || undo.back().time + debounce <= now;

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

static void draw_joint_contextual_actions(Model_editor_screen::Impl& impl, OpenSim::Joint& selection) {

    ImGui::Columns(2);

    if (auto const* jsp = dynamic_cast<OpenSim::JointSet const*>(&selection.getOwner()); jsp) {
        int idx = -1;
        for (int i = 0; i < jsp->getSize(); ++i) {
            if (&(*jsp)[i] == &selection) {
                idx = i;
                break;
            }
        }

        if (idx >= 0) {
            ImVec4 enabled_color{0.1f, 0.6f, 0.1f, 1.0f};
            auto const& joint_tid = typeid(selection);

            ImGui::Text("change joint type");
            ImGui::NextColumn();

            std::unique_ptr<OpenSim::Joint> added_joint = nullptr;

            {
                int pushed_styles = 0;
                if (joint_tid == typeid(OpenSim::FreeJoint)) {
                    ImGui::PushStyleColor(ImGuiCol_Button, enabled_color);
                    ++pushed_styles;
                }

                if (ImGui::Button("fj")) {
                    added_joint.reset(new OpenSim::FreeJoint{});
                }

                ImGui::PopStyleColor(pushed_styles);
            }

            ImGui::SameLine();
            {
                int pushed_styles = 0;
                if (joint_tid == typeid(OpenSim::PinJoint)) {
                    ImGui::PushStyleColor(ImGuiCol_Button, enabled_color);
                    ++pushed_styles;
                }

                if (ImGui::Button("pj")) {
                    added_joint.reset(new OpenSim::PinJoint{});
                }

                ImGui::PopStyleColor(pushed_styles);
            }

            ImGui::SameLine();
            {
                int pushed_styles = 0;
                if (joint_tid == typeid(OpenSim::UniversalJoint)) {
                    ImGui::PushStyleColor(ImGuiCol_Button, enabled_color);
                    ++pushed_styles;
                }

                if (ImGui::Button("uj")) {
                    added_joint.reset(new OpenSim::UniversalJoint{});
                }

                ImGui::PopStyleColor(pushed_styles);
            }

            ImGui::SameLine();
            {
                int pushed_styles = 0;
                if (joint_tid == typeid(OpenSim::BallJoint)) {
                    ImGui::PushStyleColor(ImGuiCol_Button, enabled_color);
                    ++pushed_styles;
                }

                if (ImGui::Button("bj")) {
                    added_joint.reset(new OpenSim::BallJoint{});
                }

                ImGui::PopStyleColor(pushed_styles);
            }

            if (added_joint) {
                copy_common_joint_properties(selection, *added_joint);

                impl.before_modify_model();
                impl.set_selection(added_joint.get());
                const_cast<OpenSim::JointSet*>(jsp)->set(idx, added_joint.release());
                impl.after_modify_model();
            }

            ImGui::NextColumn();
        }
    }

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

    if (auto* frame = dynamic_cast<OpenSim::PhysicalFrame*>(impl.selection()); frame) {
        draw_frame_contextual_actions(impl, *frame);
    } else if (auto* joint = dynamic_cast<OpenSim::Joint*>(impl.selection()); joint) {
        draw_joint_contextual_actions(impl, *joint);
    } else if (auto* hcf = dynamic_cast<OpenSim::HuntCrossleyForce*>(impl.selection()); hcf) {
        draw_hcf_contextual_actions(impl, *hcf);
    }
}

static void draw_socket_editor(Model_editor_screen::Impl& impl) {

    if (!impl.selection()) {
        ImGui::Text("cannot draw socket editor: selection is blank (shouldn't be)");
        return;
    }
    OpenSim::Component& selection = *impl.selection();

    std::vector<std::string> socknames = selection.getSocketNames();
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

static void draw_selection_editor(Model_editor_screen::Impl& impl) {
    if (!impl.selection()) {
        ImGui::Text("(nothing selected)");
        return;
    }
    OpenSim::Component& selection = *impl.selection();

    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    ImGui::Text("top-level attributes:");
    ImGui::Separator();
    draw_top_level_editor(impl);

    // contextual actions
    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    ImGui::Text("contextual actions:");
    ImGui::Separator();
    draw_contextual_actions(impl);

    // property editor
    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    ImGui::Text("properties:");
    ImGui::Separator();
    {
        auto before_property_edited = [&]() { impl.before_modify_selection(); };
        auto after_property_edited = [&]() { impl.after_modify_selection(); };
        draw_properties_editor(impl.ui.properties_editor, selection, before_property_edited, after_property_edited);
    }

    // socket editor
    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    ImGui::Text("sockets:");
    ImGui::Separator();
    draw_socket_editor(impl);
}

static void on_delete_selection(osmv::Model_editor_screen::Impl& impl) {
    OpenSim::Component* selected = impl.selection();

    if (!selected) {
        return;
    }

    if (auto const* geom = find_ancestor<OpenSim::Geometry>(selected); geom) {
        // it's some child of geometry (probably a mesh), try and find its owning
        // frame and clear the frame of all geometry
        if (auto const* frame = find_ancestor<OpenSim::Frame>(geom); frame) {
            impl.before_modify_model();
            const_cast<OpenSim::Frame*>(frame)->updProperty_attached_geometry().clear();
            impl.set_selection(nullptr);
            impl.after_modify_model();
        }
    } else if (auto* joint = dynamic_cast<OpenSim::Joint*>(selected); joint) {
        if (auto const* jointset = dynamic_cast<OpenSim::JointSet const*>(&joint->getOwner()); jointset) {
            for (int i = 0; i < jointset->getSize(); ++i) {
                if (&jointset->get(i) == joint) {

                    impl.before_modify_model();
                    const_cast<OpenSim::JointSet*>(jointset)->remove(i);
                    impl.model().finalizeFromProperties();
                    impl.set_selection(nullptr);
                    impl.after_modify_model();
                    break;
                }
            }
        }
    }
}

static void main_menu_undo(Model_editor_screen::Impl& impl) {
    if (!impl.can_undo()) {
        return;
    }
    impl.do_undo();
}

static void main_menu_redo(Model_editor_screen::Impl& impl) {
    if (!impl.can_redo()) {
        return;
    }
    impl.do_redo();
}

static void draw_main_menu_edit_tab(osmv::Model_editor_screen::Impl& impl) {
    if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Undo", "Ctrl+Z", false, impl.can_undo())) {
            main_menu_undo(impl);
        }

        if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z", false, impl.can_redo())) {
            main_menu_redo(impl);
        }
        ImGui::EndMenu();
    }
}

static bool on_keydown(osmv::Model_editor_screen::Impl& impl, SDL_KeyboardEvent const& e) {
    if (e.keysym.mod & KMOD_CTRL) {
        // CTRL

        if (e.keysym.mod & KMOD_SHIFT) {
            // CTRL+SHIFT

            switch (e.keysym.sym) {
            case SDLK_s:
                main_menu_save_as(impl.model());
                return true;
            case SDLK_z:
                main_menu_redo(impl);
                return true;
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
            // UNSAFE because saving a model may modify its document name, which
            //        isn't a mutation the abstraction should care about (for now)
            main_menu_save(impl.model());
            return true;
        case SDLK_q:
            Application::current().request_quit_application();
            return true;
        case SDLK_z:
            main_menu_undo(impl);
            return true;
        }

        return false;
    }

    switch (e.keysym.sym) {
    case SDLK_DELETE:
        on_delete_selection(impl);
        return true;
    case SDLK_F12:
        // UNSAFE because saving a model may modify its document name, which
        //        isn't a mutation the abstraction should care about (for now)
        main_menu_save_as(impl.model());
        return true;
    }

    return false;
}

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

    return handled ? true : impl->model_viewer.on_event(e);
}

void osmv::Model_editor_screen::tick() {
    // auto update from file: check for changes in the underlying file, reloading
    // the UI if a change is detected

    auto const& fname = impl->model().getInputFileName();
    if (impl->file_poller.change_detected(fname)) {
        log::info("file change detected: loading updated file");
        std::unique_ptr<OpenSim::Model> p;
        try {
            p = std::make_unique<OpenSim::Model>(fname);
            log::info("loaded updated file");
        } catch (std::exception const& ex) {
            log::error("error occurred while trying to automatically load a model file:");
            log::error(ex.what());
            log::error("the file will not be loaded into osmv (you won't see the change in the UI)");
        }

        if (p) {
            impl->set_model(std::move(p));
        }
    }
}

void osmv::Model_editor_screen::draw() {
    // draw main menu
    if (ImGui::BeginMainMenuBar()) {
        draw_main_menu_file_tab(impl->ui.main_menu_tab, &impl->model());
        draw_main_menu_edit_tab(*impl);
        draw_main_menu_about_tab();
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

    // draw model viewer
    {
        auto on_selection_change = [this](OpenSim::Component const* new_selection) {
            impl->set_selection(const_cast<OpenSim::Component*>(new_selection));
        };

        auto on_hover_change = [this](OpenSim::Component const* new_hover) {
            impl->set_hover(const_cast<OpenSim::Component*>(new_hover));
        };

        impl->model_viewer.draw(
            "render",
            impl->model(),
            impl->get_state(),
            impl->selection(),
            impl->hover(),
            on_selection_change,
            on_hover_change);
    }

    // draw hierarchy viewer
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

    // draw log panel
    draw_log_viewer_widget(impl->ui.log_viewer, "Log");
}
