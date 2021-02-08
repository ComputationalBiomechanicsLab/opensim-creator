#include "model_editor_screen.hpp"

#include "application.hpp"
#include "fd_simulation.hpp"
#include "sdl_wrapper.hpp"
#include "simple_model_renderer.hpp"
#include "splash_screen.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <imgui.h>
#include <optional>

namespace osmv {
    struct Model_editor_screen_impl final {
        OpenSim::Model model;
        Simple_model_renderer renderer{app().window_dimensions().w, app().window_dimensions().h, app().samples()};

        osmv::State fdss{model.initSystem()};
        std::optional<Fd_simulator> fdsim;

        double mass = 0.1;
        SimTK::Vec3 offset{0.0};
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
    if (impl->fdsim) {
        impl->fdsim->try_pop_state(impl->fdss);
        impl->model.realizeReport(impl->fdss);
        impl->renderer.draw(impl->model, impl->fdss);
        return;
    }

    // while editing, draw the model from the initial state each
    // draw call
    SimTK::State s = impl->model.initSystem();
    impl->model.realizePosition(s);
    impl->renderer.generate_geometry(impl->model, s);

    // screen-specific fixup: all hoverables are their parents
    {
        for (OpenSim::Component const*& c : impl->renderer.geometry.associated_components) {
            c = &c->getOwner();
        }
    }

    impl->renderer.apply_standard_rim_coloring(nullptr);
    impl->renderer.draw();

    // if the user is hovering a component, write the component's name
    // next to the user's mouse
    if (impl->renderer.hovered_component) {
        OpenSim::Component const& c = *impl->renderer.hovered_component;
        sdl::Mouse_state m = sdl::GetMouseState();
        ImVec2 pos{static_cast<float>(m.x + 20), static_cast<float>(m.y)};
        ImGui::GetBackgroundDrawList()->AddText(pos, 0xff0000ff, c.getName().c_str());
    }

    bool b = true;
    if (ImGui::Begin("Actions", &b)) {
        ImGui::Text(impl->fdsim ? "running" : "not running");

        {
            float m = static_cast<float>(impl->mass);
            if (ImGui::SliderFloat("mass", &m, 0.001f, 10.0f)) {
                impl->mass = static_cast<double>(m);
            }
        }
        {
            float x = static_cast<float>(impl->offset[0]);
            if (ImGui::SliderFloat("x", &x, -10.0f, 10.0f)) {
                impl->offset[0] = static_cast<double>(x);
            }
        }
        {
            float y = static_cast<float>(impl->offset[1]);
            if (ImGui::SliderFloat("y", &y, -10.0f, 10.0f)) {
                impl->offset[1] = static_cast<double>(y);
            }
        }
        {
            float z = static_cast<float>(impl->offset[2]);
            if (ImGui::SliderFloat("z", &z, -10.0f, 10.0f)) {
                impl->offset[2] = static_cast<double>(z);
            }
        }

        if (ImGui::Button("Add body")) {
            OpenSim::Body* origin = new OpenSim::Body{"o", impl->mass, SimTK::Vec3{0.0}, SimTK::Inertia(0.5)};
            impl->model.addComponent(origin);
            OpenSim::PhysicalOffsetFrame* pof = new OpenSim::PhysicalOffsetFrame{"offset", *origin, impl->offset};
            impl->model.addComponent(pof);
        }

        ImGui::Separator();

        if (impl->selected_component) {
            ImGui::Text("selected: %s", impl->selected_component->getName().c_str());
            OpenSim::Body const* body = dynamic_cast<OpenSim::Body const*>(impl->selected_component);
            for (OpenSim::Body const& b2 : impl->model.getComponentList<OpenSim::Body>()) {
                if (&b2 == body) {
                    continue;
                }
                // add other body
                // add joint connecting body 1 to body 2
                //     - pulldown for types of joints
                //     - (good default): FreeJoint
                // name coords in joint? (e.g. pin joint has 1 DoF): call it "whatever"
                // force element / muscle (Model::addForce(body1, body2))

                // if they add a joint
                // - present "connectors" for the joint
                // - they use the connector UI to select the bodies

                // torque:
                // has a connector for torque (select coord of Joint)
            }
            if (body) {
                if (ImGui::Button("add joint")) {
                }
                ImGui::Text("it's a body");
            }
        }
    }
    ImGui::End();
}
