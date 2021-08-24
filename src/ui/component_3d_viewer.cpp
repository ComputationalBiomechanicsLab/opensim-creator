#include "component_3d_viewer.hpp"

#include "src/app.hpp"
#include "src/3d/constants.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/instanced_renderer.hpp"
#include "src/3d/model.hpp"
#include "src/opensim_bindings/scene_generator.hpp"
#include "src/utils/scope_guard.hpp"

#include <imgui.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <vector>
#include <limits>

using namespace osc;

struct osc::Component_3d_viewer::Impl final {
    Component3DViewerFlags flags;

    Scene_generator sg;
    Scene_decorations decorations;

    Instanced_renderer renderer;
    Instanced_drawlist drawlist;
    Render_params renderer_params;

    Polar_perspective_camera camera;

    std::vector<unsigned char> rims;
    std::vector<BVH_Collision> scene_hittest_results;
    std::vector<BVH_Collision> triangle_hittest_results;

    bool render_hovered = false;
    bool render_left_clicked = false;
    bool render_right_clicked = false;

    Impl(Component3DViewerFlags flags_) : flags{flags_} {
    }
};

static void update_camera(osc::Component_3d_viewer::Impl& impl) {

    impl.camera.radius *= 1.0f - ImGui::GetIO().MouseWheel/10.0f;

    // update camera
    //
    // At the moment, all camera updates happen via the middle-mouse. The mouse
    // input is designed to mirror Blender fairly closely (because, imho, it has
    // decent UX for this problem space)
    if (impl.render_hovered && ImGui::IsMouseDown(ImGuiMouseButton_Left)) { // TODO: middle mouse
        ImVec2 screendims = impl.renderer.dimsf();
        float aspect_ratio = screendims.x / screendims.y;
        ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.0f);
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);

        // relative vectors
        float rdx = delta.x/screendims.x;
        float rdy = delta.y/screendims.y;

        if (ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT)) {
            // shift + middle-mouse: pan
            impl.camera.do_pan(aspect_ratio, {rdx, rdy});
        } else if (ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL)) {
            // shift + middle-mouse: zoom
            impl.camera.radius *= 1.0f + rdy;
        } else {
            // middle: mouse drag
            impl.camera.do_drag({rdx, rdy});
        }
    }
}

static void draw_options_menu(osc::Component_3d_viewer::Impl& impl) {

    ImGui::CheckboxFlags(
        "draw dynamic geometry", &impl.flags, Component3DViewerFlags_DrawDynamicDecorations);

    ImGui::CheckboxFlags(
        "draw static geometry", &impl.flags, Component3DViewerFlags_DrawStaticDecorations);

    ImGui::CheckboxFlags("draw frames", &impl.flags, Component3DViewerFlags_DrawFrames);

    ImGui::CheckboxFlags("draw debug geometry", &impl.flags, Component3DViewerFlags_DrawDebugGeometry);
    ImGui::CheckboxFlags("draw labels", &impl.flags, Component3DViewerFlags_DrawLabels);

    ImGui::Separator();

    ImGui::Text("Graphical Options:");

    ImGui::CheckboxFlags("wireframe mode", &impl.renderer_params.flags, DrawcallFlags_WireframeMode);
    ImGui::CheckboxFlags("show normals", &impl.renderer_params.flags, DrawcallFlags_ShowMeshNormals);
    ImGui::CheckboxFlags("draw rims", &impl.renderer_params.flags, DrawcallFlags_DrawRims);
    ImGui::CheckboxFlags("draw scene geometry", &impl.renderer_params.flags, DrawcallFlags_DrawSceneGeometry);
    ImGui::CheckboxFlags("draw floor", &impl.flags, Component3DViewerFlags_DrawFloor);
    ImGui::CheckboxFlags("show XZ grid", &impl.flags, Component3DViewerFlags_DrawXZGrid);
    ImGui::CheckboxFlags("show XY grid", &impl.flags, Component3DViewerFlags_DrawXYGrid);
    ImGui::CheckboxFlags("show YZ grid", &impl.flags, Component3DViewerFlags_DrawYZGrid);
    ImGui::CheckboxFlags("show alignment axes", &impl.flags, Component3DViewerFlags_DrawAlignmentAxes);
}

static void draw_scene_menu(osc::Component_3d_viewer::Impl& impl) {
    if (ImGui::Button("Top")) {
        impl.camera.theta = 0.0f;
        impl.camera.phi = fpi2;
    }

    if (ImGui::Button("Left")) {
        // assumes models tend to point upwards in Y and forwards in +X
        // (so sidewards is theta == 0 or PI)
        impl.camera.theta = fpi;
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
        impl.camera.phi = 3.0f * fpi2;
    }

    ImGui::NewLine();

    ImGui::SliderFloat("radius", &impl.camera.radius, 0.0f, 10.0f);
    ImGui::SliderFloat("theta", &impl.camera.theta, 0.0f, 2.0f * fpi);
    ImGui::SliderFloat("phi", &impl.camera.phi, 0.0f, 2.0f * fpi);
    ImGui::InputFloat("fov", &impl.camera.fov);
    ImGui::InputFloat("znear", &impl.camera.znear);
    ImGui::InputFloat("zfar", &impl.camera.zfar);
    ImGui::NewLine();
    ImGui::SliderFloat("pan_x", &impl.camera.pan.x, -100.0f, 100.0f);
    ImGui::SliderFloat("pan_y", &impl.camera.pan.y, -100.0f, 100.0f);
    ImGui::SliderFloat("pan_z", &impl.camera.pan.z, -100.0f, 100.0f);

    ImGui::Separator();

    ImGui::SliderFloat("light_dir_x", &impl.renderer_params.light_dir.x, -1.0f, 1.0f);
    ImGui::SliderFloat("light_dir_y", &impl.renderer_params.light_dir.y, -1.0f, 1.0f);
    ImGui::SliderFloat("light_dir_z", &impl.renderer_params.light_dir.z, -1.0f, 1.0f);
    ImGui::ColorEdit3("light_color", reinterpret_cast<float*>(&impl.renderer_params.light_rgb));
    ImGui::ColorEdit3("background color", reinterpret_cast<float*>(&impl.renderer_params.background_rgba));

    ImGui::Separator();

    // ImGui::InputFloat("fixup scale factor", &impl.fixup_scale_factor); TODO
}

static void draw_main_menu_contents(osc::Component_3d_viewer::Impl& impl) {
    if (ImGui::BeginMenu("Options")) {
        draw_options_menu(impl);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Scene")) {
        draw_scene_menu(impl);
        ImGui::EndMenu();
    }

    // TODO: muscle recoloring
}

// public API

osc::Component_3d_viewer::Component_3d_viewer(Component3DViewerFlags flags) :
    m_Impl{new Impl{flags}} {
}

osc::Component_3d_viewer::~Component_3d_viewer() noexcept = default;

bool osc::Component_3d_viewer::is_moused_over() const noexcept {
    return m_Impl->render_hovered;
}

bool osc::Component_3d_viewer::on_event(SDL_Event const&) {
    return false;  // TODO
}

Component3DViewerResponse osc::Component_3d_viewer::draw(
        char const* panel_name,
        OpenSim::Component const& c,
        OpenSim::ModelDisplayHints const& mdh,
        SimTK::State const& st,
        OpenSim::Component const* current_selection,
        OpenSim::Component const* current_hover) {

    Impl& impl = *m_Impl;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
    OSC_SCOPE_GUARD({ ImGui::PopStyleVar(); });

    // try to start drawing the main panel, but return early if it's closed
    // to prevent the UI from having to do redundant work
    bool opened = ImGui::Begin(panel_name, nullptr, ImGuiWindowFlags_MenuBar);
    OSC_SCOPE_GUARD({ ImGui::End(); });

    if (!opened) {
        return {};  // main panel is closed, so skip the rest of the rendering etc.
    }

    // update camera from user input
    update_camera(impl);

    // draw panel menu
    if (ImGui::BeginMenuBar()) {
        draw_main_menu_contents(impl);
        ImGui::EndMenuBar();
    }

    // put 3D scene in an undraggable child panel, to prevent accidental panel
    // dragging when the user drags their mouse over the scene
    if (!ImGui::BeginChild("##child", {0.0f, 0.0f}, false, ImGuiWindowFlags_NoMove)) {
        return {};  // child not visible
    }
    OSC_SCOPE_GUARD({ ImGui::EndChild(); });

    // generate scene decorations from the model + state
    {
        Modelstate_decoration_generator_flags flags = Modelstate_decoration_generator_flags_Default;
        OpenSim::ModelDisplayHints mdhcpy{mdh};

        // map component flags onto renderer/generator flags

        if (impl.flags & Component3DViewerFlags_DrawDebugGeometry) {
            mdhcpy.set_show_debug_geometry(true);
        } else {
            mdhcpy.set_show_debug_geometry(false);
        }

        if (impl.flags & Component3DViewerFlags_DrawDynamicDecorations) {
            flags |= Modelstate_decoration_generator_flags_GenerateDynamicDecorations;
        } else {
            flags &= ~Modelstate_decoration_generator_flags_GenerateDynamicDecorations;
        }

        if (impl.flags & Component3DViewerFlags_DrawFrames) {
            mdhcpy.set_show_frames(true);
        } else {
            mdhcpy.set_show_frames(false);
        }

        if (impl.flags & Component3DViewerFlags_DrawLabels) {
            mdhcpy.set_show_labels(true);
        } else {
            mdhcpy.set_show_labels(false);
        }

        if (impl.flags & Component3DViewerFlags_DrawStaticDecorations) {
            flags |= Modelstate_decoration_generator_flags_GenerateStaticDecorations;
        } else {
            flags &= ~Modelstate_decoration_generator_flags_GenerateStaticDecorations;
        }

        impl.sg.generate(c, st, mdh, impl.decorations, flags);
    }

    // make rim highlights reflect selection/hover state
    impl.rims.clear();
    impl.rims.resize(impl.decorations.model_xforms.size(), 0x00);
    for (size_t i = 0; i < impl.decorations.model_xforms.size(); ++i) {
        if (impl.decorations.components[i] == current_selection) {
            impl.rims[i] = 0xff;
        } else if (impl.decorations.components[i] == current_hover) {
            impl.rims[i] = 0x77;
        }
    }

    // upload scene decorations to a drawlist
    {
        Drawlist_compiler_input dci;
        dci.ninstances = impl.decorations.model_xforms.size();
        dci.model_xforms = impl.decorations.model_xforms.data();
        dci.normal_xforms = impl.decorations.normal_xforms.data();
        dci.colors = impl.decorations.rgbas.data();
        dci.meshes = impl.decorations.gpu_meshes.data();
        dci.textures = nullptr;
        dci.rim_intensity = impl.rims.data();
        upload_inputs_to_drawlist(dci, impl.drawlist);
    }

    // TODO:
    // - draw floor (in its own drawlist?)
    // - draw XZ grid
    // - draw XY grid
    // - draw YZ grid
    // - draw alignment axes

    // render scene with renderer
    ImVec2 content_region = ImGui::GetContentRegionAvail();
    if (content_region.x >= 1.0f && content_region.y >= 1.0f) {

        // ensure renderer dims match the panel's dims
        glm::ivec2 dims{static_cast<int>(content_region.x), static_cast<int>(content_region.y)};
        impl.renderer.set_dims(dims);
        impl.renderer.set_msxaa_samples(App::cur().get_samples());

        // render scene to texture (the texture is used later, by ImGui::Image)
        impl.renderer_params.projection_matrix = impl.camera.projection_matrix(content_region.x/content_region.y);
        impl.renderer_params.view_matrix = impl.camera.view_matrix();
        impl.renderer_params.view_pos = impl.camera.pos();
        impl.renderer.render(impl.renderer_params, impl.drawlist);
    }

    // perform hittest (AABB raycast, triangle raycast, BVH accelerated)
    OpenSim::Component const* hittest_result = nullptr;
    if (impl.render_hovered) {
        // figure out mouse pos in panel's NDC system
        glm::vec2 window_scr_pos = ImGui::GetWindowPos();  // where current ImGui window is in the screen
        glm::vec2 mouse_scr_pos = ImGui::GetMousePos();  // where mouse is in the screen
        glm::vec2 mouse_winpos = mouse_scr_pos - window_scr_pos;  // where mouse is in current window
        glm::vec2 cursor_winpos = ImGui::GetCursorPos();  // where cursor is in current window
        glm::vec2 mouse_itempos = mouse_winpos - cursor_winpos;  // where mouse is in current item
        glm::vec2 item_dims = ImGui::GetContentRegionAvail();  // how big current window will be

        // un-project the mouse position as a ray in worldspace
        Line camera_ray = impl.camera.screenpos_to_world_ray(mouse_itempos, item_dims);

        // use scene BVH to intersect that ray with the scene
        impl.scene_hittest_results.clear();
        BVH_get_ray_collision_AABBs(impl.decorations.aabb_bvh, camera_ray, &impl.scene_hittest_results);

        // go through triangle BVHes to figure out which, if any, triangle is closest intersecting
        int closest_idx = -1;
        float closest_distance = std::numeric_limits<float>::max();

        // iterate through each scene-level hit and perform a triangle-level hittest
        for (BVH_Collision const& c : impl.scene_hittest_results) {
            int instance_idx = c.prim_id;
            glm::mat4 const& instance_mmtx = impl.decorations.model_xforms[instance_idx];
            CPU_mesh const& instance_mesh = *impl.decorations.cpu_meshes[instance_idx];

            Line camera_ray_modelspace = apply_xform_to_line(camera_ray, glm::inverse(instance_mmtx));

            // perform ray-triangle BVH hittest
            impl.triangle_hittest_results.clear();
            BVH_get_ray_collisions_triangles(
                        instance_mesh.triangle_bvh,
                        instance_mesh.data.verts.data(),
                        instance_mesh.data.verts.size(),
                        camera_ray_modelspace,
                        &impl.triangle_hittest_results);

            // check each triangle collision and take the closest
            for (BVH_Collision const& tc : impl.triangle_hittest_results) {
                if (tc.distance < closest_distance) {
                    closest_idx = instance_idx;
                    closest_distance = tc.distance;
                }
            }
        }

        if (closest_idx >= 0) {
            hittest_result = impl.decorations.components[closest_idx];
        }
    }

    // blit scene render (texture) to screen with ImGui::Image
    {
        GLint tex_gl_handle = impl.renderer.output_texture().get();
        void* tex_imgui_handle = reinterpret_cast<void*>(tex_gl_handle);

        ImVec2 img_dims = ImGui::GetContentRegionAvail();
        ImVec2 texcoord_uv0 = {0.0f, 1.0f};
        ImVec2 texcoord_uv1 = {1.0f, 0.0f};

        ImGui::Image(tex_imgui_handle, img_dims, texcoord_uv0, texcoord_uv1);

        impl.render_hovered = ImGui::IsItemHovered();
        impl.render_left_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
        impl.render_right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
    }

    Component3DViewerResponse resp;
    resp.hovertest_result = hittest_result;
    resp.is_moused_over = impl.render_hovered;
    resp.is_left_clicked = impl.render_left_clicked;
    resp.is_right_clicked = impl.render_right_clicked;
    return resp;
}

Component3DViewerResponse osc::Component_3d_viewer::draw(
        char const* panel_name,
        OpenSim::Model const& model,
        SimTK::State const& state,
        OpenSim::Component const* current_selection,
        OpenSim::Component const* current_hover) {

    return draw(panel_name,
                model,
                model.getDisplayHints(),
                state,
                current_selection,
                current_hover);
}
