#include "model_editor_screen.hpp"

#include "src/3d/gpu_cache.hpp"
#include "src/application.hpp"
#include "src/config.hpp"
#include "src/log.hpp"
#include "src/opensim_bindings/fd_simulation.hpp"
#include "src/opensim_bindings/model_ui_state.hpp"
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
private:
    Model_ui_state ms;
    Gpu_cache gpu_cache;
    Circular_buffer<Undo_redo_entry, 32> undo;
    Circular_buffer<Undo_redo_entry, 32> redo;
    bool recovered_from_disaster = false;

public:
    Model_viewer_widget model_viewer{gpu_cache, ModelViewerWidgetFlags_Default | ModelViewerWidgetFlags_DrawFrames};

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
    } ui;

    File_change_poller file_poller{1000ms, ""};

    void before_modify_model() {
        log::info("start modification");
        attempt_new_undo_push();
    }

    void after_modify_model() {
        log::info("end modification");
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

        if (!undo.empty() && (undo.back().time + 5s) > now) {
            return;  // too temporally close to previous push
        }

        undo.emplace_back(now, ms);
        redo.clear();
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

static void draw_contextual_actions(Model_editor_screen::Impl& impl) {
    if (!impl.selection()) {
        ImGui::Text("cannot draw contextual actions: selection is blank (shouldn't be)");
        return;
    }

    if (auto* frame = dynamic_cast<OpenSim::PhysicalFrame*>(impl.selection()); frame) {
        draw_frame_contextual_actions(impl, *frame);
    } else if (auto* joint = dynamic_cast<OpenSim::Joint*>(impl.selection()); joint) {
        draw_joint_contextual_actions(impl, *joint);
    }
}

static void draw_socket_editor(Model_editor_screen::Impl& impl) {

    if (!impl.selection()) {
        ImGui::Text("cannot draw socket editor: selection is blank (shouldn't be)");
        return;
    }
    OpenSim::Component& selection = *impl.selection();

    // this UNSAFE is because I want to get the socket names, which is a non-const method on
    // the `selection`, but doesn't actually modify the selection (much, at least...)
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

static bool on_keydown(osmv::Model_editor_screen::Impl& impl, SDL_KeyboardEvent const& e) {
    if (e.keysym.mod & KMOD_CTRL) {
        // CTRL

        if (e.keysym.mod & KMOD_SHIFT) {
            // CTRL+SHIFT

            switch (e.keysym.sym) {
            case SDLK_s:
                main_menu_save_as(impl.model());
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
    auto const& fname = impl->model().getInputFileName();
    if (impl->file_poller.change_detected(fname)) {
        std::unique_ptr<OpenSim::Model> p;
        try {
            p = std::make_unique<OpenSim::Model>(fname);
            log::info("opened updated file");
        } catch (std::exception const& ex) {
            log::error("an error occurred while trying to automatically load a model file");
            log::error(ex.what());
        }

        if (p) {
            impl->set_model(std::move(p));
        }
    }
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

    if (ImGui::BeginMainMenuBar()) {
        draw_main_menu_file_tab(impl->ui.main_menu_tab, &impl->model());
        draw_main_menu_about_tab();
        ImGui::EndMainMenuBar();
    }

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
        auto on_select = [this](auto const* c) { impl->set_selection(const_cast<OpenSim::Component*>(c)); };
        auto on_hover = [this](auto const* c) { impl->set_hover(const_cast<OpenSim::Component*>(c)); };
        draw_component_hierarchy_widget(
            &impl->model().getRoot(), impl->selection(), impl->hover(), on_select, on_hover);
    }
    ImGui::End();

    // selection viewer
    if (ImGui::Begin("Selection")) {
        auto on_selection_changed = [this](auto const* c) { impl->set_selection(const_cast<OpenSim::Component*>(c)); };
        draw_component_selection_widget(impl->get_state(), impl->selection(), on_selection_changed);
    }
    ImGui::End();

    // prop editor
    if (ImGui::Begin("Edit Props")) {
        draw_selection_editor(*impl);
    }
    ImGui::End();

    if (ImGui::Begin("Snapshots")) {
        if (impl->can_undo() && ImGui::Button("undo")) {
            impl->do_undo();
        }

        if (impl->can_redo() && ImGui::Button("redo")) {
            impl->do_redo();
        }
    }
    ImGui::End();

    // 'actions' ImGui panel
    //
    // this is a dumping ground for generic editing actions (add body, add something to selection)
    if (ImGui::Begin("Actions")) {
        static constexpr char const* add_body_modal_name = "add body";

        if (ImGui::Button("Add body")) {
            ImGui::OpenPopup(add_body_modal_name);
        }

        auto on_body_add = [& impl = *this->impl](Added_body_modal_output out) {
            impl.before_modify_model();
            impl.model().addJoint(out.joint.release());
            OpenSim::Body const* b = out.body.get();
            impl.model().addBody(out.body.release());
            impl.set_selection(const_cast<OpenSim::Body*>(b));
            impl.after_modify_model();
        };

        try_draw_add_body_modal(impl->ui.abm, add_body_modal_name, impl->model(), on_body_add);

        auto on_add_joint = [this](auto joint) {
            auto const* ptr = joint.get();
            impl->before_modify_model();
            impl->model().addJoint(joint.release());
            impl->set_selection(const_cast<OpenSim::Joint*>(ptr));
            impl->after_modify_model();
        };

        for (Add_joint_modal& modal : impl->ui.add_joint_modals) {
            if (ImGui::Button(modal.modal_name.c_str())) {
                modal.show();
            }
            modal.draw(impl->model(), on_add_joint);
        }

        if (ImGui::Button("Show model in viewer")) {
            Application::current().request_screen_transition<Show_model_screen>(
                std::make_unique<OpenSim::Model>(impl->model()));
        }

        if (auto* joint = dynamic_cast<OpenSim::Joint*>(impl->selection()); joint) {
            if (ImGui::Button("Add offset frame")) {
                auto frame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
                frame->setParentFrame(impl->model().getGround());

                impl->before_modify_selection();
                joint->addFrame(frame.release());
                impl->after_modify_selection();
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
