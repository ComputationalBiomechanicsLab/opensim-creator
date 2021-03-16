#include "model_editor_screen.hpp"

#include "src/3d/gpu_cache.hpp"
#include "src/application.hpp"
#include "src/config.hpp"
#include "src/log.hpp"
#include "src/opensim_bindings/fd_simulation.hpp"
#include "src/screens/show_model_screen.hpp"
#include "src/screens/splash_screen.hpp"
#include "src/utils/circular_buffer.hpp"
#include "src/utils/indirect_ptr.hpp"
#include "src/utils/indirect_ref.hpp"
#include "src/utils/scope_guard.hpp"
#include "src/utils/sdl_wrapper.hpp"
#include "src/widgets/add_body_modal.hpp"
#include "src/widgets/add_joint_modal.hpp"
#include "src/widgets/attach_geometry_modal.hpp"
#include "src/widgets/component_hierarchy_widget.hpp"
#include "src/widgets/component_selection_widget.hpp"
#include "src/widgets/log_viewer_widget.hpp"
#include "src/widgets/model_viewer_widget.hpp"
#include "src/widgets/properties_editor.hpp"
#include "src/widgets/reassign_socket_modal.hpp"

#include <OpenSim/Common/AbstractProperty.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ModelDisplayHints.h>
#include <OpenSim/Common/Property.h>
#include <OpenSim/Common/PropertyObjArray.h>
#include <OpenSim/Common/Set.h>
#include <OpenSim/Simulation/Model/BodySet.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/JointSet.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/BallJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/UniversalJoint.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SimTKcommon.h>
#include <SimTKcommon/Mechanics.h>
#include <SimTKcommon/SmallMatrix.h>
#include <imgui.h>
#include <nfd.h>

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

template<typename T>
static bool has_ancestor(OpenSim::Component const* c) {
    return find_ancestor<T>(c) != nullptr;
}

static bool filename_lexographically_gt(fs::path const& a, fs::path const& b) {
    return a.filename() < b.filename();
}

static std::vector<fs::path> find_all_vtp_resources() {
    fs::path geometry_dir = config::resource_path("geometry");

    std::vector<fs::path> rv;

    if (not fs::exists(geometry_dir)) {
        // application installation is probably mis-configured, or missing
        // the geometry dir (e.g. the user deleted it)
        return rv;
    }

    if (not fs::is_directory(geometry_dir)) {
        // something horrible has happened, such as the user creating a file
        // called "geometry" in the application resources dir. Silently eat
        // this for now
        return rv;
    }

    // ensure the number of files iterated over does not exeed some (arbitrary)
    // limit to protect the application from the edge-case that the implementation
    // selects (e.g.) a root directory and ends up recursing over the entire
    // filesystem
    int i = 0;
    static constexpr int file_limit = 10000;

    for (fs::directory_entry const& entry : fs::recursive_directory_iterator{geometry_dir}) {
        if (i++ > file_limit) {
            // TODO: log warning
            return rv;
        }

        if (entry.path().extension() == ".vtp") {
            rv.push_back(entry.path());
        }
    }

    std::sort(rv.begin(), rv.end(), filename_lexographically_gt);

    return rv;
}

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

    static SimTK::State init_fresh_state(OpenSim::Model& model) {
        SimTK::State rv = model.initSystem();
        model.realizePosition(rv);
        return rv;
    }

    // relocate a component pointer to point into `model`, rather into whatever
    // model it used to point into
    static OpenSim::Component* relocate_pointer(OpenSim::Model const& model, OpenSim::Component* ptr) {
        if (not ptr) {
            return nullptr;
        }

        return const_cast<OpenSim::Component*>(model.findComponent(ptr->getAbsolutePath()));
    }

    std::unique_ptr<OpenSim::Model> model;
    SimTK::State state;
    OpenSim::Component* selected_component;
    OpenSim::Component* hovered_component;

    Model_ui_state() : model{nullptr}, state{SimTK::State{}}, selected_component{nullptr}, hovered_component{nullptr} {
    }

    Model_ui_state(std::unique_ptr<OpenSim::Model> _model) :
        model{std::move(_model)},
        state{init_fresh_state(*model)},
        selected_component{nullptr},
        hovered_component{nullptr} {
    }

    Model_ui_state(Model_ui_state const& other) :
        model{copy_model(*other.model)},
        state{init_fresh_state(*model)},
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
        std::swap(a.selected_component, b.selected_component);
        std::swap(a.hovered_component, b.hovered_component);
        std::swap(a.state, b.state);
    }

    Model_ui_state& operator=(Model_ui_state other) {
        swap(*this, other);
        return *this;
    }

    void update_state() {
        state = init_fresh_state(*model);
    }
};

template<typename T>
struct Timestamped final {
    std::chrono::system_clock::time_point t;
    T v;

    Timestamped() = default;

    template<typename... Args>
    Timestamped(std::chrono::system_clock::time_point _t, Args&&... args) :
        t{std::move(_t)},
        v{std::forward<Args>(args)...} {
    }
};

struct Model_editor_screen::Impl final {
private:
    std::vector<fs::path> available_vtps = find_all_vtp_resources();
    Model_ui_state ms;
    Gpu_cache gpu_cache;
    Circular_buffer<Timestamped<Model_ui_state>, 32> undo;
    Circular_buffer<Timestamped<Model_ui_state>, 32> redo;
    bool recovered_from_disaster = false;

    struct Selection_ptr final : public Indirect_ptr<OpenSim::Component> {
        Impl& parent;
        Selection_ptr(Impl& _parent) : parent{_parent} {
        }

    private:
        OpenSim::Component* impl_upd() override {
            return parent.ms.selected_component;
        }

        void impl_set(OpenSim::Component* c) override {
            parent.ms.selected_component = c;
        }

        void on_begin_modify() override {
            if (parent.ms.selected_component) {
                log::info("selection on_begin_modify (name = %s)", parent.ms.selected_component->getName().c_str());
                parent.attempt_new_undo_push();
            }
        }

        void on_end_modify() noexcept override {
            if (parent.ms.selected_component) {
                log::info("selection on_end_modify (name = %s)", parent.ms.selected_component->getName().c_str());
                parent.try_update_model_system_and_state_with_rollback();
            }
        }
    } selection_ptr{*this};

    struct Hover_ptr final : public Indirect_ptr<OpenSim::Component> {
        Impl& parent;
        Hover_ptr(Impl& _parent) : parent{_parent} {
        }

    private:
        OpenSim::Component* impl_upd() override {
            return parent.ms.hovered_component;
        }

        void impl_set(OpenSim::Component* c) override {
            parent.ms.hovered_component = c;
        }

        void on_begin_modify() override {
            if (parent.ms.hovered_component) {
                log::info("hover on_begin_modify (name = %s)", parent.ms.hovered_component->getName().c_str());
                parent.attempt_new_undo_push();
            }
        }

        void on_end_modify() noexcept override {
            if (parent.ms.hovered_component) {
                log::info("hover on_end_modify (name = %s)", parent.ms.hovered_component->getName().c_str());
                parent.try_update_model_system_and_state_with_rollback();
            }
        }
    } hover_ptr{*this};

    struct Model_ref final : public Indirect_ref<OpenSim::Model> {
        Impl& parent;
        Model_ref(Impl& _parent) : parent{_parent} {
        }

    private:
        OpenSim::Model& impl_upd() override {
            return *parent.ms.model;
        }

        void on_begin_modify() override {
            log::info("start model modification");
            parent.attempt_new_undo_push();
        }

        void on_end_modify() noexcept override {
            log::info("end model modification");
            parent.try_update_model_system_and_state_with_rollback();
        }
    } model_ref{*this};

public:
    Model_viewer_widget model_viewer{gpu_cache, ModelViewerWidgetFlags_Default | ModelViewerWidgetFlags_DrawFrames};

    struct {
        Added_body_modal_state abm;
        std::array<Add_joint_modal, 4> add_joint_modals = {
            Add_joint_modal::create<OpenSim::FreeJoint>("Add FreeJoint"),
            Add_joint_modal::create<OpenSim::PinJoint>("Add PinJoint"),
            Add_joint_modal::create<OpenSim::UniversalJoint>("Add UniversalJoint"),
            Add_joint_modal::create<OpenSim::BallJoint>("Add BallJoint")};
        Properties_editor properties_editor;
        Reassign_socket_modal reassign_socket;
        Attach_geometry_modal attach_geometry_modal;
    } ui;

    [[nodiscard]] Indirect_ptr<OpenSim::Component>& selection() noexcept {
        return selection_ptr;
    }

    [[nodiscard]] Indirect_ptr<OpenSim::Component>& hover() noexcept {
        return hover_ptr;
    }

    [[nodiscard]] Indirect_ref<OpenSim::Model>& model() noexcept {
        return model_ref;
    }

    void perform_end_of_draw_steps() {
        if (recovered_from_disaster) {
            redo.clear();
        }
    }

    [[nodiscard]] bool can_undo() const noexcept {
        return not undo.empty();
    }

    [[nodiscard]] bool can_redo() const noexcept {
        return not redo.empty();
    }

    void attempt_new_undo_push() {
        auto now = std::chrono::system_clock::now();

        if (not undo.empty() and undo.back().t + 5s > now) {
            return;  // too temporally close to previous push
        }

        undo.emplace_back(now, ms);
        redo.clear();
    }

    void do_redo() {
        if (redo.empty()) {
            return;
        }

        undo.emplace_back(std::chrono::system_clock::now(), std::move(ms));
        ms = std::move(redo.try_pop_back()).value().v;
    }

    void do_undo() {
        if (undo.empty()) {
            return;
        }

        redo.emplace_back(std::chrono::system_clock::now(), std::move(ms));
        ms = std::move(undo.try_pop_back()).value().v;
    }

    void try_update_model_system_and_state_with_rollback() noexcept {
        try {
            ms.update_state();
        } catch (std::exception const& ex) {
            log::error("exception thrown when initializing updated model: %s", ex.what());
            log::error("attempting to rollback to earlier (pre-modification) version of the model");
            if (not undo.empty()) {
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

    std::vector<fs::path> const& vtps() {
        return available_vtps;
    }

    Impl(std::unique_ptr<OpenSim::Model> _model) : ms{std::move(_model)} {
    }
};

static void draw_top_level_editor(Indirect_ptr<OpenSim::Component>& selection) {
    if (not selection) {
        ImGui::Text("cannot draw top level editor: nothing selected?");
        return;
    }

    ImGui::Columns(2);

    ImGui::Text("name");
    ImGui::NextColumn();

    char nambuf[128];
    nambuf[sizeof(nambuf) - 1] = '\0';
    std::strncpy(nambuf, selection->getName().c_str(), sizeof(nambuf) - 1);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
    if (ImGui::InputText("##nameditor", nambuf, sizeof(nambuf), ImGuiInputTextFlags_EnterReturnsTrue)) {

        if (std::strlen(nambuf) > 0) {
            auto guard = selection.modify();
            guard->setName(nambuf);
        }
    }
    ImGui::NextColumn();

    ImGui::Columns();
}

static void draw_frame_contextual_actions(
    Attach_geometry_modal& modal, std::vector<fs::path> const& vtps, Indirect_ptr<OpenSim::PhysicalFrame>& frame) {

    ImGui::Columns(2);

    ImGui::Text("geometry");
    ImGui::NextColumn();

    if (frame->getProperty_attached_geometry().empty()) {
        if (ImGui::Button("add geometry")) {
            modal.show();
        }
    } else {
        std::string name;
        if (frame->getProperty_attached_geometry().size() > 1) {
            name = "multiple";
        } else if (auto const* mesh = dynamic_cast<OpenSim::Mesh const*>(&frame->get_attached_geometry(0)); mesh) {
            name = mesh->get_mesh_file();
        } else {
            name = frame->get_attached_geometry(0).getConcreteClassName();
        }

        if (ImGui::Button(name.c_str())) {
            modal.show();
        }
    }
    modal.draw(vtps, frame);
    ImGui::NextColumn();

    ImGui::Text("offset frame");
    ImGui::NextColumn();
    if (ImGui::Button("add offset frame")) {
        auto pof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        pof->setParentFrame(*frame);
        frame.modify()->addComponent(pof.release());
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
        if (not parent_assigned) {
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
        if (not child_assigned) {
            // the source's child is a reference to some frame that the source
            // doesn't, itself, own, so the destination should just also refer
            // to the same (not-owned) frame
            dest.connectSocket_child_frame(src_child);
        }
    }
}

static void
    draw_joint_contextual_actions(Indirect_ref<OpenSim::Model>& model, Indirect_ptr<OpenSim::Joint>& selection) {
    if (not selection) {
        ImGui::Text("cannot draw contextual actions: nothing selected");
        return;
    }

    ImGui::Columns(2);

    if (auto const* jsp = dynamic_cast<OpenSim::JointSet const*>(&selection->getOwner()); jsp) {
        int idx = -1;
        for (int i = 0; i < jsp->getSize(); ++i) {
            if (&(*jsp)[i] == selection) {
                idx = i;
                break;
            }
        }

        if (idx >= 0) {
            auto const& joint_tid = typeid(selection.get());

            ImGui::Text("change joint type");
            ImGui::NextColumn();

            std::unique_ptr<OpenSim::Joint> added_joint = nullptr;

            {
                int pushed_styles = 0;
                if (joint_tid == typeid(OpenSim::FreeJoint)) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.0f, 1.0f, 0.0f, 1.0f});
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
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.0f, 1.0f, 0.0f, 1.0f});
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
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.0f, 1.0f, 0.0f, 1.0f});
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
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.0f, 1.0f, 0.0f, 1.0f});
                    ++pushed_styles;
                }

                if (ImGui::Button("bj")) {
                    added_joint.reset(new OpenSim::BallJoint{});
                }

                ImGui::PopStyleColor(pushed_styles);
            }

            if (added_joint) {
                copy_common_joint_properties(*selection, *added_joint);
                selection.reset(added_joint.get());

                // `const_cast` necessary because jsp is extracted via the `getParent` call in
                // OpenSim
                const_cast<OpenSim::JointSet*>(jsp)->set(idx, added_joint.release());

                // UNSAFE because we have effectively modified *the model* by hopping up to the
                // selection's parent: really, this modification should happen under a model guard
                // (e.g. by traversing from the root model the JointSet under a guard)
                model.UNSAFE_on_begin_modify();
                model.UNSAFE_on_end_modify();
            }

            ImGui::NextColumn();
        }
    }

    ImGui::Text("add offset frame");
    ImGui::NextColumn();

    if (ImGui::Button("parent")) {
        auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        pf->setParentFrame(selection->getParentFrame());
        auto guard = selection.modify();
        guard->addFrame(pf.release());
    }
    ImGui::SameLine();
    if (ImGui::Button("child")) {
        auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        pf->setParentFrame(selection->getChildFrame());
        auto guard = selection.modify();
        guard->addFrame(pf.release());
    }
    ImGui::NextColumn();

    ImGui::Columns();
}

static void draw_contextual_actions(osmv::Model_editor_screen::Impl& impl) {
    if (not impl.selection()) {
        ImGui::Text("cannot draw contextual actions: selection is blank (shouldn't be)");
        return;
    }

    if (auto frame = try_downcast<OpenSim::PhysicalFrame>(impl.selection()); frame) {
        draw_frame_contextual_actions(impl.ui.attach_geometry_modal, impl.vtps(), *frame);
    } else if (auto joint = try_downcast<OpenSim::Joint>(impl.selection()); joint) {
        draw_joint_contextual_actions(impl.model(), *joint);
    }
}

static void draw_socket_editor(
    Reassign_socket_modal& modal, Indirect_ref<OpenSim::Model>& model, Indirect_ptr<OpenSim::Component>& selection) {

    if (not selection) {
        ImGui::Text("cannot draw socket editor: selection is blank (shouldn't be)");
        return;
    }

    // this UNSAFE is because I want to get the socket names, which is a non-const method on
    // the `selection`, but doesn't actually modify the selection (much, at least...)
    std::vector<std::string> socknames = selection.UNSAFE_upd()->getSocketNames();
    ImGui::Columns(2);
    for (std::string const& sn : socknames) {
        ImGui::Text("%s", sn.c_str());
        ImGui::NextColumn();

        OpenSim::AbstractSocket const& socket = selection->getSocket(sn);
        std::string sockname = socket.getConnecteePath();
        std::string popupname = std::string{"reassign"} + sockname;

        if (ImGui::Button(sockname.c_str())) {
            modal.show(popupname.c_str());
        }

        // these `const_cast`s are necessary because there isn't (currently) a clean way of
        // expressing that the modification is happening to a subcomponent (it would require
        // a lens/projection proxy that I haven't developed yet ;))
        auto getter = [&socket]() { return std::ref(const_cast<OpenSim::AbstractSocket&>(socket)); };

        // this UNSAFE is because we are notifying that the selection was changed (because a child
        // socket in the selection was changed)
        auto on_socket_modification = [&selection]() {
            selection.UNSAFE_on_begin_modify();
            selection.UNSAFE_on_begin_modify();
        };

        auto ref = Lambda_indirect_ref<OpenSim::AbstractSocket, decltype(getter), decltype(on_socket_modification)>{
            getter, on_socket_modification};

        modal.draw(popupname.c_str(), model, ref);

        ImGui::NextColumn();
    }
    ImGui::Columns();
}

static void draw_selection_editor(Model_editor_screen::Impl& impl) {
    if (not impl.selection()) {
        ImGui::Text("(nothing selected)");
        return;
    }

    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    ImGui::Text("top-level attributes:");
    ImGui::Separator();
    draw_top_level_editor(impl.selection());

    // contextual actions
    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    ImGui::Text("contextual actions:");
    ImGui::Separator();
    draw_contextual_actions(impl);

    // property editor
    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    ImGui::Text("properties:");
    ImGui::Separator();
    impl.ui.properties_editor.draw(impl.selection());

    // socket editor
    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    ImGui::Text("sockets:");
    ImGui::Separator();
    draw_socket_editor(impl.ui.reassign_socket, impl.model(), impl.selection());
}

static std::optional<std::filesystem::path> prompt_save_single_file() {
    nfdchar_t* outpath = nullptr;
    nfdresult_t result = NFD_SaveDialog("osim", nullptr, &outpath);
    OSMV_SCOPE_GUARD_IF(outpath != nullptr, { free(outpath); });

    return result == NFD_OKAY ? std::optional{std::string{outpath}} : std::nullopt;
}

static bool is_subpath(std::filesystem::path const& dir, std::filesystem::path const& pth) {
    auto dir_n = std::distance(dir.begin(), dir.end());
    auto pth_n = std::distance(pth.begin(), pth.end());

    if (pth_n < dir_n) {
        return false;
    }

    return std::equal(dir.begin(), dir.end(), pth.begin());
}

static bool is_example_file(std::filesystem::path const& path) {
    std::filesystem::path examples_dir = config::resource_path("models");
    return is_subpath(examples_dir, path);
}

template<typename T, typename MappingFunction>
static auto map_optional(MappingFunction f, std::optional<T> opt)
    -> std::optional<decltype(f(std::move(opt).value()))> {

    return opt ? std::optional{f(std::move(opt).value())} : std::nullopt;
}

static std::string path2string(std::filesystem::path p) {
    return p.string();
}

static std::optional<std::string> try_get_save_location(OpenSim::Model const& m) {

    if (std::string const& backing_path = m.getInputFileName();
        backing_path != "Unassigned" and backing_path.size() > 0) {

        // the model has an associated file
        //
        // we can save over this document - *IF* it's not an example file
        if (is_example_file(backing_path)) {
            return map_optional(path2string, prompt_save_single_file());
        } else {
            return backing_path;
        }
    } else {
        // the model has no associated file, so prompt the user for a save
        // location
        return prompt_save_single_file();
    }
}

static void save_model(OpenSim::Model& model, std::string const& save_loc) {
    try {
        model.print(save_loc);
        model.setInputFileName(save_loc);
        log::info("saved model to %s", save_loc.c_str());
        config::add_recent_file(save_loc);
    } catch (OpenSim::Exception const& ex) {
        log::error("error saving model: %s", ex.what());
    }
}

static void try_save_model(OpenSim::Model& model) {
    std::optional<std::string> maybe_save_loc = try_get_save_location(model);

    if (maybe_save_loc) {
        save_model(model, *maybe_save_loc);
    }
}

static void on_delete_selection(osmv::Model_editor_screen::Impl& impl) {
    OpenSim::Component const* selected = impl.selection();

    if (not selected) {
        return;
    }

    if (auto body = try_downcast<OpenSim::Body>(impl.selection()); body) {
        // it's a body. If the owner is a BodySet then try to remove the body
        // from the BodySet
        if (auto* bs = const_cast<OpenSim::BodySet*>(dynamic_cast<OpenSim::BodySet const*>(&body->get()->getOwner()));
            bs) {
            log::error("deleting bodys from model NYI: it segfaults");
            /*
            for (int i = 0; i < bs->getSize(); ++i) {
                if (&bs->get(i) == p) {
                    bs->remove(i);
                    impl->selected_component = nullptr;
                }
            }
            */
        }
    } else if (auto const* geom = find_ancestor<OpenSim::Geometry>(selected); geom) {
        // it's some child of geometry (probably a mesh), try and find its owning
        // frame and clear the frame of all geometry
        if (auto const* frame = find_ancestor<OpenSim::Frame>(geom); frame) {
            const_cast<OpenSim::Frame*>(frame)->updProperty_attached_geometry().clear();

            // clear the selection (it's now deleted)
            impl.selection().reset();

            // notify that the model was modified (UNSAFE because we actually modified it
            // via an ancestor of a pointer to a component it happens to own - that
            // relationship is hard to express entirely in the type system)
            impl.model().UNSAFE_on_begin_modify();
            impl.model().UNSAFE_on_end_modify();
        }
    } else if (auto joint = try_downcast<OpenSim::Joint>(impl.selection()); joint) {
        if (auto const* jointset = dynamic_cast<OpenSim::JointSet const*>(&joint->get()->getOwner()); jointset) {
            for (int i = 0; i < jointset->getSize(); ++i) {
                if (&jointset->get(i) == *joint) {
                    const_cast<OpenSim::JointSet*>(jointset)->remove(i);
                    impl.selection().reset();
                    impl.model().modify()->finalizeFromProperties();
                    break;
                }
            }
        }
    }
}

static void on_save_as(osmv::Model_editor_screen::Impl& impl) {
    std::optional<std::string> maybe_path = prompt_save_single_file();
    if (maybe_path) {
        save_model(impl.model().modify(), *maybe_path);
    }
}

static bool on_keydown(osmv::Model_editor_screen::Impl& impl, SDL_KeyboardEvent const& e) {
    if (e.keysym.mod & KMOD_CTRL) {
        // CTRL-modified keybinds

        switch (e.keysym.sym) {
        case SDLK_s:
            try_save_model(impl.model().modify());
            return true;
        }
    } else {
        // unmodified keybinds

        switch (e.keysym.sym) {
        case SDLK_ESCAPE:
            Application::current().request_screen_transition<Splash_screen>();
            return true;
        case SDLK_DELETE:
            on_delete_selection(impl);
            return true;
        case SDLK_F12:
            on_save_as(impl);
            return true;
        }
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

void osmv::Model_editor_screen::draw() {
    //    // TODO: show simulation

    //    OpenSim::Model& model = *impl->model;

    //    // if running a simulation only show the simulation
    //    if (impl->fdsim) {
    //        auto state_ptr = impl->fdsim->try_pop_state();

    //        if (state_ptr) {
    //            model.realizeReport(*state_ptr);
    //            OpenSim::Component const* selected = nullptr;
    //            OpenSim::Component const* hovered = nullptr;
    //            impl->model_viewer.draw("render", model, *state_ptr, &selected, &hovered);
    //            return;
    //        }
    //    }
    //    // else: the user is editing the model and all panels should be shown

    impl->model_viewer.draw("render", impl->model().get(), impl->get_state(), impl->selection(), impl->hover());

    // screen-specific fixup: all hoverables are their parents
    // TODO: impl->renderer.geometry.for_each_component([](OpenSim::Component const*& c) { c = &c->getOwner(); });

    // hovering
    //
    // if the user's mouse is hovering over a component, print the component's name next to
    // the user's mouse
    if (impl->hover()) {
        OpenSim::Component const& c = *impl->hover();
        sdl::Mouse_state m = sdl::GetMouseState();
        ImVec2 pos{static_cast<float>(m.x + 20), static_cast<float>(m.y)};
        ImGui::GetBackgroundDrawList()->AddText(pos, 0xff0000ff, c.getName().c_str());
    }

    // hierarchy viewer
    if (ImGui::Begin("Hierarchy")) {
        Component_hierarchy_widget hv;
        hv.draw(&impl->model().get().getRoot(), impl->selection(), impl->hover());
    }
    ImGui::End();

    // selection viewer
    if (ImGui::Begin("Selection")) {
        Component_selection_widget sv;
        sv.draw(impl->get_state(), impl->selection());
    }
    ImGui::End();

    // prop editor
    if (ImGui::Begin("Edit Props")) {
        draw_selection_editor(*impl);
    }
    ImGui::End();

    if (ImGui::Begin("Snapshots")) {
        if (impl->can_undo() and ImGui::Button("undo")) {
            impl->do_undo();
        }

        if (impl->can_redo() and ImGui::Button("redo")) {
            impl->do_redo();
        }
    }
    ImGui::End();

    // 'actions' ImGui panel
    //
    // this is a dumping ground for generic editing actions (add body, add something to selection)
    if (ImGui::Begin("Actions")) {
        if (ImGui::Button("Add body")) {
            show_add_body_modal(impl->ui.abm);
        }
        try_draw_add_body_modal(impl->ui.abm, impl->model(), impl->selection());

        for (Add_joint_modal& modal : impl->ui.add_joint_modals) {
            if (ImGui::Button(modal.modal_name.c_str())) {
                modal.show();
            }
            modal.draw(impl->model(), impl->selection());
        }

        if (ImGui::Button("Show model in viewer")) {
            Application::current().request_screen_transition<Show_model_screen>(
                std::make_unique<OpenSim::Model>(impl->model().get()));
        }

        if (auto joint = try_downcast<OpenSim::Joint>(impl->selection()); joint) {
            if (ImGui::Button("Add offset frame")) {
                auto frame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
                frame->setParentFrame(impl->model().get().getGround());
                joint->modify()->addFrame(frame.release());
            }
        }
    }
    ImGui::End();

    // console panel
    if (ImGui::Begin("Log")) {
        Log_viewer_widget{}.draw();
    }
    ImGui::End();
}
