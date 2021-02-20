#include "model_viewer_widget.hpp"

#include "src/3d/gl.hpp"
#include "src/3d/simple_model_renderer.hpp"
#include "src/application.hpp"
#include "src/sdl_wrapper.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <imgui.h>

namespace osmv {
    struct Model_viewer_widget_impl final {
        Simple_model_renderer renderer;
        bool mouse_over_render = false;

        Model_viewer_widget_impl() : renderer{100, 100, app().samples()} {
        }
    };
}

osmv::Model_viewer_widget::Model_viewer_widget() : impl{new Model_viewer_widget_impl{}} {
}

osmv::Model_viewer_widget::~Model_viewer_widget() noexcept {
    delete impl;
}

bool osmv::Model_viewer_widget::is_moused_over() const noexcept {
    return impl->mouse_over_render;
}

osmv::Polar_camera& osmv::Model_viewer_widget::camera() noexcept {
    return impl->renderer.camera;
}

bool osmv::Model_viewer_widget::on_event(const SDL_Event& e) {
    // if no events were captured above, let the model viewer handle
    // the event
    if (impl->mouse_over_render or e.type == SDL_MOUSEBUTTONUP) {
        return impl->renderer.on_event(e);
    }
    return false;
}

void osmv::Model_viewer_widget::draw(
    char const* panel_name,
    OpenSim::Model const& model,
    SimTK::State const& state,
    OpenSim::Component const** selected,
    OpenSim::Component const** hovered) {

    Simple_model_renderer& renderer = impl->renderer;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0, 0.0));
    if (ImGui::Begin(panel_name)) {
        if (ImGui::BeginChild("##child", ImVec2(0, 0), false, ImGuiWindowFlags_NoMove)) {
            // generate OpenSim scene geometry
            renderer.generate_geometry(model, state);

            // perform screen-specific geometry fixups
            //    if (only_select_muscles) {
            renderer.geometry.for_each(
                [&model](OpenSim::Component const*& associated_component, Raw_mesh_instance const&) {
                    // for this screen specifically, the "owner"s should be fixed up to point to
                    // muscle objects, rather than direct (e.g. GeometryPath) objects
                    OpenSim::Component const* c = associated_component;
                    while (c != nullptr and c->hasOwner()) {
                        if (dynamic_cast<OpenSim::Muscle const*>(c)) {
                            break;
                        }
                        c = &c->getOwner();
                    }
                    if (c == &model) {
                        c = nullptr;
                    }

                    associated_component = c;
                });
            //    }

            /*
        if (muscle_recoloring == MuscleRecoloring_Strain) {
            renderer.geometry.for_each([&state](OpenSim::Component const* c, Raw_mesh_instance& mi) {
                OpenSim::Muscle const* musc = dynamic_cast<OpenSim::Muscle const*>(c);
                if (not musc) {
                    return;
                }

                mi.rgba.r = static_cast<unsigned char>(255.0 * musc->getTendonStrain(state));
                mi.rgba.g = 255 / 2;
                mi.rgba.b = 255 / 2;
                mi.rgba.a = 255;
            });
        }

        if (muscle_recoloring == MuscleRecoloring_Length) {
            renderer.geometry.for_each([&state](OpenSim::Component const* c, Raw_mesh_instance& mi) {
                OpenSim::Muscle const* musc = dynamic_cast<OpenSim::Muscle const*>(c);
                if (not musc) {
                    return;
                }

                mi.rgba.r = static_cast<unsigned char>(255.0 * musc->getLength(state));
                mi.rgba.g = 255 / 4;
                mi.rgba.b = 255 / 4;
                mi.rgba.a = 255;
            });
        }
        */

            // draw the scene to an OpenGL texture
            renderer.apply_standard_rim_coloring(*selected);
            auto dims = ImGui::GetContentRegionAvail();

            if (dims.x >= 1 and dims.y >= 1) {
                renderer.reallocate_buffers(static_cast<int>(dims.x), static_cast<int>(dims.y), app().samples());

                gl::Texture_2d& render = renderer.draw();

                {
                    // required by ImGui::Image
                    //
                    // UV coords: ImGui::Image uses different texture coordinates from the renderer
                    //            (specifically, Y is reversed)
                    void* texture_handle = reinterpret_cast<void*>(render.raw_handle());
                    ImVec2 image_dimensions{dims.x, dims.y};
                    ImVec2 uv0{0, 1};
                    ImVec2 uv1{1, 0};

                    ImVec2 cp = ImGui::GetCursorPos();
                    ImVec2 mp = ImGui::GetMousePos();
                    ImVec2 wp = ImGui::GetWindowPos();

                    ImGui::Image(texture_handle, image_dimensions, uv0, uv1);

                    impl->mouse_over_render = ImGui::IsItemHovered();

                    renderer.hovertest_x = static_cast<int>((mp.x - wp.x) - cp.x);
                    // y is reversed (OpenGL coords, not screen)
                    renderer.hovertest_y = static_cast<int>(dims.y - ((mp.y - wp.y) - cp.y));
                }

                // overlay: if the user is hovering over a component, write the component's name
                //          next to the mouse
                if (renderer.hovered_component) {
                    OpenSim::Component const& c = *renderer.hovered_component;
                    sdl::Mouse_state m = sdl::GetMouseState();
                    ImVec2 pos{static_cast<float>(m.x + 20), static_cast<float>(m.y)};
                    ImGui::GetBackgroundDrawList()->AddText(pos, 0xff0000ff, c.getName().c_str());
                }

                if (renderer.hovered_component != *hovered) {
                    *hovered = renderer.hovered_component;
                }
            }

            ImGui::EndChild();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
