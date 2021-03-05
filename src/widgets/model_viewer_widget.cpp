#include "model_viewer_widget.hpp"

#include "src/3d/gl.hpp"
#include "src/3d/gpu_cache.hpp"
#include "src/3d/mesh_instance.hpp"
#include "src/3d/polar_camera.hpp"
#include "src/3d/render_target.hpp"
#include "src/3d/renderer.hpp"
#include "src/application.hpp"
#include "src/constants.hpp"
#include "src/opensim_bindings/model_drawlist.hpp"
#include "src/opensim_bindings/model_drawlist_generator.hpp"
#include "src/utils/sdl_wrapper.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <imgui.h>

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

using namespace osmv;

namespace {

    void apply_standard_rim_coloring(
        Model_drawlist& drawlist,
        OpenSim::Component const* hovered = nullptr,
        OpenSim::Component const* selected = nullptr) {

        if (selected == nullptr) {
            // replace with a senteniel because nullptr means "not assigned"
            // in the geometry list
            selected = reinterpret_cast<OpenSim::Component const*>(-1);
        }

        drawlist.for_each([selected, hovered](OpenSim::Component const* owner, Mesh_instance& mi) {
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

struct osmv::Model_viewer_widget::Impl final {
    Gpu_cache& cache;
    Render_target render_target{100, 100, Application::current().samples()};
    Renderer renderer;
    Model_drawlist geometry;

    int hovertest_x = -1;
    int hovertest_y = -1;
    OpenSim::Component const* hovered_component = nullptr;
    Polar_camera camera;
    glm::vec3 light_pos = {1.5f, 3.0f, 0.0f};
    glm::vec3 light_rgb = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
    glm::vec4 background_rgba = {0.89f, 0.89f, 0.89f, 1.0f};
    glm::vec4 rim_rgba = {1.0f, 0.4f, 0.0f, 0.85f};

    ModelViewerWidgetFlags flags;
    DrawcallFlags rendering_flags = RawRendererFlags_Default;

    bool mouse_over_render = false;

    Impl(Gpu_cache& _cache, ModelViewerWidgetFlags _flags) : cache{_cache}, flags{_flags} {
    }

    gl::Texture_2d& draw(DrawcallFlags drawflags) {
        Raw_drawcall_params params;
        params.passthrough_hittest_x = hovertest_x;
        params.passthrough_hittest_y = hovertest_y;
        params.view_matrix = camera.view_matrix();
        params.projection_matrix = camera.projection_matrix(render_target.aspect_ratio());
        params.view_pos = camera.pos();
        params.light_pos = light_pos;
        params.light_rgb = light_rgb;
        params.background_rgba = background_rgba;
        params.rim_rgba = rim_rgba;
        params.flags = drawflags;
        if (Application::current().is_in_debug_mode()) {
            params.flags |= RawRendererFlags_DrawDebugQuads;
        } else {
            params.flags &= ~RawRendererFlags_DrawDebugQuads;
        }

        // draw scene
        Passthrough_data passthrough = renderer.draw(cache.storage, params, geometry.raw_drawlist(), render_target);

        // post-draw: check if the hit-test passed
        // TODO:: optimized indices are from the previous frame, which might
        //        contain now-stale components
        hovered_component = geometry.component_from_passthrough(passthrough);

        return render_target.main();
    }
};

Model_viewer_widget::Model_viewer_widget(Gpu_cache& cache, ModelViewerWidgetFlags flags) :
    impl{new Impl{cache, flags}} {
    OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
}

Model_viewer_widget::~Model_viewer_widget() noexcept {
    delete impl;
}

bool Model_viewer_widget::is_moused_over() const noexcept {
    return impl->mouse_over_render;
}

bool Model_viewer_widget::on_event(const SDL_Event& e) {
    if (not(impl->mouse_over_render or e.type == SDL_MOUSEBUTTONUP)) {
        return false;
    }

    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_w:
            impl->rendering_flags ^= DrawcallFlags_WireframeMode;
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
        glm::vec2 d = impl->render_target.dimensions();
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

void Model_viewer_widget::draw(
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
                    "coerce selection to muscle", &impl->flags, ModelViewerWidgetFlags_CanOnlyInteractWithMuscles);

                ImGui::CheckboxFlags(
                    "draw dynamic geometry", &impl->flags, ModelViewerWidgetFlags_DrawDynamicDecorations);

                ImGui::CheckboxFlags(
                    "draw static geometry", &impl->flags, ModelViewerWidgetFlags_DrawStaticDecorations);

                ImGui::CheckboxFlags("draw frames", &impl->flags, ModelViewerWidgetFlags_DrawFrames);

                ImGui::CheckboxFlags("draw debug geometry", &impl->flags, ModelViewerWidgetFlags_DrawDebugGeometry);
                ImGui::CheckboxFlags("draw labels", &impl->flags, ModelViewerWidgetFlags_DrawLabels);

                ImGui::Separator();

                ImGui::Text("Graphical Options:");

                ImGui::CheckboxFlags("wireframe mode", &impl->rendering_flags, DrawcallFlags_WireframeMode);
                ImGui::CheckboxFlags("show normals", &impl->rendering_flags, DrawcallFlags_ShowMeshNormals);
                ImGui::CheckboxFlags("draw rims", &impl->rendering_flags, DrawcallFlags_DrawRims);
                ImGui::CheckboxFlags("hit testing", &impl->rendering_flags, RawRendererFlags_PerformPassthroughHitTest);
                ImGui::CheckboxFlags(
                    "optimized hit testing",
                    &impl->rendering_flags,
                    RawRendererFlags_UseOptimizedButDelayed1FrameHitTest);
                ImGui::CheckboxFlags("draw scene geometry", &impl->rendering_flags, RawRendererFlags_DrawSceneGeometry);
                ImGui::CheckboxFlags("draw floor", &impl->flags, ModelViewerWidgetFlags_DrawFloor);
                ImGui::CheckboxFlags("optimize draw order", &impl->flags, ModelViewerWidgetFlags_OptimizeDrawOrder);
                ImGui::CheckboxFlags(
                    "use instanced (optimized) renderer",
                    &impl->rendering_flags,
                    RawRendererFlags_UseInstancedRenderer);

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

                static constexpr std::array<char const*, 3> options = {
                    "default muscle coloring", "color muscles by strain", "color muscles by length"};

                ImGui::Dummy(ImVec2{5.0f, 0.0f});
                ImGui::SetNextItemWidth(font_dims.x);
                int choice = 0;
                if (impl->flags & ModelViewerWidgetFlags_RecolorMusclesByStrain) {
                    choice = 1;
                } else if (impl->flags & ModelViewerWidgetFlags_RecolorMusclesByLength) {
                    choice = 2;
                }

                if (ImGui::Combo("##musclecoloring", &choice, options.data(), options.size())) {
                    switch (choice) {
                    case 0:
                        impl->flags |= ModelViewerWidgetFlags_DefaultMuscleColoring;
                        impl->flags &= ~ModelViewerWidgetFlags_RecolorMusclesByLength;
                        impl->flags &= ~ModelViewerWidgetFlags_RecolorMusclesByStrain;
                        break;
                    case 1:
                        impl->flags &= ~ModelViewerWidgetFlags_DefaultMuscleColoring;
                        impl->flags &= ~ModelViewerWidgetFlags_RecolorMusclesByLength;
                        impl->flags |= ModelViewerWidgetFlags_RecolorMusclesByStrain;
                        break;
                    case 2:
                        impl->flags &= ~ModelViewerWidgetFlags_DefaultMuscleColoring;
                        impl->flags |= ModelViewerWidgetFlags_RecolorMusclesByLength;
                        impl->flags &= ~ModelViewerWidgetFlags_RecolorMusclesByStrain;
                        break;
                    }
                }
            }

            ImGui::EndMenuBar();
        }

        // put the renderer in a child window that can't be moved to prevent accidental dragging
        if (ImGui::BeginChild("##child", ImVec2(0, 0), false, ImGuiWindowFlags_NoMove)) {

            // generate OpenSim scene geometry
            {
                impl->geometry.clear();
                ModelDrawlistFlags flags = ModelDrawlistFlags_None;
                if (impl->flags & ModelViewerWidgetFlags_DrawStaticDecorations) {
                    flags |= ModelDrawlistFlags_StaticGeometry;
                }
                if (impl->flags & ModelViewerWidgetFlags_DrawDynamicDecorations) {
                    flags |= ModelDrawlistFlags_DynamicGeometry;
                }

                OpenSim::ModelDisplayHints cpy = model.getDisplayHints();

                cpy.upd_show_frames() = impl->flags & ModelViewerWidgetFlags_DrawFrames;
                cpy.upd_show_debug_geometry() = impl->flags & ModelViewerWidgetFlags_DrawDebugGeometry;
                cpy.upd_show_labels() = impl->flags & ModelViewerWidgetFlags_DrawLabels;

                generate_decoration_drawlist(model, state, cpy, impl->cache, impl->geometry, flags);
            }

            // HACK: add floor in
            if (impl->flags & ModelViewerWidgetFlags_DrawFloor) {
                glm::mat4 model_mtx = []() {
                    glm::mat4 rv = glm::identity<glm::mat4>();

                    // OpenSim: might contain floors at *exactly* Y = 0.0, so shift the chequered
                    // floor down *slightly* to prevent Z fighting from planes rendered from the
                    // model itself (the contact planes, etc.)
                    rv = glm::translate(rv, {0.0f, -0.001f, 0.0f});
                    rv = glm::rotate(rv, osmv::pi_f / 2, {-1.0, 0.0, 0.0});
                    rv = glm::scale(rv, {100.0f, 100.0f, 0.0f});

                    return rv;
                }();

                Rgba32 color{glm::vec4{1.0f, 0.0f, 1.0f, 1.0f}};
                impl->geometry.emplace_back(
                    nullptr, model_mtx, color, impl->cache.floor_quad, impl->cache.chequered_texture);
            }

            if (impl->flags & ModelViewerWidgetFlags_OptimizeDrawOrder) {
                optimize(impl->geometry);
            }

            // perform screen-specific geometry fixups
            if (impl->flags & ModelViewerWidgetFlags_CanOnlyInteractWithMuscles) {
                impl->geometry.for_each(
                    [&model](OpenSim::Component const*& associated_component, Mesh_instance const&) {
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

            if (impl->flags & ModelViewerWidgetFlags_RecolorMusclesByStrain) {
                impl->geometry.for_each([&state](OpenSim::Component const* c, Mesh_instance& mi) {
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

            if (impl->flags & ModelViewerWidgetFlags_RecolorMusclesByLength) {
                impl->geometry.for_each([&state](OpenSim::Component const* c, Mesh_instance& mi) {
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

            if (impl->rendering_flags & DrawcallFlags_DrawRims) {
                apply_standard_rim_coloring(impl->geometry, *hovered, *selected);
            }

            // draw the scene to an OpenGL texture
            auto dims = ImGui::GetContentRegionAvail();

            if (dims.x >= 1 and dims.y >= 1) {
                impl->render_target.reconfigure(
                    static_cast<int>(dims.x), static_cast<int>(dims.y), Application::current().samples());

                gl::Texture_2d& render = impl->draw(impl->rendering_flags);
                bool mouse_right_clicked_render = false;
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
                    mouse_right_clicked_render = ImGui::IsItemClicked(ImGuiMouseButton_Right);

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

                if (impl->hovered_component and mouse_right_clicked_render) {
                    *selected = impl->hovered_component;
                }
            }

            ImGui::EndChild();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
