#include "model_editor_screen.hpp"

#include "splash_screen.hpp"
#include "src/application.hpp"
#include "src/config.hpp"
#include "src/fd_simulation.hpp"
#include "src/sdl_wrapper.hpp"
#include "src/widgets/component_hierarchy_widget.hpp"
#include "src/widgets/component_selection_widget.hpp"
#include "src/widgets/model_viewer_widget.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <imgui.h>

#include <filesystem>
#include <optional>
#include <vector>

namespace fs = std::filesystem;

namespace {
    template<typename T>
    T const* find_ancestor(OpenSim::Component const* c) {
        while (c) {
            T const* p = dynamic_cast<T const*>(c);
            if (p) {
                return p;
            }
            c = c->hasOwner() ? &c->getOwner() : nullptr;
        }
        return nullptr;
    }

    bool filename_lexographically_gt(fs::path const& a, fs::path const& b) {
        return a.filename() < b.filename();
    }

    std::vector<fs::path> find_all_vtp_resources() {
        fs::path geometry_dir = osmv::config::resource_path("geometry");

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

}

namespace osmv {
    struct Model_editor_screen_impl final {
        std::vector<fs::path> available_vtps = find_all_vtp_resources();

        OpenSim::Model model;

        char meshname[128] = "block.vtp";

        std::vector<OpenSim::Model> snapshots;
        int snapshotidx = 0;

        int parentbodyidx = -1;

        Model_viewer_widget model_viewer;

        osmv::State simulator_state{model.initSystem()};
        osmv::State editor_state{model.initSystem()};
        std::optional<Fd_simulator> fdsim;
        OpenSim::Component const* selected_component = nullptr;
        OpenSim::Component const* hovered_component = nullptr;

        Model_editor_screen_impl() {
            // TODO: renderer.flags |= SimpleModelRendererFlags_HoverableStaticDecorations;
            model.realizeReport(simulator_state);
            model.updDisplayHints().set_show_frames(true);
            model_viewer.geometry_flags |= ModelViewerGeometryFlags_CanInteractWithStaticDecorations;
            model_viewer.geometry_flags &= ~ModelViewerGeometryFlags_CanOnlyInteractWithMuscles;
        }

        void draw_prop_editor() {
            if (not selected_component) {
                ImGui::Text("select something");
                return;
            }

            // TODO: naughty naughty: this is because pointers to non-const els
            // cannot be easily be coerced into points to const els (even though
            // you'd expect one to be a logical subset of the other)
            //
            // see:
            // https://stackoverflow.com/questions/36302078/non-const-reference-to-a-non-const-pointer-pointing-to-the-const-object
            OpenSim::Component& c = const_cast<OpenSim::Component&>(*selected_component);

            // properties
            ImGui::Columns(2);

            // edit name
            {
                ImGui::Text("name");
                ImGui::NextColumn();

                char nambuf[128];
                nambuf[sizeof(nambuf) - 1] = '\0';
                std::strncpy(nambuf, c.getName().c_str(), sizeof(nambuf) - 1);
                if (ImGui::InputText("##nameditor", nambuf, sizeof(nambuf))) {
                    c.setName(nambuf);
                }
                ImGui::NextColumn();
            }

            for (int i = 0; i < c.getNumProperties(); ++i) {
                OpenSim::AbstractProperty& p = c.updPropertyByIndex(i);
                ImGui::Text("%s", p.getName().c_str());
                ImGui::NextColumn();

                ImGui::PushID(p.getName().c_str());
                bool editor_rendered = false;

                // try string
                {
                    auto* sp = dynamic_cast<OpenSim::Property<std::string>*>(&p);
                    if (sp) {
                        char buf[64]{};
                        buf[sizeof(buf) - 1] = '\0';
                        std::strncpy(buf, sp->getValue().c_str(), sizeof(buf) - 1);

                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::InputText("##stringeditor", buf, sizeof(buf));

                        editor_rendered = true;
                    }
                }

                // try double
                {
                    auto* dp = dynamic_cast<OpenSim::Property<double>*>(&p);
                    if (dp and not dp->isListProperty()) {
                        float v = static_cast<float>(dp->getValue());

                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        if (ImGui::InputFloat("##doubleditor", &v)) {
                            dp->setValue(static_cast<double>(v));
                        }

                        editor_rendered = true;
                    }
                }

                // try Vec3
                {
                    auto* vp = dynamic_cast<OpenSim::Property<SimTK::Vec3>*>(&p);
                    if (vp and not vp->isListProperty()) {
                        SimTK::Vec3 const& v = vp->getValue();
                        float vs[3];
                        vs[0] = static_cast<float>(v[0]);
                        vs[1] = static_cast<float>(v[1]);
                        vs[2] = static_cast<float>(v[2]);

                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        if (ImGui::InputFloat3("##vec3editor", vs)) {
                            vp->setValue(SimTK::Vec3{
                                static_cast<double>(vs[0]), static_cast<double>(vs[1]), static_cast<double>(vs[2])});
                        }

                        editor_rendered = true;
                    }
                }

                if (not editor_rendered) {
                    ImGui::Text("%s", p.toString().c_str());
                }

                ImGui::PopID();

                ImGui::NextColumn();
            }
            ImGui::Columns();
        }

        void draw_physical_frame_actions(OpenSim::PhysicalFrame& pf) {
            if (ImGui::Button("Add offset frame")) {
                auto* frame = new OpenSim::PhysicalOffsetFrame{};
                frame->setParentFrame(pf);
                pf.addComponent(frame);
            }

            if (ImGui::Button("Add body")) {
                auto* body = new OpenSim::Body{"added_body", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{0.1}};
                model.addBody(body);
            }

            if (ImGui::Button("Add FreeJoint")) {
                auto* mesh = new OpenSim::Mesh{meshname};
                auto* body = new OpenSim::Body{"added_body", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{0.1}};
                body->attachGeometry(mesh);

                auto* joint = new OpenSim::FreeJoint{"freejoint", pf, *body};

                // SimTK::Vec3 offset{0.0, 0.0, 0.0};
                // auto* pof = new OpenSim::PhysicalOffsetFrame{"parent_offset", *parent, offset};
                // joint->addFrame(pof);

                // auto* cof = new OpenSim::PhysicalOffsetFrame{"child_offset", *body, offset};
                // joint->addFrame(cof);

                auto& bs = model.updBodySet();
                bs.insert(bs.getSize(), body);

                auto& js = model.updJointSet();
                js.insert(js.getSize(), joint);
            }

            if (ImGui::Button("Add PinJoint")) {
                auto* mesh = new OpenSim::Mesh{meshname};
                auto* body = new OpenSim::Body{"added_body", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{0.1}};
                body->attachGeometry(mesh);

                auto* joint = new OpenSim::PinJoint{"pinjoint", pf, *body};

                // SimTK::Vec3 offset{0.0, 0.0, 0.0};
                // auto* pof = new OpenSim::PhysicalOffsetFrame{"parent_offset", *parent, offset};
                // joint->addFrame(pof);

                // auto* cof = new OpenSim::PhysicalOffsetFrame{"child_offset", *body, offset};
                // joint->addFrame(cof);

                auto& bs = model.updBodySet();
                bs.insert(bs.getSize(), body);

                auto& js = model.updJointSet();
                js.insert(js.getSize(), joint);
            }

            for (fs::path const& p : available_vtps) {
                if (ImGui::Button(p.filename().string().c_str())) {
                    // pf.updProperty_attached_geometry().clear();
                    pf.attachGeometry(new OpenSim::Mesh{p});
                }
            }
        }
    };
}

osmv::Model_editor_screen::Model_editor_screen() : impl{new Model_editor_screen_impl{}} {
}

osmv::Model_editor_screen::~Model_editor_screen() noexcept {
    delete impl;
}

bool osmv::Model_editor_screen::on_event(SDL_Event const& e) {
    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_ESCAPE) {
            app().request_screen_transition<Splash_screen>();
            return true;
        }

        if (e.key.keysym.sym == SDLK_DELETE and impl->selected_component) {
            auto* geom = const_cast<OpenSim::Geometry*>(find_ancestor<OpenSim::Geometry>(impl->selected_component));
            auto* pf = const_cast<OpenSim::Frame*>(find_ancestor<OpenSim::Frame>(geom));
            if (geom and pf) {
                pf->updProperty_attached_geometry().clear();
                impl->selected_component = nullptr;
            }
        }
    }

    // if the user right-clicks something in the viewport while the renderer detects
    // a hover-over, then make the hover-over the selection
    if (e.type == SDL_MOUSEBUTTONUP) {
        if (e.button.button == SDL_BUTTON_RIGHT and impl->hovered_component) {
            impl->selected_component = impl->hovered_component;
        }
    }

    return impl->model_viewer.on_event(e);
}

void osmv::Model_editor_screen::tick() {
}

void osmv::Model_editor_screen::draw() {
    OpenSim::Model& model = impl->model;

    // if running a simulation only show the simulation
    if (impl->fdsim and impl->fdsim->try_pop_state(impl->simulator_state)) {
        model.realizeReport(impl->simulator_state);

        OpenSim::Component const* selected = nullptr;
        OpenSim::Component const* hovered = nullptr;
        impl->model_viewer.draw("render", model, impl->simulator_state, &selected, &hovered);
        return;
    }
    // else: the user is editing the model and all panels should be shown

    // while editing, draw the model from its initial state every time
    //
    // this means that the model can be edited directly, but that we're also editing it in
    // a state that can easily be thrown into (e.g.) an fd simulation for ad-hoc testing
    SimTK::State state = model.initSystem();
    model.realizePosition(state);

    impl->model_viewer.draw("render", model, state, &impl->selected_component, &impl->hovered_component);

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
        impl->draw_prop_editor();
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

                impl->model = impl->snapshots[idx];
                impl->snapshotidx = 0;
                impl->fdsim.reset();
                impl->selected_component = nullptr;
            }
        }

        if (ImGui::Button("take snapshot")) {
            impl->snapshots.push_back(impl->model);
        }
    }
    ImGui::End();

    // 'actions' ImGui panel
    //
    // this is a dumping ground for generic editing actions (add body, add something to selection)
    if (ImGui::Begin("Actions")) {
        if (impl->selected_component == nullptr) {
            ImGui::Text("select something");
        }

        // if a physical frame is selected, show "add joint" option
        auto* pf = const_cast<OpenSim::PhysicalFrame*>(find_ancestor<OpenSim::PhysicalFrame>(impl->selected_component));
        if (pf) {
            impl->draw_physical_frame_actions(*pf);
        }

        auto* j = const_cast<OpenSim::Joint*>(find_ancestor<OpenSim::Joint>(impl->selected_component));
        if (j) {

            ImGui::Text("joint selected");
        }
    }
    ImGui::End();
}
