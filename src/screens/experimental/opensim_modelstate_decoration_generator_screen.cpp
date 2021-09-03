#include "opensim_modelstate_decoration_generator_screen.hpp"

#include "src/app.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/gl_glm.hpp"
#include "src/3d/instanced_renderer.hpp"
#include "src/3d/shaders/solid_color_shader.hpp"
#include "src/screens/experimental/experiments_screen.hpp"
#include "src/opensim_bindings/scene_generator.hpp"
#include "src/utils/imgui_utils.hpp"
#include "src/utils/perf.hpp"

#include <imgui.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKcommon.h>

using namespace osc;

static gl::Vertex_array make_vao(Solid_color_shader& scs, gl::Array_buffer<glm::vec3>& vbo) {
    gl::Vertex_array rv;
    gl::BindVertexArray(rv);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(scs.aPos, false, sizeof(glm::vec3), 0);
    gl::EnableVertexAttribArray(scs.aPos);
    gl::BindVertexArray();
    return rv;
}

struct osc::Opensim_modelstate_decoration_generator_screen::Impl final {
    Instanced_renderer renderer;
    Instanced_drawlist drawlist;
    Render_params render_params;

    Scene_generator generator;
    Scene_decorations scene_decorations;
    std::vector<unsigned char> rim_highlights;

    //OpenSim::Model model{App::resource("models/RajagopalModel/Rajagopal2015.osim").string()};
    //OpenSim::Model model{App::resource("models/GeometryBackendTest/full.osim").string()};
    OpenSim::Model model{App::resource("models/Arm26/arm26.osim").string()};
    //OpenSim::Model model{App::resource("models/ToyLanding/ToyLandingModel.osim").string()};
    SimTK::State state = [this]() {
        model.finalizeFromProperties();
        model.finalizeConnections();
        SimTK::State s = model.initSystem();
        model.realizeReport(s);
        return s;
    }();
    Polar_perspective_camera camera;

    Basic_perf_timer timer_meshgen;
    Basic_perf_timer timer_sort;
    Basic_perf_timer timer_render;
    Basic_perf_timer timer_blit;
    Basic_perf_timer timer_scene_hittest;
    Basic_perf_timer timer_triangle_hittest;
    Basic_perf_timer timer_e2e;

    Solid_color_shader scs;
    NewMesh wireframe_mesh = gen_cube_lines();
    gl::Array_buffer<glm::vec3> wireframe_vbo{wireframe_mesh.verts};
    gl::Vertex_array wireframe_vao = make_vao(scs, wireframe_vbo);

    gl::Array_buffer<glm::vec3> triangle_vbo;
    gl::Vertex_array triangle_vao = make_vao(scs, triangle_vbo);

    std::vector<BVH_Collision> hit_aabbs;
    std::vector<BVH_Collision> hit_tris_bvhcache;
    struct Triangle_collision {
        int instanceidx;
        BVH_Collision collision;
    };
    std::vector<Triangle_collision> hit_tris;

    bool generate_decorations_each_frame = true;
    bool optimize_draw_order = true;
    bool draw_scene = true;
    bool draw_rims = false;
    bool draw_aabbs = false;
    bool do_scene_hittest = true;
    bool do_triangle_hittest = true;
    bool draw_triangle_intersection = true;
};

// public API

osc::Opensim_modelstate_decoration_generator_screen::Opensim_modelstate_decoration_generator_screen() :
    m_Impl{new Impl{}} {

    App::cur().disable_vsync();
    m_Impl->model.updDisplayHints().set_show_frames(true);
    m_Impl->model.updDisplayHints().set_show_wrap_geometry(true);
    m_Impl->generator.generate(
        m_Impl->model,
        m_Impl->state,
        m_Impl->model.getDisplayHints(),
        Modelstate_decoration_generator_flags_Default,
        1.0f,
        m_Impl->scene_decorations);
}

osc::Opensim_modelstate_decoration_generator_screen::~Opensim_modelstate_decoration_generator_screen() noexcept = default;

void osc::Opensim_modelstate_decoration_generator_screen::on_mount() {
    osc::ImGuiInit();
}

void osc::Opensim_modelstate_decoration_generator_screen::on_unmount() {
    osc::ImGuiShutdown();
}

void osc::Opensim_modelstate_decoration_generator_screen::on_event(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        App::cur().request_transition<Experiments_screen>();
        return;
    }
}

void osc::Opensim_modelstate_decoration_generator_screen::draw() {
    auto guard = m_Impl->timer_e2e.measure();

    osc::ImGuiNewFrame();

    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Impl& s = *m_Impl;

    update_camera_from_user_input(App::cur().dims(), m_Impl->camera);

    // decoration generation
    if (s.generate_decorations_each_frame) {
        auto meshgen_timer_guard = s.timer_meshgen.measure();
        s.generator.generate(
            s.model,
            s.state,
            s.model.getDisplayHints(),
            Modelstate_decoration_generator_flags_Default,
            1.0f,
            s.scene_decorations);
    }

    // do scene hittest
    s.hit_aabbs.clear();
    if (s.do_scene_hittest) {
        auto scene_hittest_guard = s.timer_scene_hittest.measure();
        Line ray_worldspace = s.camera.screenpos_to_world_ray(ImGui::GetIO().MousePos, App::cur().dims());
        Scene_decorations const& decs = s.scene_decorations;
        BVH_get_ray_collision_AABBs(decs.aabb_bvh, ray_worldspace, &s.hit_aabbs);
    }

    // do triangle hittest
    s.hit_tris.clear();
    if (s.do_scene_hittest && s.do_triangle_hittest) {
        auto triangle_hittest_guard = s.timer_triangle_hittest.measure();

        Line ray_worldspace = s.camera.screenpos_to_world_ray(ImGui::GetIO().MousePos, App::cur().dims());

        // can just iterate through the scene-level hittest result
        for (BVH_Collision const& c : s.hit_aabbs) {

            // then use a triangle-level BVH to figure out which triangles intersect
            glm::mat4 const& model_mtx = s.scene_decorations.model_xforms[c.prim_id];
            CPU_mesh const& mesh = *s.scene_decorations.cpu_meshes[c.prim_id];

            Line ray_modelspace = apply_xform_to_line(ray_worldspace, glm::inverse(model_mtx));

            if (BVH_get_ray_collisions_triangles(mesh.triangle_bvh, mesh.data.verts.data(), mesh.data.verts.size(), ray_modelspace, &s.hit_tris_bvhcache)) {
                for (BVH_Collision const& tri_collision : s.hit_tris_bvhcache) {
                    s.hit_tris.push_back(Impl::Triangle_collision{c.prim_id, tri_collision});
                }
                s.hit_tris_bvhcache.clear();
            }
        }
    }

    // perform object highlighting
    if (s.draw_triangle_intersection && !s.hit_tris.empty()) {
        // get closest triangle
        auto& ts = s.hit_tris;
        auto is_closest = [](Impl::Triangle_collision const& a, Impl::Triangle_collision const& b) {
            return a.collision.distance < b.collision.distance;
        };
        std::sort(ts.begin(), ts.end(), is_closest);
        Impl::Triangle_collision closest = ts.front();

        // populate rim highlights
        s.rim_highlights.clear();
        s.rim_highlights.resize(s.scene_decorations.model_xforms.size(), 0x00);
        s.rim_highlights[closest.instanceidx] = 0xff;
    }

    // GPU upload, with object highlighting
    {
        auto sort_guard = s.timer_sort.measure();

        Drawlist_compiler_input inp;
        inp.ninstances = s.scene_decorations.model_xforms.size();
        inp.model_xforms = s.scene_decorations.model_xforms.data();
        inp.normal_xforms = s.scene_decorations.normal_xforms.data();
        inp.colors = s.scene_decorations.rgbas.data();
        inp.meshes = s.scene_decorations.gpu_meshes.data();
        inp.textures = nullptr;
        s.rim_highlights.resize(s.scene_decorations.model_xforms.size());
        inp.rim_intensity = s.rim_highlights.data();

        upload_inputs_to_drawlist(inp, s.drawlist);
    }

    // draw the scene
    if (s.draw_scene) {
        // render the decorations into the renderer's texture
        s.renderer.set_dims(App::cur().idims());
        s.renderer.set_msxaa_samples(App::cur().get_samples());
        s.render_params.projection_matrix = s.camera.projection_matrix(s.renderer.aspect_ratio());
        s.render_params.view_matrix = s.camera.view_matrix();
        s.render_params.view_pos = s.camera.pos();
        if (s.draw_rims) {
            s.render_params.flags |= DrawcallFlags_DrawRims;
        } else {
            s.render_params.flags &= ~DrawcallFlags_DrawRims;
        }

        auto render_guard = s.timer_render.measure();
        s.renderer.render(s.render_params, s.drawlist);

        glFlush();
    }

    // draw the AABBs
    if (s.draw_aabbs) {
        gl::BindFramebuffer(GL_FRAMEBUFFER, s.renderer.output_fbo());

        gl::UseProgram(s.scs.prog);
        gl::BindVertexArray(s.wireframe_vao);
        gl::Uniform(s.scs.uProjection, s.camera.projection_matrix(s.renderer.aspect_ratio()));
        gl::Uniform(s.scs.uView, s.camera.view_matrix());
        for (size_t i = 0; i < s.scene_decorations.aabbs.size(); ++i) {
            AABB const& aabb = s.scene_decorations.aabbs[i];

            glm::vec3 half_widths = aabb_dims(aabb) / 2.0f;
            glm::vec3 center = aabb_center(aabb);

            glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, half_widths);
            glm::mat4 mover = glm::translate(glm::mat4{1.0f}, center);
            glm::mat4 mmtx = mover * scaler;

            bool hit = std::find_if(s.hit_aabbs.begin(), s.hit_aabbs.end(), [id = static_cast<int>(i)](BVH_Collision const& c) {
                return c.prim_id == id;
            }) != s.hit_aabbs.end();

            if (hit) {
                gl::Uniform(s.scs.uColor, {1.0f, 0.0f, 0.0f, 1.0f});
            } else {
                gl::Uniform(s.scs.uColor, {0.0f, 0.0f, 0.0f, 1.0f});
            }
            gl::Uniform(s.scs.uModel, mmtx);

            gl::DrawArrays(GL_LINES, 0, s.wireframe_vbo.sizei());
        }
        gl::BindVertexArray();

        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
    }

    // draw closest triangle intersection
    if (s.draw_triangle_intersection && !s.hit_tris.empty()) {
        // get closest triangle
        auto& ts = s.hit_tris;
        auto is_closest = [](Impl::Triangle_collision const& a, Impl::Triangle_collision const& b) {
            return a.collision.distance < b.collision.distance;
        };
        std::sort(ts.begin(), ts.end(), is_closest);
        Impl::Triangle_collision closest = ts.front();

        // upload triangle to GPU
        CPU_mesh const& m = *s.scene_decorations.cpu_meshes[closest.instanceidx];
        glm::vec3 const* tristart = m.data.verts.data() + closest.collision.prim_id;
        glm::mat4 model2world = s.scene_decorations.model_xforms[closest.instanceidx];
        glm::vec3 tri_worldpsace[] = {
            model2world * glm::vec4{tristart[0], 1.0f},
            model2world * glm::vec4{tristart[1], 1.0f},
            model2world * glm::vec4{tristart[2], 1.0f},
        };
        s.triangle_vbo.assign(tri_worldpsace, 3);

        // draw the triangle
        gl::BindFramebuffer(GL_FRAMEBUFFER, s.renderer.output_fbo());
        gl::UseProgram(s.scs.prog);
        gl::Uniform(s.scs.uColor, {0.0f, 0.0f, 0.0f, 1.0f});
        gl::Uniform(s.scs.uProjection, s.camera.projection_matrix(s.renderer.aspect_ratio()));
        gl::Uniform(s.scs.uView, s.camera.view_matrix());
        gl::Uniform(s.scs.uModel, gl::identity_val);
        gl::BindVertexArray(s.triangle_vao);
        gl::Disable(GL_DEPTH_TEST);
        gl::DrawArrays(GL_TRIANGLES, 0, 3);
        gl::Enable(GL_DEPTH_TEST);
        gl::BindVertexArray();
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
    }

    // blit the scene
    {
        auto blit_guard = s.timer_blit.measure();

        gl::Texture_2d& render = s.renderer.output_texture();

        // draw texture using ImGui::Image
        ImGui::SetNextWindowPos({0.0f, 0.0f});
        ImGui::SetNextWindowSize(App::cur().dims());
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, {1.0f, 0.0f, 0.0f, 1.0f});
        ImGui::Begin("render output", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoSavedSettings);
        void* texture_handle = reinterpret_cast<void*>(static_cast<uintptr_t>(render.get()));
        ImVec2 image_dimensions{App::cur().dims()};
        ImVec2 uv0{0, 1};
        ImVec2 uv1{1, 0};
        ImGui::Image(texture_handle, image_dimensions, uv0, uv1);
        ImGui::SetCursorPos({0.0f, 0.0f});
        ImGui::End();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();
    }

    // draw debug panels/controls
    {
        ImGui::Begin("controls");
        ImGui::Text("FPS = %.2f", ImGui::GetIO().Framerate);
        ImGui::Text("decoration generation (us) = %.1f", s.timer_meshgen.micros());
        ImGui::Text("instance batching sort (us) = %.1f", s.timer_sort.micros());
        ImGui::Text("scene-level BVHed hittest (us) = %.1f", s.timer_scene_hittest.micros());
        ImGui::Text("mesh-level triangle hittest (us) = %.1f", s.timer_triangle_hittest.micros());
        ImGui::Text("instanced render call (us) = %.1f", s.timer_render.micros());
        ImGui::Text("texture blit (us) = %.1f", s.timer_blit.micros());
        ImGui::Text("e2e (us) = %.1f", s.timer_e2e.micros());
        ImGui::Checkbox("generate decorations each frame", &s.generate_decorations_each_frame);
        ImGui::Checkbox("optimize draw order", &s.optimize_draw_order);
        ImGui::Checkbox("draw scene", &s.draw_scene);
        ImGui::Checkbox("draw rims", &s.draw_rims);
        ImGui::Checkbox("draw AABBs", &s.draw_aabbs);
        ImGui::Checkbox("do hittest", &s.do_scene_hittest);
        ImGui::End();
    }

    osc::ImGuiRender();
}
