#include "model_viewer_widget.hpp"

#include "src/3d/constants.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/labelled_model_drawlist.hpp"
#include "src/3d/model_drawlist_generator.hpp"
#include "src/3d/polar_camera.hpp"
#include "src/3d/raw_renderer.hpp"
#include "src/application.hpp"
#include "src/sdl_wrapper.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <imgui.h>
#include <imgui_internal.h>

namespace {
    using namespace osmv;

    using ModelViewerRecoloring = int;
    enum ModelViewerRecoloring_ {
        ModelViewerRecoloring_None = 0,
        ModelViewerRecoloring_Strain,
        ModelViewerRecoloring_Length,
        ModelViewerRecoloring_COUNT,
    };

    void generate_geometry(
        ModelViewerGeometryFlags flags,
        Labelled_model_drawlist& geometry,
        Model_drawlist_generator& drawlist_generator,
        OpenSim::Model const& model,
        SimTK::State const& state) {

        // iterate over all components in the OpenSim model, keeping a few things in mind:
        //
        // - Anything in the component tree *might* render geometry
        //
        // - For selection logic, we only (currently) care about certain high-level components,
        //   like muscles
        //
        // - Pretend the component tree traversal is implementation-defined because OpenSim's
        //   implementation of component-tree walking is a bit of a clusterfuck. At time of
        //   writing, it's a breadth-first recursive descent
        //
        // - Components of interest, like muscles, might not render their geometry - it might be
        //   delegated to a subcomponent
        //
        // So this algorithm assumes that the list iterator is arbitrary, but always returns
        // *something* in a tree that has the current model as a root. So, for each component that
        // pops out of `getComponentList`, crawl "up" to the root. If we encounter something
        // interesting (e.g. a `Muscle`) then we tag the geometry against that component, rather
        // than the component that is rendering.

        geometry.clear();

        auto on_append =
            [&](osmv::ModelDrawlistOnAppendFlags f, OpenSim::Component const*& c, osmv::Raw_mesh_instance&) {
                if (f & osmv::ModelDrawlistOnAppendFlags_IsStatic and
                    not(flags & ModelViewerGeometryFlags_CanInteractWithStaticDecorations)) {
                    c = nullptr;
                }

                if (f & ModelDrawlistOnAppendFlags_IsDynamic and
                    not(flags & ModelViewerGeometryFlags_CanInteractWithDynamicDecorations)) {
                    c = nullptr;
                }
            };

        ModelDrawlistGeneratorFlags draw_flags = ModelDrawlistGeneratorFlags_None;
        if (flags & ModelViewerGeometryFlags_DrawStaticDecorations) {
            draw_flags |= ModelDrawlistGeneratorFlags_GenerateStaticDecorations;
        }
        if (flags & ModelViewerGeometryFlags_DrawDynamicDecorations) {
            draw_flags |= ModelDrawlistGeneratorFlags_GenerateDynamicDecorations;
        }

        drawlist_generator.generate(model, state, geometry, on_append, draw_flags);

        geometry.optimize();
    }

    void apply_standard_rim_coloring(
        Labelled_model_drawlist& drawlist,
        OpenSim::Component const* hovered = nullptr,
        OpenSim::Component const* selected = nullptr) {

        if (selected == nullptr) {
            // replace with a senteniel because nullptr means "not assigned"
            // in the geometry list
            selected = reinterpret_cast<OpenSim::Component const*>(-1);
        }

        drawlist.for_each([selected, hovered](OpenSim::Component const* owner, Raw_mesh_instance& mi) {
            unsigned char rim_alpha;
            if (owner == selected) {
                rim_alpha = 255;
            } else if (hovered != nullptr and hovered == owner) {
                rim_alpha = 70;
            } else {
                rim_alpha = 0;
            }

            mi.set_rim_alpha(rim_alpha);
        });
    }
}

namespace osmv {
    struct Model_viewer_widget_impl final {
        Raw_renderer renderer;
        Labelled_model_drawlist geometry;
        Model_drawlist_generator drawlist_generator;

        int hovertest_x = -1;
        int hovertest_y = -1;
        OpenSim::Component const* hovered_component = nullptr;
        Polar_camera camera;
        glm::vec3 light_pos = {1.5f, 3.0f, 0.0f};
        glm::vec3 light_rgb = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
        glm::vec4 background_rgba = {0.89f, 0.89f, 0.89f, 1.0f};
        glm::vec4 rim_rgba = {1.0f, 0.4f, 0.0f, 0.85f};

        ModelViewerRecoloring recoloring = ModelViewerRecoloring_None;
        Raw_renderer_flags rendering_flags = RawRendererFlags_Default;
        bool mouse_over_render = false;

        Model_viewer_widget_impl() : renderer{Raw_renderer_config{100, 100, app().samples()}} {
        }

        gl::Texture_2d& draw(Raw_renderer_flags flags) {
            Raw_drawcall_params params;
            params.passthrough_hittest_x = hovertest_x;
            params.passthrough_hittest_y = hovertest_y;
            params.view_matrix = camera.view_matrix();
            params.projection_matrix = camera.projection_matrix(renderer.aspect_ratio());
            params.view_pos = camera.pos();
            params.light_pos = light_pos;
            params.light_rgb = light_rgb;
            params.background_rgba = background_rgba;
            params.rim_rgba = rim_rgba;
            params.flags = flags;
            if (app().is_in_debug_mode()) {
                params.flags |= RawRendererFlags_DrawDebugQuads;
            } else {
                params.flags &= ~RawRendererFlags_DrawDebugQuads;
            }

            // perform draw call
            Raw_drawcall_result result = renderer.draw(params, geometry.raw_drawlist());

            // post-draw: check if the hit-test passed
            // TODO:: optimized indices are from the previous frame, which might
            //        contain now-stale components
            hovered_component = geometry.component_from_passthrough(result.passthrough_result);

            return result.texture;
        }
    };
}

osmv::Model_viewer_widget::Model_viewer_widget() : impl{new Model_viewer_widget_impl{}} {
    OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
}

osmv::Model_viewer_widget::~Model_viewer_widget() noexcept {
    delete impl;
}

bool osmv::Model_viewer_widget::is_moused_over() const noexcept {
    return impl->mouse_over_render;
}

bool osmv::Model_viewer_widget::on_event(const SDL_Event& e) {
    if (not(impl->mouse_over_render or e.type == SDL_MOUSEBUTTONUP)) {
        return false;
    }

    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_w:
            impl->rendering_flags ^= RawRendererFlags_WireframeMode;
            return true;
        }
    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
        switch (e.button.button) {
        case SDL_BUTTON_LEFT:
            impl->camera.on_left_click_down();
            return true;
        case SDL_BUTTON_RIGHT:
            impl->camera.on_right_click_down();
            return true;
        }
    } else if (e.type == SDL_MOUSEBUTTONUP) {
        switch (e.button.button) {
        case SDL_BUTTON_LEFT:
            impl->camera.on_left_click_up();
            return true;
        case SDL_BUTTON_RIGHT:
            impl->camera.on_right_click_up();
            return true;
        }
    } else if (e.type == SDL_MOUSEMOTION) {
        glm::vec2 d = impl->renderer.dimensions();
        float aspect_ratio = d.x / d.y;
        float dx = static_cast<float>(e.motion.xrel) / d.x;
        float dy = static_cast<float>(e.motion.yrel) / d.y;
        impl->camera.on_mouse_motion(aspect_ratio, dx, dy);
    } else if (e.type == SDL_MOUSEWHEEL) {
        if (e.wheel.y > 0) {
            impl->camera.on_scroll_up();
        } else {
            impl->camera.on_scroll_down();
        }
        return true;
    }
    return false;
}

void osmv::Model_viewer_widget::draw(
    char const* panel_name,
    OpenSim::Model const& model,
    SimTK::State const& state,
    OpenSim::Component const** selected,
    OpenSim::Component const** hovered) {

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0, 0.0));
    if (ImGui::Begin(panel_name, nullptr, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Options")) {

                ImGui::Text("Selection logic:");

                ImGui::CheckboxFlags(
                    "coerce selection to muscle", &geometry_flags, ModelViewerGeometryFlags_CanOnlyInteractWithMuscles);

                ImGui::CheckboxFlags(
                    "can interact with static geometry",
                    &geometry_flags,
                    ModelViewerGeometryFlags_CanInteractWithStaticDecorations);

                ImGui::CheckboxFlags(
                    "can interact with dynamic geometry",
                    &geometry_flags,
                    ModelViewerGeometryFlags_CanInteractWithDynamicDecorations);

                ImGui::CheckboxFlags(
                    "draw dynamic geometry", &geometry_flags, ModelViewerGeometryFlags_DrawDynamicDecorations);

                ImGui::CheckboxFlags(
                    "draw static geometry", &geometry_flags, ModelViewerGeometryFlags_DrawStaticDecorations);

                ImGui::Separator();

                ImGui::Text("Graphical Options:");

                ImGui::CheckboxFlags("wireframe mode", &impl->rendering_flags, RawRendererFlags_WireframeMode);
                ImGui::CheckboxFlags("show normals", &impl->rendering_flags, RawRendererFlags_ShowMeshNormals);
                ImGui::CheckboxFlags("show floor", &impl->rendering_flags, RawRendererFlags_ShowFloor);
                ImGui::CheckboxFlags("draw rims", &impl->rendering_flags, RawRendererFlags_DrawRims);
                ImGui::CheckboxFlags("hit testing", &impl->rendering_flags, RawRendererFlags_PerformPassthroughHitTest);
                ImGui::CheckboxFlags(
                    "optimized hit testing",
                    &impl->rendering_flags,
                    RawRendererFlags_UseOptimizedButDelayed1FrameHitTest);
                ImGui::CheckboxFlags("draw scene geometry", &impl->rendering_flags, RawRendererFlags_DrawSceneGeometry);

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Scene")) {
                if (ImGui::Button("Left")) {
                    // assumes models tend to point upwards in Y and forwards in +X
                    // (so sidewards is theta == 0 or PI)
                    impl->camera.theta = pi_f;
                    impl->camera.phi = 0.0f;
                }
                ImGui::SameLine();
                if (ImGui::Button("Right")) {
                    // assumes models tend to point upwards in Y and forwards in +X
                    // (so sidewards is theta == 0 or PI)
                    impl->camera.theta = 0.0f;
                    impl->camera.phi = 0.0f;
                }

                if (ImGui::Button("Look top")) {
                    impl->camera.theta = 0.0f;
                    impl->camera.phi = pi_f / 2.0f;
                }
                ImGui::SameLine();
                if (ImGui::Button("Look bottom")) {
                    impl->camera.theta = 0.0f;
                    impl->camera.phi = 3.0f * (pi_f / 2.0f);
                }

                ImGui::NewLine();

                ImGui::SliderFloat("radius", &impl->camera.radius, 0.0f, 10.0f);
                ImGui::SliderFloat("theta", &impl->camera.theta, 0.0f, 2.0f * pi_f);
                ImGui::SliderFloat("phi", &impl->camera.phi, 0.0f, 2.0f * pi_f);
                ImGui::NewLine();
                ImGui::SliderFloat("pan_x", &impl->camera.pan.x, -100.0f, 100.0f);
                ImGui::SliderFloat("pan_y", &impl->camera.pan.y, -100.0f, 100.0f);
                ImGui::SliderFloat("pan_z", &impl->camera.pan.z, -100.0f, 100.0f);

                ImGui::Separator();

                ImGui::SliderFloat("light_x", &impl->light_pos.x, -30.0f, 30.0f);
                ImGui::SliderFloat("light_y", &impl->light_pos.y, -30.0f, 30.0f);
                ImGui::SliderFloat("light_z", &impl->light_pos.z, -30.0f, 30.0f);
                ImGui::ColorEdit3("light_color", reinterpret_cast<float*>(&impl->light_rgb));

                ImGui::EndMenu();
            }

            // muscle coloring
            {
                char buf[32];
                std::snprintf(buf, sizeof(buf), "something longer than options");
                ImVec2 font_dims = ImGui::CalcTextSize(buf);

                static constexpr char const* recoloring_strs[ModelViewerRecoloring_COUNT] = {
                    "default muscle coloring",
                    "color muscles by strain",
                    "color muscles by length",
                };
                ImGui::Dummy(ImVec2{5.0f, 0.0f});
                ImGui::SetNextItemWidth(font_dims.x);
                ImGui::Combo("##musclecoloring", &impl->recoloring, recoloring_strs, ModelViewerRecoloring_COUNT);
            }

            ImGui::EndMenuBar();
        }

        // put the renderer in a child window that can't be moved to prevent accidental dragging
        if (ImGui::BeginChild("##child", ImVec2(0, 0), false, ImGuiWindowFlags_NoMove)) {

            // generate OpenSim scene geometry
            generate_geometry(geometry_flags, impl->geometry, impl->drawlist_generator, model, state);

            // perform screen-specific geometry fixups
            if (geometry_flags & ModelViewerGeometryFlags_CanOnlyInteractWithMuscles) {
                impl->geometry.for_each(
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
            }

            if (impl->recoloring == ModelViewerRecoloring_Strain) {
                impl->geometry.for_each([&state](OpenSim::Component const* c, Raw_mesh_instance& mi) {
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

            if (impl->recoloring == ModelViewerRecoloring_Length) {
                impl->geometry.for_each([&state](OpenSim::Component const* c, Raw_mesh_instance& mi) {
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

            if (impl->rendering_flags & RawRendererFlags_DrawRims) {
                apply_standard_rim_coloring(impl->geometry, *hovered, *selected);
            }

            // draw the scene to an OpenGL texture
            auto dims = ImGui::GetContentRegionAvail();

            if (dims.x >= 1 and dims.y >= 1) {
                impl->renderer.change_config(
                    Raw_renderer_config{static_cast<int>(dims.x), static_cast<int>(dims.y), app().samples()});

                gl::Texture_2d& render = impl->draw(impl->rendering_flags);

                {
                    // required by ImGui::Image
                    //
                    // UV coords: ImGui::Image uses different texture coordinates from the renderer
                    //            (specifically, Y is reversed)
                    void* texture_handle = reinterpret_cast<void*>(static_cast<uintptr_t>(render.raw_handle()));
                    ImVec2 image_dimensions{dims.x, dims.y};
                    ImVec2 uv0{0, 1};
                    ImVec2 uv1{1, 0};

                    ImVec2 cp = ImGui::GetCursorPos();
                    ImVec2 mp = ImGui::GetMousePos();
                    ImVec2 wp = ImGui::GetWindowPos();

                    ImGui::Image(texture_handle, image_dimensions, uv0, uv1);

                    impl->mouse_over_render = ImGui::IsItemHovered();

                    impl->hovertest_x = static_cast<int>((mp.x - wp.x) - cp.x);
                    // y is reversed (OpenGL coords, not screen)
                    impl->hovertest_y = static_cast<int>(dims.y - ((mp.y - wp.y) - cp.y));
                }

                // overlay: if the user is hovering over a component, write the component's name
                //          next to the mouse
                if (impl->hovered_component) {
                    OpenSim::Component const& c = *impl->hovered_component;
                    sdl::Mouse_state m = sdl::GetMouseState();
                    ImVec2 pos{static_cast<float>(m.x + 20), static_cast<float>(m.y)};
                    ImGui::GetBackgroundDrawList()->AddText(pos, 0xff0000ff, c.getName().c_str());
                }

                *hovered = impl->hovered_component;
            }

            ImGui::EndChild();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
