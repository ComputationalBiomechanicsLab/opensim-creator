#include "component_3d_viewer.hpp"

#include "src/3d/cameras.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/3d.hpp"
#include "src/application.hpp"
#include "src/constants.hpp"
#include "src/opensim_bindings/component_drawlist.hpp"
#include "src/utils/sdl_wrapper.hpp"
#include "src/log.hpp"

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

using namespace osc;

struct osc::Component_3d_viewer::Impl final {
    // the buffers + textures that the backend renderer renders the scene into
    Render_target render_target{16, 16, Application::current().samples()};

    // a list of mesh instances the backend renderer should draw
    //
    // recycled per frame (by scanning over the model)
    Component_drawlist drawlist;

    // X+Y locations in screen that the backend should test for hover collisions
    int hovertest_x = -1;
    int hovertest_y = -1;

    // the currently-hovered component, as determined from the backend hovertest
    // + the drawlist
    OpenSim::Component const* hovered_component = nullptr;

    // the main viewport camera
    Polar_perspective_camera camera;

    // direction of the main scene light
    //
    // the light is a standard directional (rather than point) light
    glm::vec3 light_dir = {-0.34f, -0.25f, 0.05f};

    // color of the light
    //
    // combined with scene color in backend shading calculations
    glm::vec3 light_rgb = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};

    // color of the scene background
    glm::vec4 background_rgba = {0.89f, 0.89f, 0.89f, 1.0f};

    // color of rim highlights
    //
    // rim highlights are the thin lines that appear around hovered/selected items
    glm::vec4 rim_rgba = {1.0f, 0.4f, 0.0f, 0.85f};

    // user-facing rendering flags passed in at construction time
    //
    // these affect how/what is rendered
    Component3DViewerFlags flags;

    // backend rendering flags
    DrawcallFlags rendering_flags = DrawcallFlags_Default;

    // set to true at the end of a drawcall if the mouse is known to be over the
    // render rect
    //
    // note: because it's set *at the end*, this is technically one frame old
    bool mouse_over_render = false;

    // set to true at the end of a drawcall if the mouse is known to have left-clicked
    // the render
    bool is_left_clicked = false;

    // set to true at the end of a drawcall if the mouse is known to have right-clicked
    // the render
    bool is_right_clicked = false;

    Impl(Component3DViewerFlags _flags) : flags{_flags} {
    }
};

static bool is_subcomponent_of(OpenSim::Component const* parent, OpenSim::Component const* c) {
    for (auto* ptr = c; ptr != nullptr; ptr = ptr->hasOwner() ? &ptr->getOwner() : nullptr) {
        if (ptr == parent) {
            return true;
        }
    }
    return false;
}

static void apply_standard_rim_coloring(
    Component_drawlist& drawlist,
    OpenSim::Component const* hovered = nullptr,
    OpenSim::Component const* selected = nullptr) {

    drawlist.for_each([selected, hovered](OpenSim::Component const* c, Mesh_instance& mi) {
        GLubyte rim_alpha;

        if (is_subcomponent_of(selected, c)) {
            rim_alpha = 255;
        } else if (is_subcomponent_of(hovered, c)) {
            rim_alpha = 70;
        } else {
            rim_alpha = 0;
        }

        mi.passthrough.rim_alpha = rim_alpha;
    });
}

static bool action_toggle_wireframe_mode(osc::Component_3d_viewer::Impl& impl) {
    impl.rendering_flags ^= DrawcallFlags_WireframeMode;
    return true;
}

static bool action_step_zoom_in(osc::Component_3d_viewer::Impl& impl) {
    if (impl.camera.radius >= 0.1f) {
        impl.camera.radius *= 0.9f;
        return true;
    } else {
        return false;
    }
}

static bool action_step_zoom_out(osc::Component_3d_viewer::Impl& impl) {
    if (impl.camera.radius < 100.0f) {
        impl.camera.radius /= 0.9f;
        return true;
    } else {
        return false;
    }
}

static bool on_event(osc::Component_3d_viewer::Impl& impl, SDL_Event const& e) {
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_w) {
        return action_toggle_wireframe_mode(impl);
    } else if (e.type == SDL_MOUSEWHEEL) {
        return e.wheel.y > 0 ? action_step_zoom_in(impl) : action_step_zoom_out(impl);
    } else {
        return false;
    }
}

static void update_camera_from_user_input(osc::Component_3d_viewer::Impl& impl) {
    // update camera
    //
    // At the moment, all camera updates happen via the middle-mouse. The mouse
    // input is designed to mirror Blender fairly closely (because, imho, it has
    // decent UX for this problem space)
    if (impl.mouse_over_render && ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
        ImVec2 screendims = impl.render_target.dimensions();
        float aspect_ratio = screendims.x / screendims.y;
        ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle, 0.0f);
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);

        // relative vectors
        float rdx = delta.x/screendims.x;
        float rdy = delta.y/screendims.y;

        if (ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT)) {
            // shift + middle-mouse: pan
            pan(impl.camera, aspect_ratio, {rdx, rdy});
        } else if (ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL)) {
            // shift + middle-mouse: zoom
            impl.camera.radius *= 1.0f + rdy;
        } else {
            // middle: mouse drag
            drag(impl.camera, {rdx, rdy});
        }
    }
}

static void draw_viewport_main_menu(osc::Component_3d_viewer::Impl& impl) {
    if (!ImGui::BeginMenuBar()) {
        return;  // menu bar is not visible
    }

    if (ImGui::BeginMenu("Options")) {

        ImGui::Text("Selection logic:");

        ImGui::CheckboxFlags(
            "coerce selection to muscle", &impl.flags, Component3DViewerFlags_CanOnlyInteractWithMuscles);

        ImGui::CheckboxFlags(
            "draw dynamic geometry", &impl.flags, Component3DViewerFlags_DrawDynamicDecorations);

        ImGui::CheckboxFlags(
            "draw static geometry", &impl.flags, Component3DViewerFlags_DrawStaticDecorations);

        ImGui::CheckboxFlags("draw frames", &impl.flags, Component3DViewerFlags_DrawFrames);

        ImGui::CheckboxFlags("draw debug geometry", &impl.flags, Component3DViewerFlags_DrawDebugGeometry);
        ImGui::CheckboxFlags("draw labels", &impl.flags, Component3DViewerFlags_DrawLabels);

        ImGui::Separator();

        ImGui::Text("Graphical Options:");

        ImGui::CheckboxFlags("wireframe mode", &impl.rendering_flags, DrawcallFlags_WireframeMode);
        ImGui::CheckboxFlags("show normals", &impl.rendering_flags, DrawcallFlags_ShowMeshNormals);
        ImGui::CheckboxFlags("draw rims", &impl.rendering_flags, DrawcallFlags_DrawRims);
        ImGui::CheckboxFlags("hit testing", &impl.rendering_flags, DrawcallFlags_PerformPassthroughHitTest);
        ImGui::CheckboxFlags(
            "optimized hit testing",
            &impl.rendering_flags,
            DrawcallFlags_UseOptimizedButDelayed1FrameHitTest);
        ImGui::CheckboxFlags("draw scene geometry", &impl.rendering_flags, DrawcallFlags_DrawSceneGeometry);
        ImGui::CheckboxFlags("draw floor", &impl.flags, Component3DViewerFlags_DrawFloor);
        ImGui::CheckboxFlags("show XZ grid", &impl.flags, Component3DViewerFlags_DrawXZGrid);
        ImGui::CheckboxFlags("show XY grid", &impl.flags, Component3DViewerFlags_DrawXYGrid);
        ImGui::CheckboxFlags("show YZ grid", &impl.flags, Component3DViewerFlags_DrawYZGrid);
        ImGui::CheckboxFlags("show alignment axes", &impl.flags, Component3DViewerFlags_DrawAlignmentAxes);
        ImGui::CheckboxFlags("optimize draw order", &impl.flags, Component3DViewerFlags_OptimizeDrawOrder);
        ImGui::CheckboxFlags(
            "use instanced (optimized) renderer",
            &impl.rendering_flags,
            DrawcallFlags_UseInstancedRenderer);

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Scene")) {
        if (ImGui::Button("Top")) {
            impl.camera.theta = 0.0f;
            impl.camera.phi = pi_f / 2.0f;
        }

        if (ImGui::Button("Left")) {
            // assumes models tend to point upwards in Y and forwards in +X
            // (so sidewards is theta == 0 or PI)
            impl.camera.theta = pi_f;
            impl.camera.phi = 0.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Right")) {
            // assumes models tend to point upwards in Y and forwards in +X
            // (so sidewards is theta == 0 or PI)
            impl.camera.theta = 0.0f;
            impl.camera.phi = 0.0f;
        }

        if (ImGui::Button("Bottom")) {
            impl.camera.theta = 0.0f;
            impl.camera.phi = 3.0f * (pi_f / 2.0f);
        }

        ImGui::NewLine();

        ImGui::SliderFloat("radius", &impl.camera.radius, 0.0f, 10.0f);
        ImGui::SliderFloat("theta", &impl.camera.theta, 0.0f, 2.0f * pi_f);
        ImGui::SliderFloat("phi", &impl.camera.phi, 0.0f, 2.0f * pi_f);
        ImGui::InputFloat("fov", &impl.camera.fov);
        ImGui::InputFloat("znear", &impl.camera.znear);
        ImGui::InputFloat("zfar", &impl.camera.zfar);
        ImGui::NewLine();
        ImGui::SliderFloat("pan_x", &impl.camera.pan.x, -100.0f, 100.0f);
        ImGui::SliderFloat("pan_y", &impl.camera.pan.y, -100.0f, 100.0f);
        ImGui::SliderFloat("pan_z", &impl.camera.pan.z, -100.0f, 100.0f);

        ImGui::Separator();

        ImGui::SliderFloat("light_dir_x", &impl.light_dir.x, -1.0f, 1.0f);
        ImGui::SliderFloat("light_dir_y", &impl.light_dir.y, -1.0f, 1.0f);
        ImGui::SliderFloat("light_dir_z", &impl.light_dir.z, -1.0f, 1.0f);
        ImGui::ColorEdit3("light_color", reinterpret_cast<float*>(&impl.light_rgb));
        ImGui::ColorEdit3("background color", reinterpret_cast<float*>(&impl.background_rgba));

        ImGui::EndMenu();
    }

    // dropdown: muscle coloring
    {
        static constexpr std::array<char const*, 3> options = {"default", "by strain", "by length"};

        char buf[32];
        std::snprintf(buf, sizeof(buf), "something longer");
        ImVec2 font_dims = ImGui::CalcTextSize(buf);

        ImGui::Dummy(ImVec2{5.0f, 0.0f});
        ImGui::SetNextItemWidth(font_dims.x);
        int choice = 0;
        if (impl.flags & Component3DViewerFlags_RecolorMusclesByStrain) {
            choice = 1;
        } else if (impl.flags & Component3DViewerFlags_RecolorMusclesByLength) {
            choice = 2;
        }

        if (ImGui::Combo("muscle coloring", &choice, options.data(), static_cast<int>(options.size()))) {
            switch (choice) {
            case 0:
                impl.flags |= Component3DViewerFlags_DefaultMuscleColoring;
                impl.flags &= ~Component3DViewerFlags_RecolorMusclesByLength;
                impl.flags &= ~Component3DViewerFlags_RecolorMusclesByStrain;
                break;
            case 1:
                impl.flags &= ~Component3DViewerFlags_DefaultMuscleColoring;
                impl.flags &= ~Component3DViewerFlags_RecolorMusclesByLength;
                impl.flags |= Component3DViewerFlags_RecolorMusclesByStrain;
                break;
            case 2:
                impl.flags &= ~Component3DViewerFlags_DefaultMuscleColoring;
                impl.flags |= Component3DViewerFlags_RecolorMusclesByLength;
                impl.flags &= ~Component3DViewerFlags_RecolorMusclesByStrain;
                break;
            }
        }
    }

    ImGui::EndMenuBar();
}

static gl::Texture_2d& perform_backend_render_drawcall(osc::Component_3d_viewer::Impl& impl) {
    Render_params params;
    params.hittest.x = impl.hovertest_x;
    params.hittest.y = impl.hovertest_y;
    params.view_matrix = view_matrix(impl.camera);
    params.projection_matrix = projection_matrix(impl.camera, impl.render_target.aspect_ratio());
    params.view_pos = pos(impl.camera);
    params.light_dir = impl.light_dir;
    params.light_rgb = impl.light_rgb;
    params.background_rgba = impl.background_rgba;
    params.rim_rgba = impl.rim_rgba;
    params.flags = impl.rendering_flags;
    if (Application::current().is_in_debug_mode()) {
        params.flags |= DrawcallFlags_DrawDebugQuads;
    } else {
        params.flags &= ~DrawcallFlags_DrawDebugQuads;
    }

    // draw scene
    draw_scene(Application::current().get_gpu_storage(), params, impl.drawlist.raw_drawlist(), impl.render_target);
    impl.hovered_component = impl.drawlist.component_from_passthrough(impl.render_target.hittest_result);

    return impl.render_target.main();
}

static void draw_viewport_3d_render(
        osc::Component_3d_viewer::Impl& impl,
        OpenSim::Component const& model,
        OpenSim::ModelDisplayHints const& mdh,
        SimTK::State const& state,
        OpenSim::Component const* current_selection,
        OpenSim::Component const* current_hover,
        Component3DViewerResponse& resp) {

    // put the renderer in a child window that can't be moved to prevent accidental dragging
    if (!ImGui::BeginChild("##child", ImVec2(0, 0), false, ImGuiWindowFlags_NoMove)) {
        return;  // panel not visible
    }

    // this renderer pulls cached assets (e.g. meshes) from GPU storage
    GPU_storage& cache = Application::current().get_gpu_storage();

    // generate OpenSim scene geometry
    {
        impl.drawlist.clear();
        ModelDrawlistFlags flags = ModelDrawlistFlags_None;
        if (impl.flags & Component3DViewerFlags_DrawStaticDecorations) {
            flags |= ModelDrawlistFlags_StaticGeometry;
        }
        if (impl.flags & Component3DViewerFlags_DrawDynamicDecorations) {
            flags |= ModelDrawlistFlags_DynamicGeometry;
        }

        OpenSim::ModelDisplayHints cpy{mdh};

        cpy.upd_show_frames() = impl.flags & Component3DViewerFlags_DrawFrames;
        cpy.upd_show_debug_geometry() = impl.flags & Component3DViewerFlags_DrawDebugGeometry;
        cpy.upd_show_labels() = impl.flags & Component3DViewerFlags_DrawLabels;

        generate_component_decorations(model, state, cpy, cache, impl.drawlist, flags);
    }

    // if applicable, draw chequered floor
    if (impl.flags & Component3DViewerFlags_DrawFloor) {
        Mesh_instance mi;

        mi.model_xform = []() {
            // rotate from XY (+Z dir) to ZY (+Y dir)
            glm::mat4 rv = glm::rotate(glm::mat4{1.0f}, -pi_f/2.0f, {1.0f, 0.0f, 0.0f});

            // make floor extend far in all directions
            rv = glm::scale(glm::mat4{1.0f}, {100.0f, 1.0f, 100.0f}) * rv;

            // lower slightly, so that it doesn't conflict with OpenSim model planes
            // that happen to lie at Z==0
            rv = glm::translate(glm::mat4{1.0f}, {0.0f, -0.001f, 0.0f}) * rv;

            return rv;
        }();

        mi.normal_xform = normal_matrix(mi.model_xform);

        // drawn as a quad
        mi.meshidx = cache.floor_quad_idx;

        // that is textured with the chequer texture
        mi.texidx = cache.chequer_idx;

        // and isn't subject to shading (from the light)
        mi.flags.set_skip_shading();

        impl.drawlist.push_back(nullptr, mi);
    }

    // if applicable, draw a XY (worldspace) grid
    if (impl.flags & Component3DViewerFlags_DrawXYGrid) {
        Mesh_instance mi;

        mi.model_xform = []() {
            // scale from [-1.0f, +1.0f] to [-1.25f, +1.25f] so that each cell
            // has dimensions (0.1f, 0.1f) in worldspace
            glm::mat4 rv = glm::scale(glm::mat4{1.0f}, {1.25f, 1.25f, 1.0f});

            // translate from [-1.25f, +1.25f] to [0.0f, +2.5f] so that the grid
            // is entirely in +Y, which is where OpenSim models are typically built
            rv = glm::translate(glm::mat4{1.0f}, {0.0f, 1.25f, 0.0f}) * rv;

            return rv;
        }();

        mi.normal_xform = normal_matrix(mi.model_xform);
        mi.rgba = {0xb2, 0xb2, 0xb2, 0x26};
        mi.meshidx = cache.grid_25x25_idx;
        mi.flags.set_draw_lines();
        mi.flags.set_skip_shading();
        impl.drawlist.push_back(nullptr, mi);
    }

    // if applicable, draw a XZ (worldspace) grid
    if (impl.flags & Component3DViewerFlags_DrawXZGrid) {
        Mesh_instance mi;

        mi.model_xform = []() {
            // rotate from XY (+Z dir) to XZ (+Y dir)
            glm::mat4 rv = glm::rotate(glm::mat4{1.0f}, -osc::pi_f/2.0f, {1.0f, 0.0f, 0.0f});

            // rescale from [-1.0f, +1.0f] to [-1.25f, +1.25f] so that each cell
            // has dimensions (0.1f, 0.1f) in worldspace
            rv = glm::scale(glm::mat4{1.0f}, {1.25f, 1.0f, 1.25f}) * rv;

            return rv;
        }();

        mi.normal_xform = normal_matrix(mi.model_xform);
        mi.rgba = {0xb2, 0xb2, 0xb2, 0x26};
        mi.meshidx = cache.grid_25x25_idx;
        mi.flags.set_draw_lines();
        mi.flags.set_skip_shading();
        impl.drawlist.push_back(nullptr, mi);
    }

    // if applicable, draw a YZ (worldspace) grid
    if (impl.flags & Component3DViewerFlags_DrawYZGrid) {
        Mesh_instance mi;

        mi.model_xform = []() {
            // rotate from XY (+Z dir) to YZ (+X dir)
            glm::mat4 rv = glm::rotate(glm::mat4{1.0f}, osc::pi_f/2.0f, {0.0f, 1.0f, 0.0f});

            // rescale from [-1.0f, +1.0f] to [-1.25f, +1.25f] so that each cell
            // has dimensions (0.1f, 0.1f) in worldspace
            rv = glm::scale(glm::mat4{1.0f}, {1.0f, 1.25f, 1.25f}) * rv;

            // translate from [-1.25f, +1.25f] to [0.0f, +2.5f] so that the grid
            // is entirely in +Y, which is where OpenSim models are typically built
            rv = glm::translate(glm::mat4{1.0f}, {0.0f, 1.25f, 0.0f}) * rv;

            return rv;
        }();

        mi.normal_xform = normal_matrix(mi.model_xform);
        mi.rgba = {0xb2, 0xb2, 0xb2, 0x26};
        mi.meshidx = cache.grid_25x25_idx;
        mi.flags.set_draw_lines();
        mi.flags.set_skip_shading();
        impl.drawlist.push_back(nullptr, mi);
    }

    // if applicable, draw alignment axes
    //
    // these are the little axes overlays that appear in a corner of the viewport and help
    // the user orient their screen
    if (impl.flags & Component3DViewerFlags_DrawAlignmentAxes) {
        glm::mat4 model2view = view_matrix(impl.camera);

        // we only care about rotation of the axes, not scaling
        model2view[3] = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f};

        // rescale + translate the y-line vertices
        glm::mat4 make_line_one_sided = glm::translate(glm::identity<glm::mat4>(), glm::vec3{0.0f, 1.0f, 0.0f});
        glm::mat4 scaler = glm::scale(glm::identity<glm::mat4>(), glm::vec3{0.025f});
        glm::mat4 translator = glm::translate(glm::identity<glm::mat4>(), glm::vec3{-0.95f, -0.95f, 0.0f});
        glm::mat4 base_model_mtx = translator * scaler * model2view;

        Instance_flags flags;
        flags.set_skip_shading();
        flags.set_skip_vp();
        flags.set_draw_lines();

        // y axis
        {
            Mesh_instance mi;
            mi.model_xform = base_model_mtx * make_line_one_sided;
            mi.normal_xform = normal_matrix(mi.model_xform);
            mi.rgba = {0x00, 0xff, 0x00, 0xff};
            mi.meshidx = cache.yline_idx;
            mi.flags = flags;
            impl.drawlist.push_back(nullptr, mi);
        }

        // x axis
        {
            glm::mat4 rotate_y_to_x =
                glm::rotate(glm::identity<glm::mat4>(), pi_f / 2.0f, glm::vec3{0.0f, 0.0f, -1.0f});

            Mesh_instance mi;
            mi.model_xform = base_model_mtx * rotate_y_to_x * make_line_one_sided;
            mi.normal_xform = normal_matrix(mi.model_xform);
            mi.rgba = {0xff, 0x00, 0x00, 0xff};
            mi.meshidx = cache.yline_idx;
            mi.flags = flags;
            impl.drawlist.push_back(nullptr, mi);
        }

        // z axis
        {
            glm::mat4 rotate_y_to_z =
                glm::rotate(glm::identity<glm::mat4>(), pi_f / 2.0f, glm::vec3{1.0f, 0.0f, 0.0f});

            Mesh_instance mi;
            mi.model_xform = base_model_mtx * rotate_y_to_z * make_line_one_sided;
            mi.normal_xform = normal_matrix(mi.model_xform);
            mi.rgba = {0x00, 0x00, 0xff, 0xff};
            mi.meshidx = cache.yline_idx;
            mi.flags = flags;
            impl.drawlist.push_back(nullptr, mi);
        }
    }

    // if applicable, optimize the drawlist for optimal rendering
    if (impl.flags & Component3DViewerFlags_OptimizeDrawOrder) {
        optimize(impl.drawlist);
    }

    // perform screen-specific geometry fixups
    if (impl.flags & Component3DViewerFlags_CanOnlyInteractWithMuscles) {
        impl.drawlist.for_each(
            [&model](OpenSim::Component const*& associated_component, Mesh_instance const&) {
                // for this screen specifically, the "owner"s should be fixed up to point to
                // muscle objects, rather than direct (e.g. GeometryPath) objects
                OpenSim::Component const* c = associated_component;
                while (c != nullptr && c->hasOwner()) {
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

    if (impl.flags & Component3DViewerFlags_RecolorMusclesByStrain) {
        impl.drawlist.for_each([&state](OpenSim::Component const* c, Mesh_instance& mi) {
            OpenSim::Muscle const* musc = dynamic_cast<OpenSim::Muscle const*>(c);
            if (!musc) {
                return;
            }

            mi.rgba.r = static_cast<unsigned char>(255.0 * musc->getTendonStrain(state));
            mi.rgba.g = 255 / 2;
            mi.rgba.b = 255 / 2;
            mi.rgba.a = 255;
        });
    }

    if (impl.flags & Component3DViewerFlags_RecolorMusclesByLength) {
        impl.drawlist.for_each([&state](OpenSim::Component const* c, Mesh_instance& mi) {
            OpenSim::Muscle const* musc = dynamic_cast<OpenSim::Muscle const*>(c);
            if (!musc) {
                return;
            }

            mi.rgba.r = static_cast<unsigned char>(255.0 * musc->getLength(state));
            mi.rgba.g = 255 / 4;
            mi.rgba.b = 255 / 4;
            mi.rgba.a = 255;
        });
    }

    // if applicable, apply rim coloring to selected items
    if (impl.rendering_flags & DrawcallFlags_DrawRims) {
        apply_standard_rim_coloring(impl.drawlist, current_hover, current_selection);
    }

    // draw scene
    ImVec2 dims = ImGui::GetContentRegionAvail();
    if (dims.x >= 1.0f && dims.y >= 1.0f) {

        // ensure render target dimensions match ImGui::Image dimensions
        impl.render_target.reconfigure(
            static_cast<int>(dims.x), static_cast<int>(dims.y), Application::current().samples());

        // update hovertest location
        {
            ImVec2 cp = ImGui::GetCursorPos();
            ImVec2 mp = ImGui::GetMousePos();
            ImVec2 wp = ImGui::GetWindowPos();
            impl.hovertest_x = static_cast<int>((mp.x - wp.x) - cp.x);
            impl.hovertest_y = static_cast<int>(dims.y - ((mp.y - wp.y) - cp.y));
        }

        // render
        gl::Texture_2d& render = perform_backend_render_drawcall(impl);

        // blit render to ImGui::Image, so that we can use ImGui for hit testing, etc.
        {
            void* texture_handle = reinterpret_cast<void*>(static_cast<uintptr_t>(render.get()));
            ImVec2 image_dimensions{dims.x, dims.y};
            ImVec2 uv0{0, 1};
            ImVec2 uv1{1, 0};
            ImGui::Image(texture_handle, image_dimensions, uv0, uv1);
        }

        // update user-viewable mouse state
        {
            impl.mouse_over_render = ImGui::IsItemHovered();
            impl.is_left_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
            impl.is_right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
        }

        if (impl.mouse_over_render) {
            ImGui::CaptureMouseFromApp(false);
        }
    }

    resp.hovertest_result = impl.hovered_component;
    resp.is_moused_over = impl.mouse_over_render;
    resp.is_left_clicked = impl.is_left_clicked;
    resp.is_right_clicked = impl.is_right_clicked;

    ImGui::EndChild();
}

static osc::Component3DViewerResponse draw(
        osc::Component_3d_viewer::Impl& impl,
        char const* panel_name,
        OpenSim::Component const& model,
        OpenSim::ModelDisplayHints const& mdh,
        SimTK::State const& state,
        OpenSim::Component const* current_selection,
        OpenSim::Component const* current_hover) {

    Component3DViewerResponse resp;
    int imgui_style_pushes = 0;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0, 0.0));
    ++imgui_style_pushes;

    if (!ImGui::Begin(panel_name, nullptr, ImGuiWindowFlags_MenuBar)) {

        // panel is closed, so don't show anything
        ImGui::End();
        ImGui::PopStyleVar(imgui_style_pushes);
        return resp;
    }

    // else: show panel content

    update_camera_from_user_input(impl);
    draw_viewport_main_menu(impl);
    draw_viewport_3d_render(impl, model, mdh, state, current_selection, current_hover, resp);

    ImGui::End();
    ImGui::PopStyleVar(imgui_style_pushes);

    return resp;
}


// Component_3d_viewer public interface

Component_3d_viewer::Component_3d_viewer(Component3DViewerFlags flags) : impl{new Impl{flags}} {
}

Component_3d_viewer::Component_3d_viewer(Component_3d_viewer&&) noexcept = default;

Component_3d_viewer& Component_3d_viewer::operator=(Component_3d_viewer&&) noexcept = default;

Component_3d_viewer::~Component_3d_viewer() noexcept = default;

bool Component_3d_viewer::is_moused_over() const noexcept {
    return impl->mouse_over_render;
}

bool Component_3d_viewer::on_event(const SDL_Event& e) {
    return ::on_event(*impl, e);
}

Component3DViewerResponse Component_3d_viewer::draw(
    char const* panel_name,
    OpenSim::Component const& model,
    OpenSim::ModelDisplayHints const& mdh,
    SimTK::State const& state,
    OpenSim::Component const* current_selection,
    OpenSim::Component const* current_hover) {

    return ::draw(*impl,
                  panel_name,
                  model,
                  mdh,
                  state,
                  current_selection,
                  current_hover);

}

Component3DViewerResponse Component_3d_viewer::draw(
    char const* panel_name,
    OpenSim::Model const& model,
    SimTK::State const& st,
    OpenSim::Component const* current_selection,
    OpenSim::Component const* current_hover) {

    return ::draw(*impl,
                  panel_name,
                  model,
                  model.getDisplayHints(),
                  st,
                  current_selection,
                  current_hover);
}
