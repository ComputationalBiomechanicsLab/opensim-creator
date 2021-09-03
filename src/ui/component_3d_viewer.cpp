#include "component_3d_viewer.hpp"

#include "src/app.hpp"
#include "src/3d/constants.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/gl_glm.hpp"
#include "src/3d/instanced_renderer.hpp"
#include "src/3d/model.hpp"
#include "src/3d/texturing.hpp"
#include "src/3d/shaders/solid_color_shader.hpp"
#include "src/opensim_bindings/scene_generator.hpp"
#include "src/utils/imgui_utils.hpp"
#include "src/utils/scope_guard.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <vector>
#include <limits>

using namespace osc;

static CPU_mesh gen_floor_mesh() {
    CPU_mesh rv;
    rv.data = gen_textured_quad();
    for (auto& coord : rv.data.texcoords) {
        coord *= 200.0f;
    }
    rv.aabb = aabb_from_points(rv.data.verts.data(), rv.data.verts.size());
    BVH_BuildFromTriangles(rv.triangle_bvh, rv.data.verts.data(), rv.data.verts.size());
    return rv;
}

static gl::Vertex_array make_vao(Solid_color_shader& shader, gl::Array_buffer<glm::vec3>& vbo) {
    gl::Vertex_array rv;
    gl::BindVertexArray(rv);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(shader.aPos, false, sizeof(glm::vec3), 0);
    gl::EnableVertexAttribArray(shader.aPos);
    return rv;
}

struct osc::Component_3d_viewer::Impl final {
    Component3DViewerFlags flags;

    Scene_generator sg;
    Scene_decorations decorations;

    Instanced_renderer renderer;
    Instanced_drawlist drawlist;
    Render_params renderer_params;

    Polar_perspective_camera camera;

    // floor data
    std::shared_ptr<CPU_mesh> floor_mesh = std::make_shared<CPU_mesh>(gen_floor_mesh());
    std::shared_ptr<gl::Texture_2d> chequer_tex = std::make_shared<gl::Texture_2d>(generate_chequered_floor_texture());
    Instanceable_meshdata floor_meshdata = upload_meshdata_for_instancing(floor_mesh->data);

    // plain shader for drawing flat overlay elements
    Solid_color_shader solid_color_shader;

    // grid data
    gl::Array_buffer<glm::vec3> grid_vbo{gen_NxN_grid(100).verts};
    gl::Vertex_array grid_vao = make_vao(solid_color_shader, grid_vbo);

    // line data
    gl::Array_buffer<glm::vec3> line_vbo{gen_y_line().verts};
    gl::Vertex_array line_vao = make_vao(solid_color_shader, line_vbo);

    // aabb data
    gl::Array_buffer<glm::vec3> cubewire_vbo{gen_cube_lines().verts};
    gl::Vertex_array cubewire_vao = make_vao(solid_color_shader, cubewire_vbo);

    std::vector<unsigned char> rims;
    std::vector<std::shared_ptr<gl::Texture_2d>> textures;
    std::vector<BVH_Collision> scene_hittest_results;
    std::vector<BVH_Collision> triangle_hittest_results;

    glm::vec2 render_dims = {0.0f, 0.0f};
    bool render_hovered = false;
    bool render_left_clicked = false;
    bool render_right_clicked = false;

    // scale factor for all non-mesh, non-overlay scene elements (e.g.
    // the floor, bodies)
    //
    // this is necessary because some meshes can be extremely small/large and
    // scene elements need to be scaled accordingly (e.g. without this, a body
    // sphere end up being much larger than a mesh instance). Imagine if the
    // mesh was the leg of a fly, in meters.
    float fixup_scale_factor = 1.0f;

    Impl(Component3DViewerFlags flags_) : flags{flags_} {
    }
};

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
    ImGui::CheckboxFlags("show XZ grid", &impl.flags, Component3DViewerFlags_DrawXZGrid);
    ImGui::CheckboxFlags("show XY grid", &impl.flags, Component3DViewerFlags_DrawXYGrid);
    ImGui::CheckboxFlags("show YZ grid", &impl.flags, Component3DViewerFlags_DrawYZGrid);
    ImGui::CheckboxFlags("show alignment axes", &impl.flags, Component3DViewerFlags_DrawAlignmentAxes);
    ImGui::CheckboxFlags("show grid lines", &impl.flags, Component3DViewerFlags_DrawAxisLines);
    ImGui::CheckboxFlags("show AABBs", &impl.flags, Component3DViewerFlags_DrawAABBs);
    ImGui::CheckboxFlags("show BVH", &impl.flags, Component3DViewerFlags_DrawBVH);
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

    ImGui::InputFloat("fixup scale factor", &impl.fixup_scale_factor);
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
}

static OpenSim::Component const* hittest_scene_decorations(osc::Component_3d_viewer::Impl& impl) {
    if (!impl.render_hovered) {
        return nullptr;
    }

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

    return closest_idx >= 0 ? impl.decorations.components[closest_idx] : nullptr;
}

static void draw_xz_grid(osc::Component_3d_viewer::Impl& impl) {
    Solid_color_shader& shader = impl.solid_color_shader;

    gl::UseProgram(shader.prog);
    gl::Uniform(shader.uModel, glm::scale(glm::rotate(glm::mat4{1.0f}, fpi2, {1.0f, 0.0f, 0.0f}), {5.0f, 5.0f, 1.0f}));
    gl::Uniform(shader.uColor, {0.7f, 0.7f, 0.7f, 0.15f});
    gl::Uniform(shader.uProjection, impl.camera.projection_matrix(impl.render_dims.x / impl.render_dims.y));
    gl::Uniform(shader.uView, impl.camera.view_matrix());
    gl::BindVertexArray(impl.grid_vao);
    gl::DrawArrays(GL_LINES, 0, impl.grid_vbo.sizei());
    gl::BindVertexArray();
}

static void draw_xy_grid(osc::Component_3d_viewer::Impl& impl) {
    Solid_color_shader& shader = impl.solid_color_shader;

    gl::UseProgram(shader.prog);
    gl::Uniform(shader.uModel, glm::scale(glm::mat4{1.0f}, {5.0f, 5.0f, 1.0f}));
    gl::Uniform(shader.uColor, {0.7f, 0.7f, 0.7f, 0.15f});
    gl::Uniform(shader.uProjection, impl.camera.projection_matrix(impl.render_dims.x / impl.render_dims.y));
    gl::Uniform(shader.uView, impl.camera.view_matrix());
    gl::BindVertexArray(impl.grid_vao);
    gl::DrawArrays(GL_LINES, 0, impl.grid_vbo.sizei());
    gl::BindVertexArray();
}

static void draw_yz_grid(osc::Component_3d_viewer::Impl& impl) {
    Solid_color_shader& shader = impl.solid_color_shader;

    gl::UseProgram(shader.prog);
    gl::Uniform(shader.uModel, glm::scale(glm::rotate(glm::mat4{1.0f}, fpi2, {0.0f, 1.0f, 0.0f}), {5.0f, 5.0f, 1.0f}));
    gl::Uniform(shader.uColor, {0.7f, 0.7f, 0.7f, 0.15f});
    gl::Uniform(shader.uProjection, impl.camera.projection_matrix(impl.render_dims.x / impl.render_dims.y));
    gl::Uniform(shader.uView, impl.camera.view_matrix());
    gl::BindVertexArray(impl.grid_vao);
    gl::DrawArrays(GL_LINES, 0, impl.grid_vbo.sizei());
    gl::BindVertexArray();
}

static void draw_alignment_axes(osc::Component_3d_viewer::Impl& impl) {
    glm::mat4 model2view = impl.camera.view_matrix();

    // we only care about rotation of the axes, not translation
    model2view[3] = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f};

    // rescale + translate the y-line vertices
    glm::mat4 make_line_one_sided = glm::translate(glm::identity<glm::mat4>(), glm::vec3{0.0f, 1.0f, 0.0f});
    glm::mat4 scaler = glm::scale(glm::identity<glm::mat4>(), glm::vec3{0.025f});
    glm::mat4 translator = glm::translate(glm::identity<glm::mat4>(), glm::vec3{-0.95f, -0.95f, 0.0f});
    glm::mat4 base_model_mtx = translator * scaler * model2view;

    Solid_color_shader& shader = impl.solid_color_shader;

    // common shader stuff
    gl::UseProgram(shader.prog);
    gl::Uniform(shader.uProjection, gl::identity_val);
    gl::Uniform(shader.uView, gl::identity_val);
    gl::BindVertexArray(impl.line_vao);
    gl::Disable(GL_DEPTH_TEST);

    // y axis
    {
        gl::Uniform(shader.uColor, {0.0f, 1.0f, 0.0f, 1.0f});
        gl::Uniform(shader.uModel, base_model_mtx * make_line_one_sided);
        gl::DrawArrays(GL_LINES, 0, impl.grid_vbo.sizei());
    }

    // x axis
    {
        glm::mat4 rotate_y_to_x =
            glm::rotate(glm::identity<glm::mat4>(), fpi2, glm::vec3{0.0f, 0.0f, -1.0f});

        gl::Uniform(shader.uColor, {1.0f, 0.0f, 0.0f, 1.0f});
        gl::Uniform(shader.uModel, base_model_mtx * rotate_y_to_x * make_line_one_sided);
        gl::DrawArrays(GL_LINES, 0, impl.grid_vbo.sizei());
    }

    // z axis
    {
        glm::mat4 rotate_y_to_z =
            glm::rotate(glm::identity<glm::mat4>(), fpi2, glm::vec3{1.0f, 0.0f, 0.0f});

        gl::Uniform(shader.uColor, {0.0f, 0.0f, 1.0f, 1.0f});
        gl::Uniform(shader.uModel, base_model_mtx * rotate_y_to_z * make_line_one_sided);
        gl::DrawArrays(GL_LINES, 0, impl.grid_vbo.sizei());
    }

    gl::BindVertexArray();
    gl::Enable(GL_DEPTH_TEST);
}

static void draw_floor_axes_lines(osc::Component_3d_viewer::Impl& impl) {
    Solid_color_shader& shader = impl.solid_color_shader;

    // common stuff
    gl::UseProgram(shader.prog);
    gl::Uniform(shader.uProjection, impl.camera.projection_matrix(impl.render_dims.x / impl.render_dims.y));
    gl::Uniform(shader.uView, impl.camera.view_matrix());
    gl::BindVertexArray(impl.line_vao);

    // X
    gl::Uniform(shader.uModel, glm::rotate(glm::mat4{1.0f}, fpi2, {0.0f, 0.0f, 1.0f}));
    gl::Uniform(shader.uColor, {1.0f, 0.0f, 0.0f, 1.0f});
    gl::DrawArrays(GL_LINES, 0, impl.grid_vbo.sizei());

    // Z
    gl::Uniform(shader.uModel, glm::rotate(glm::mat4{1.0f}, fpi2, {1.0f, 0.0f, 0.0f}));
    gl::Uniform(shader.uColor, {0.0f, 0.0f, 1.0f, 1.0f});
    gl::DrawArrays(GL_LINES, 0, impl.grid_vbo.sizei());

    gl::BindVertexArray();
}

static void draw_aabbs(osc::Component_3d_viewer::Impl& impl) {
    Solid_color_shader& shader = impl.solid_color_shader;

    // common stuff
    gl::UseProgram(shader.prog);
    gl::Uniform(shader.uProjection, impl.camera.projection_matrix(impl.render_dims.x / impl.render_dims.y));
    gl::Uniform(shader.uView, impl.camera.view_matrix());
    gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});

    gl::BindVertexArray(impl.cubewire_vao);
    for (AABB const& aabb : impl.decorations.aabbs) {
        glm::vec3 half_widths = aabb_dims(aabb) / 2.0f;
        glm::vec3 center = aabb_center(aabb);

        glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, half_widths);
        glm::mat4 mover = glm::translate(glm::mat4{1.0f}, center);
        glm::mat4 mmtx = mover * scaler;

        gl::Uniform(shader.uModel, mmtx);
        gl::DrawArrays(GL_LINES, 0, impl.cubewire_vbo.sizei());
    }
    gl::BindVertexArray();
}

// assumes `pos` is in-bounds
static void draw_BVH_recursive(osc::Component_3d_viewer::Impl& impl, int pos) {
    BVH_Node const& n = impl.decorations.aabb_bvh.nodes[pos];

    glm::vec3 half_widths = aabb_dims(n.bounds) / 2.0f;
    glm::vec3 center = aabb_center(n.bounds);

    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, half_widths);
    glm::mat4 mover = glm::translate(glm::mat4{1.0f}, center);
    glm::mat4 mmtx = mover * scaler;
    gl::Uniform(impl.solid_color_shader.uModel, mmtx);
    gl::DrawArrays(GL_LINES, 0, impl.cubewire_vbo.sizei());

    if (n.nlhs >= 0) {  // if it's an internal node
        draw_BVH_recursive(impl, pos+1);
        draw_BVH_recursive(impl, pos+n.nlhs+1);
    }
}

static void draw_BVH(osc::Component_3d_viewer::Impl& impl) {
    if (impl.decorations.aabb_bvh.nodes.empty()) {
        return;
    }

    Solid_color_shader& shader = impl.solid_color_shader;

    // common stuff
    gl::UseProgram(shader.prog);
    gl::Uniform(shader.uProjection, impl.camera.projection_matrix(impl.render_dims.x / impl.render_dims.y));
    gl::Uniform(shader.uView, impl.camera.view_matrix());
    gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});

    gl::BindVertexArray(impl.cubewire_vao);
    draw_BVH_recursive(impl, 0);
    gl::BindVertexArray();
}

static void draw_overlays(osc::Component_3d_viewer::Impl& impl) {
    gl::BindFramebuffer(GL_FRAMEBUFFER, impl.renderer.output_fbo());

    if (impl.flags & Component3DViewerFlags_DrawXZGrid) {
        draw_xz_grid(impl);
    }

    if (impl.flags & Component3DViewerFlags_DrawXYGrid) {
        draw_xy_grid(impl);
    }

    if (impl.flags & Component3DViewerFlags_DrawYZGrid) {
        draw_yz_grid(impl);
    }

    if (impl.flags & Component3DViewerFlags_DrawAlignmentAxes) {
        draw_alignment_axes(impl);
    }

    if (impl.flags & Component3DViewerFlags_DrawAxisLines) {
        draw_floor_axes_lines(impl);
    }

    if (impl.flags & Component3DViewerFlags_DrawAABBs) {
        draw_aabbs(impl);
    }

    if (impl.flags & Component3DViewerFlags_DrawBVH) {
        draw_BVH(impl);
    }

    gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
}

// public API

osc::Component_3d_viewer::Component_3d_viewer(Component3DViewerFlags flags) :
    m_Impl{new Impl{flags}} {
}

osc::Component_3d_viewer::~Component_3d_viewer() noexcept = default;

bool osc::Component_3d_viewer::is_moused_over() const noexcept {
    return m_Impl->render_hovered;
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

    if (impl.render_hovered) {
        update_camera_from_user_input(App::cur().dims(), impl.camera);
    }

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

        impl.sg.generate(
            c,
            st,
            mdhcpy,
            flags,
            impl.fixup_scale_factor,
            impl.decorations);
    }

    // append the floor to the decorations list (the floor is "in" the scene, rather than an overlay)
    {
        Scene_decorations& decs = impl.decorations;

        glm::mat4x3& model_xform = decs.model_xforms.emplace_back();
        model_xform = [&impl]() {
            // rotate from XY (+Z dir) to ZY (+Y dir)
            glm::mat4 rv = glm::rotate(glm::mat4{1.0f}, -fpi2, {1.0f, 0.0f, 0.0f});

            // make floor extend far in all directions
            rv = glm::scale(glm::mat4{1.0f}, {impl.fixup_scale_factor * 100.0f, 1.0f, impl.fixup_scale_factor * 100.0f}) * rv;

            // lower slightly, so that it doesn't conflict with OpenSim model planes
            // that happen to lie at Z==0
            rv = glm::translate(glm::mat4{1.0f}, {0.0f, -0.0001f, 0.0f}) * rv;

            return glm::mat4x3{rv};
        }();

        decs.normal_xforms.push_back(normal_matrix(model_xform));
        decs.rgbas.push_back(Rgba32{0x00, 0x00, 0x00, 0x00});
        decs.gpu_meshes.push_back(impl.floor_meshdata);
        decs.cpu_meshes.push_back(impl.floor_mesh);
        decs.aabbs.push_back(aabb_apply_xform(impl.floor_mesh->aabb, model_xform));
        decs.components.push_back(nullptr);

        impl.textures.clear();
        impl.textures.resize(decs.model_xforms.size(), nullptr);
        impl.textures.back() = impl.chequer_tex;
    }

    // make rim highlights reflect selection/hover state
    impl.rims.clear();
    impl.rims.resize(impl.decorations.model_xforms.size(), 0x00);
    for (size_t i = 0; i < impl.decorations.model_xforms.size(); ++i) {
        OpenSim::Component const* assocComponent = impl.decorations.components[i];

        if (!assocComponent) {
            continue;  // no association to rim highlight
        }

        while (assocComponent) {
            if (assocComponent == current_selection) {
                impl.rims[i] = 0xff;
                break;
            } else if (assocComponent == current_hover) {
                impl.rims[i] = 0x66;
                break;
            } else if (!assocComponent->hasOwner()) {
                break;
            } else {
                assocComponent = &assocComponent->getOwner();
            }
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
        dci.textures = impl.textures.data();
        dci.rim_intensity = impl.rims.data();
        upload_inputs_to_drawlist(dci, impl.drawlist);
    }

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

    // render overlay elements (grid axes, lines, etc.)
    draw_overlays(impl);

    // perform hittest (AABB raycast, triangle raycast, BVH accelerated)
    OpenSim::Component const* hittest_result = hittest_scene_decorations(impl);

    // blit scene render (texture) to screen with ImGui::Image
    {
        GLint tex_gl_handle = impl.renderer.output_texture().get();
        void* tex_imgui_handle = reinterpret_cast<void*>(static_cast<uintptr_t>(tex_gl_handle));

        ImVec2 img_dims = ImGui::GetContentRegionAvail();
        ImVec2 texcoord_uv0 = {0.0f, 1.0f};
        ImVec2 texcoord_uv1 = {1.0f, 0.0f};

        ImGui::Image(tex_imgui_handle, img_dims, texcoord_uv0, texcoord_uv1);

        impl.render_dims = ImGui::GetItemRectSize();
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
