#include "model_editor_screen.hpp"

#include "application.hpp"
#include "fd_simulation.hpp"
#include "hierarchy_viewer.hpp"
#include "sdl_wrapper.hpp"
#include "selection_viewer.hpp"
#include "simple_model_renderer.hpp"
#include "splash_screen.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <imgui.h>
#include <optional>

namespace osmv {
    struct Model_editor_screen_impl final {
        OpenSim::Model model;

        int parentbodyidx = -1;
        char bodyname[64] = "added_body";
        char jointname[64] = "added_joint";

        char jointTxName[64] = "xTranslation";
        float jointTx = 0.0f;

        char jointTyName[64] = "yTranslation";
        float jointTy = 0.0f;

        char jointTzName[64] = "zTranslation";
        float jointTz = 0.0f;

        Simple_model_renderer renderer{app().window_dimensions().w, app().window_dimensions().h, app().samples()};

        osmv::State fdss{model.initSystem()};
        std::optional<Fd_simulator> fdsim;

        double mass = 20.0;
        SimTK::Vec3 centerOfMass{0.0};
        OpenSim::Component const* selected_component = nullptr;

        Model_editor_screen_impl() {
            renderer.flags |= SimpleModelRendererFlags_HoverableStaticDecorations;
            model.realizeReport(fdss);
            model.updDisplayHints().set_show_frames(true);
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

        if (e.key.keysym.sym == SDLK_SPACE) {
            if (impl->fdsim) {
                impl->fdsim = std::nullopt;
            } else {
                impl->fdsim.emplace(Fd_simulation_params{Model{impl->model},
                                                         State{impl->model.initSystem()},
                                                         static_cast<double>(10),
                                                         IntegratorMethod_ExplicitEuler});
            }
            return true;
        }
    }

    // if the user right-clicks something in the viewport while the renderer detects
    // a hover-over, then make the hover-over the selection
    if (e.type == SDL_MOUSEBUTTONUP) {
        if (e.button.button == SDL_BUTTON_RIGHT and impl->renderer.hovered_component) {
            impl->selected_component = impl->renderer.hovered_component;
        }
    }

    return impl->renderer.on_event(e);
}

void osmv::Model_editor_screen::tick() {
}

void osmv::Model_editor_screen::draw() {
    OpenSim::Model& model = impl->model;

    // if running a simulation only show the simulation
    if (impl->fdsim and impl->fdsim->try_pop_state(impl->fdss)) {
        model.realizeReport(impl->fdss);
        impl->renderer.draw(model, impl->fdss);
        return;
    }
    // else: the user is editing the model and all panels should be shown

    // while editing, draw the model from its initial state every time
    //
    // this means that the model can be edited directly, but that we're also editing it in
    // a state that can easily be thrown into (e.g.) an fd simulation for ad-hoc testing
    SimTK::State state = model.initSystem();

    // render it
    {
        model.realizePosition(state);
        impl->renderer.generate_geometry(model, state);

        // screen-specific fixup: all hoverables are their parents
        {
            for (OpenSim::Component const*& c : impl->renderer.geometry.associated_components) {
                c = &c->getOwner();
            }
        }

        impl->renderer.apply_standard_rim_coloring(nullptr);
        impl->renderer.draw();
    }

    // hovering
    //
    // if the user's mouse is hovering over a component, print the component's name next to
    // the user's mouse
    if (impl->renderer.hovered_component) {
        OpenSim::Component const& c = *impl->renderer.hovered_component;
        sdl::Mouse_state m = sdl::GetMouseState();
        ImVec2 pos{static_cast<float>(m.x + 20), static_cast<float>(m.y)};
        ImGui::GetBackgroundDrawList()->AddText(pos, 0xff0000ff, c.getName().c_str());
    }

    // hierarchy viewer
    if (ImGui::Begin("Hierarchy")) {
        Hierarchy_viewer hv;
        hv.draw(&model.getRoot(), &impl->selected_component, &impl->renderer.hovered_component);
    }
    ImGui::End();

    // selection viewer
    if (ImGui::Begin("Selection")) {
        Selection_viewer sv;
        sv.draw(state, &impl->selected_component);
    }
    ImGui::End();

    // 'actions' ImGui panel
    //
    // this is a dumping ground for generic editing actions (add body, add something to selection)
    if (ImGui::Begin("Actions")) {

        ImGui::Text("add joint");
        ImGui::Separator();

        ImGui::Text("joint type: FreeJoint");

        // this body
        ImGui::Text("body details:");
        {
            ImGui::InputText("name", impl->bodyname, sizeof(impl->bodyname));
            {
                float mass = static_cast<float>(impl->mass);
                if (ImGui::SliderFloat("mass", &mass, 0.001f, 10.0f)) {
                    impl->mass = static_cast<double>(mass);
                }
            }
            {
                float offset0 = static_cast<float>(impl->centerOfMass[0]);
                if (ImGui::SliderFloat("com x", &offset0, -10.0f, 10.0f)) {
                    impl->centerOfMass[0] = static_cast<double>(offset0);
                }
            }
            {
                float offset1 = static_cast<float>(impl->centerOfMass[1]);
                if (ImGui::SliderFloat("com y", &offset1, -10.0f, 10.0f)) {
                    impl->centerOfMass[1] = static_cast<double>(offset1);
                }
            }
            {
                float offset2 = static_cast<float>(impl->centerOfMass[2]);
                if (ImGui::SliderFloat("com z", &offset2, -10.0f, 10.0f)) {
                    impl->centerOfMass[2] = static_cast<double>(offset2);
                }
            }
        }

        // parent body
        ImGui::Text("parent body details:");
        OpenSim::PhysicalFrame const* parent = nullptr;
        {
            std::vector<char const*> names;
            names.push_back("ground");

            auto const& bs = model.getBodySet();
            for (int i = 0; i < bs.getSize(); ++i) {
                OpenSim::Body const* b = &bs[i];
                names.push_back(b->getName().c_str());
            }

            ImGui::Combo("parent", &impl->parentbodyidx, names.data(), static_cast<int>(names.size()));

            if (impl->parentbodyidx < 0) {
                parent = nullptr;
            } else if (impl->parentbodyidx == 0) {
                parent = &model.getGround();
            } else {
                parent = &model.getBodySet()[impl->parentbodyidx - 1];
            }
        }

        ImGui::Text("joint details:");
        {
            ImGui::InputText("joint name", impl->jointname, sizeof(impl->jointname));

            ImGui::InputText("tx coordname", impl->jointTxName, sizeof(impl->jointTxName));
            ImGui::SliderFloat("tx", &impl->jointTx, -10.0f, 10.0f);

            ImGui::InputText("ty coordname", impl->jointTyName, sizeof(impl->jointTyName));
            ImGui::SliderFloat("ty", &impl->jointTy, -10.0f, 10.0f);

            ImGui::InputText("tz coordname", impl->jointTzName, sizeof(impl->jointTzName));
            ImGui::SliderFloat("tz", &impl->jointTz, -10.0f, 10.0f);
        }

        if (parent != nullptr) {
            if (ImGui::Button("add")) {
                auto* mesh = new OpenSim::Mesh{"block.vtp"};
                auto* body = new OpenSim::Body{impl->bodyname, impl->mass, impl->centerOfMass, SimTK::Inertia{0.1}};
                body->attachGeometry(mesh);

                auto* joint = new OpenSim::FreeJoint{impl->jointname, *parent, *body};

                // coordinate indices:
                // [0-3): rot x y z
                // [3-6): trans x y z
                joint->upd_coordinates(3).set_default_value(static_cast<double>(impl->jointTx));
                joint->upd_coordinates(4).set_default_value(static_cast<double>(impl->jointTy));
                joint->upd_coordinates(5).set_default_value(static_cast<double>(impl->jointTz));

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
        }
    }
    ImGui::End();
}
