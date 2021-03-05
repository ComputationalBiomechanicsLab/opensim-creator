#include "model_editor_screen.hpp"

#include "src/3d/gpu_cache.hpp"
#include "src/application.hpp"
#include "src/config.hpp"
#include "src/opensim_bindings/fd_simulation.hpp"
#include "src/screens/show_model_screen.hpp"
#include "src/screens/splash_screen.hpp"
#include "src/utils/sdl_wrapper.hpp"
#include "src/widgets/add_joint_modal.hpp"
#include "src/widgets/attach_geometry_modal.hpp"
#include "src/widgets/component_hierarchy_widget.hpp"
#include "src/widgets/component_selection_widget.hpp"
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
static T* find_ancestor(OpenSim::Component* c) {
    while (c) {
        T* p = dynamic_cast<T*>(c);
        if (p) {
            return p;
        }
        c = c->hasOwner() ? const_cast<OpenSim::Component*>(&c->getOwner()) : nullptr;
    }
    return nullptr;
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

struct Model_editor_screen::Impl final {
    std::vector<fs::path> available_vtps = find_all_vtp_resources();
    std::unique_ptr<OpenSim::Model> model;

    std::vector<std::unique_ptr<OpenSim::Model>> snapshots;
    int snapshotidx = 0;

    Gpu_cache gpu_cache;
    Model_viewer_widget model_viewer{gpu_cache, ModelViewerWidgetFlags_Default | ModelViewerWidgetFlags_DrawFrames};

    std::optional<Fd_simulation> fdsim;
    OpenSim::Component const* selected_component = nullptr;
    OpenSim::Component const* hovered_component = nullptr;

    std::array<Add_joint_modal, 4> add_joint_modals = {
        Add_joint_modal::create<OpenSim::FreeJoint>("Add FreeJoint"),
        Add_joint_modal::create<OpenSim::PinJoint>("Add PinJoint"),
        Add_joint_modal::create<OpenSim::UniversalJoint>("Add UniversalJoint"),
        Add_joint_modal::create<OpenSim::BallJoint>("Add BallJoint")};

    Impl(std::unique_ptr<OpenSim::Model> _model) : model{std::move(_model)} {
    }
};

static void draw_top_level_editor(OpenSim::Component& c) {
    ImGui::Columns(2);

    ImGui::Text("name");
    ImGui::NextColumn();

    char nambuf[128];
    nambuf[sizeof(nambuf) - 1] = '\0';
    std::strncpy(nambuf, c.getName().c_str(), sizeof(nambuf) - 1);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth());
    if (ImGui::InputText("##nameditor", nambuf, sizeof(nambuf), ImGuiInputTextFlags_EnterReturnsTrue)) {

        if (std::strlen(nambuf) > 0) {
            c.setName(nambuf);
        }
    }
    ImGui::NextColumn();

    ImGui::Columns();
}

static void draw_frame_contextual_actions(
    OpenSim::Model& model,
    std::vector<fs::path> const& vtps,
    std::vector<std::unique_ptr<OpenSim::Model>>& snapshots,
    OpenSim::PhysicalFrame& frame) {

    ImGui::Columns(2);

    thread_local Attach_geometry_modal modal;

    ImGui::Text("geometry");
    ImGui::NextColumn();
    if (ImGui::Button("attach geometry")) {
        modal.show();
    }
    modal.draw(model, vtps, snapshots, frame);
    ImGui::NextColumn();

    ImGui::Text("offset frame");
    ImGui::NextColumn();
    if (ImGui::Button("add offset frame")) {
        auto* pf = new OpenSim::PhysicalOffsetFrame{};
        pf->setParentFrame(frame);
        frame.addComponent(pf);
    }
    ImGui::NextColumn();

    ImGui::Columns(1);

    ImGui::Separator();
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

static void draw_joint_contextual_actions(OpenSim::Model& model, OpenSim::Component** selected, OpenSim::Joint& joint) {
    assert(selected and dynamic_cast<OpenSim::Joint*>(*selected));

    ImGui::Columns(2);

    if (auto const* jsp = dynamic_cast<OpenSim::JointSet const*>(&joint.getOwner()); jsp) {
        OpenSim::JointSet& js = *const_cast<OpenSim::JointSet*>(jsp);

        int idx = -1;
        for (int i = 0; i < js.getSize(); ++i) {
            if (&js[i] == &joint) {
                idx = i;
                break;
            }
        }

        if (idx >= 0) {
            ImGui::Text("joint type");
            ImGui::NextColumn();

            std::unique_ptr<OpenSim::Joint> added_joint = nullptr;
            if (ImGui::Button("fj")) {
                added_joint.reset(new OpenSim::FreeJoint{});
            }

            ImGui::SameLine();
            if (ImGui::Button("pj")) {
                added_joint.reset(new OpenSim::PinJoint{});
            }

            ImGui::SameLine();
            if (ImGui::Button("uj")) {
                added_joint.reset(new OpenSim::UniversalJoint{});
            }

            ImGui::SameLine();
            if (ImGui::Button("bj")) {
                added_joint.reset(new OpenSim::BallJoint{});
            }

            if (added_joint) {
                copy_common_joint_properties(joint, *added_joint);

                assert(*selected == &joint);
                *selected = added_joint.get();

                js.set(idx, added_joint.release());
            }
        }
    }

    ImGui::Columns();
}

static void draw_contextual_actions(
    OpenSim::Model& model,
    std::vector<fs::path> const& vtps,
    std::vector<std::unique_ptr<OpenSim::Model>>& snapshots,
    OpenSim::Component** selection) {

    assert(selection and *selection and "selection is blank (shouldn't be)");

    if (auto* frame = dynamic_cast<OpenSim::PhysicalFrame*>(*selection); frame) {
        draw_frame_contextual_actions(model, vtps, snapshots, *frame);
    } else if (auto* joint = dynamic_cast<OpenSim::Joint*>(*selection); joint) {
        draw_joint_contextual_actions(model, selection, *joint);
    }
}

static void draw_socket_editor(OpenSim::Model& model, OpenSim::Component& c) {
    std::vector<std::string> socknames = c.getSocketNames();
    ImGui::Columns(2);
    for (std::string const& sn : socknames) {
        ImGui::Text("%s", sn.c_str());
        ImGui::NextColumn();

        OpenSim::AbstractSocket& socket = c.updSocket(sn);
        std::string sockname = socket.getConnecteePath();
        std::string popupname = std::string{"reassign"} + sockname;

        thread_local Reassign_socket_modal modal;

        if (ImGui::Button(sockname.c_str())) {
            modal.show(popupname.c_str());
        }
        modal.draw(popupname.c_str(), model, socket);

        ImGui::NextColumn();
    }
    ImGui::Columns();
}

static void draw_selection_editor(
    OpenSim::Model& model,
    std::vector<fs::path> const& vtps,
    std::vector<std::unique_ptr<OpenSim::Model>>& snapshots,
    OpenSim::Component const** selected) {

    if (not selected or not*selected) {
        ImGui::Text("(nothing selected)");
        return;
    }

    // TODO: naughty naughty: this is because pointers to non-const els
    // cannot be easily be coerced into points to const els (even though
    // you'd expect one to be a logical subset of the other)
    //
    // see:
    // https://stackoverflow.com/questions/36302078/non-const-reference-to-a-non-const-pointer-pointing-to-the-const-object
    OpenSim::Component** mutable_selected = const_cast<OpenSim::Component**>(selected);

    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    ImGui::Text("top-level attributes:");
    ImGui::Separator();
    draw_top_level_editor(**mutable_selected);

    // contextual actions
    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    ImGui::Text("contextual actions:");
    ImGui::Separator();
    draw_contextual_actions(model, vtps, snapshots, mutable_selected);

    // property editor
    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    ImGui::Text("properties:");
    ImGui::Separator();
    draw_properties_editor(**mutable_selected);

    // socket editor
    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    ImGui::Text("sockets:");
    ImGui::Separator();
    draw_socket_editor(model, **mutable_selected);
}

Model_editor_screen::Model_editor_screen() : impl{new Impl{std::make_unique<OpenSim::Model>()}} {
}

Model_editor_screen::Model_editor_screen(std::unique_ptr<OpenSim::Model> _model) : impl{new Impl{std::move(_model)}} {
}

Model_editor_screen::~Model_editor_screen() noexcept {
    delete impl;
}

bool Model_editor_screen::on_event(SDL_Event const& e) {
    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_ESCAPE) {
            Application::current().request_screen_transition<Splash_screen>();
            return true;
        }

        if (e.key.keysym.sym == SDLK_DELETE and impl->selected_component) {
            OpenSim::Component const* selected = impl->selected_component;

            if (auto const* p = dynamic_cast<OpenSim::Body const*>(selected); p) {
                // it's a body. If the owner is a BodySet then try to remove the body
                // from the BodySet
                if (auto* bs = const_cast<OpenSim::BodySet*>(dynamic_cast<OpenSim::BodySet const*>(&p->getOwner()));
                    bs) {
                    std::cerr << "deleting bodys NYI: segfaults" << std::endl;
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
                    impl->selected_component = nullptr;
                }
            } else if (auto const* joint = dynamic_cast<OpenSim::Joint const*>(selected); joint) {
                if (auto const* jointset = dynamic_cast<OpenSim::JointSet const*>(&joint->getOwner()); jointset) {
                    OpenSim::JointSet& js = *const_cast<OpenSim::JointSet*>(jointset);
                    for (int i = 0; i < js.getSize(); ++i) {
                        if (&js.get(i) == joint) {
                            js.remove(i);
                            impl->model->finalizeFromProperties();
                            impl->selected_component = nullptr;
                            break;
                        }
                    }
                }
            }
        }
    }

    return impl->model_viewer.on_event(e);
}

void Model_editor_screen::tick() {
}

void osmv::Model_editor_screen::draw() {
    OpenSim::Model& model = *impl->model;

    // if running a simulation only show the simulation
    if (impl->fdsim) {
        auto state_ptr = impl->fdsim->try_pop_state();

        if (state_ptr) {
            model.realizeReport(*state_ptr);
            OpenSim::Component const* selected = nullptr;
            OpenSim::Component const* hovered = nullptr;
            impl->model_viewer.draw("render", model, *state_ptr, &selected, &hovered);
            return;
        }
    }
    // else: the user is editing the model and all panels should be shown

    // while editing, draw the model from its initial state every time
    //
    // this means that the model can be edited directly, but that we're also editing it in
    // a state that can easily be thrown into (e.g.) an fd simulation for ad-hoc testing
    SimTK::State state = model.initSystem();
    model.realizePosition(state);

    {
        OpenSim::Component const* selected = impl->selected_component;
        impl->model_viewer.draw("render", model, state, &selected, &impl->hovered_component);

        if (selected != impl->selected_component) {
            // selection coercion: if the user selects a FrameGeometry in the 3d view they
            // *probably* want to actually select the object that owns the frame geometry

            if (auto const* fg = dynamic_cast<OpenSim::FrameGeometry const*>(selected); fg) {
                selected = &fg->getOwner();
            }
        }

        impl->selected_component = selected;
    }

    // screen-specific fixup: all hoverables are their parents
    // TODO: impl->renderer.geometry.for_each_component([](OpenSim::Component const*& c) { c = &c->getOwner(); });

    // hovering
    //
    // if the user's mouse is hovering over a component, print the component's name next to
    // the user's mouse
    if (impl->hovered_component) {
        OpenSim::Component const& c = *impl->hovered_component;
        sdl::Mouse_state m = sdl::GetMouseState();
        ImVec2 pos{static_cast<float>(m.x + 20), static_cast<float>(m.y)};
        ImGui::GetBackgroundDrawList()->AddText(pos, 0xff0000ff, c.getName().c_str());
    }

    // hierarchy viewer
    if (ImGui::Begin("Hierarchy")) {
        Component_hierarchy_widget hv;
        hv.draw(&model.getRoot(), &impl->selected_component, &impl->hovered_component);
    }
    ImGui::End();

    // selection viewer
    if (ImGui::Begin("Selection")) {
        Component_selection_widget sv;
        sv.draw(state, &impl->selected_component);
    }
    ImGui::End();

    // prop editor
    if (ImGui::Begin("Edit Props")) {
        draw_selection_editor(*impl->model, impl->available_vtps, impl->snapshots, &impl->selected_component);
    }
    ImGui::End();

    if (ImGui::Begin("Snapshots")) {
        std::vector<std::string> names;
        names.reserve(1 + impl->snapshots.size());

        names.push_back("current");

        for (size_t i = impl->snapshots.size(); i > 0; --i) {
            names.push_back(std::to_string(i));
        }

        std::vector<char const*> cnames;
        cnames.reserve(names.size());
        for (std::string const& name : names) {
            cnames.push_back(name.c_str());
        }

        if (ImGui::Combo("snapshots", &impl->snapshotidx, cnames.data(), static_cast<int>(cnames.size()))) {
            if (impl->snapshotidx > 0) {
                size_t idx = impl->snapshots.size() - static_cast<size_t>(impl->snapshotidx);

                impl->model = std::make_unique<OpenSim::Model>(*impl->snapshots[idx]);
                impl->snapshotidx = 0;
                impl->fdsim.reset();
                impl->selected_component = nullptr;
            }
        }

        if (ImGui::Button("take snapshot")) {
            impl->snapshots.push_back(std::make_unique<OpenSim::Model>(*impl->model));
        }
    }
    ImGui::End();

    // 'actions' ImGui panel
    //
    // this is a dumping ground for generic editing actions (add body, add something to selection)
    if (ImGui::Begin("Actions")) {
        if (ImGui::Button("Add body")) {
            auto* body = new OpenSim::Body{"added_body", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{0.1}};
            model.addBody(body);
            impl->selected_component = body;
        }

        for (Add_joint_modal& modal : impl->add_joint_modals) {
            if (ImGui::Button(modal.modal_name.c_str())) {
                modal.show();
            }
            modal.draw(*impl->model, &impl->selected_component);
        }

        if (ImGui::Button("Show model in viewer")) {
            Application::current().request_screen_transition<Show_model_screen>(
                std::make_unique<OpenSim::Model>(*impl->model));
        }

        OpenSim::Component* c = const_cast<OpenSim::Component*>(impl->selected_component);

        if (auto* j = find_ancestor<OpenSim::Joint>(c); j) {
            if (ImGui::Button("Add offset frame")) {
                auto* frame = new OpenSim::PhysicalOffsetFrame{};
                frame->setParentFrame(model.getGround());
                j->addFrame(frame);
            }
        }
    }
    ImGui::End();
}
