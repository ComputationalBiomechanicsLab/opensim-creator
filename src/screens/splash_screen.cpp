#include "splash_screen.hpp"

#include "src/3d/shaders/plain_texture_shader.hpp"
#include "src/3d/constants.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/gl_glm.hpp"
#include "src/3d/instanced_renderer.hpp"
#include "src/3d/model.hpp"
#include "src/3d/texturing.hpp"
#include "src/screens/loading_screen.hpp"
#include "src/ui/main_menu.hpp"
#include "src/app.hpp"
#include "src/log.hpp"
#include "src/styling.hpp"

#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include <memory>

using namespace osc;

static gl::Texture_2d load_image_resource_into_texture(char const* res_pth) {
    return load_image_as_texture(App::resource(res_pth).string().c_str(), TexFlag_Flip_Pixels_Vertically).texture;
}

struct Splash_screen::Impl final {
    // used to render quads (e.g. logo banners)
    Plain_texture_shader pts;

    // TODO: fix this shit packing
    gl::Array_buffer<glm::vec3> quad_vbo{gen_textured_quad().verts};
    gl::Array_buffer<glm::vec2> quad_uv_vbo{gen_textured_quad().texcoords};
    gl::Vertex_array quad_pts_vao = [this]() {
        gl::Vertex_array rv;
        gl::BindVertexArray(rv);
        gl::BindBuffer(quad_vbo);
        gl::VertexAttribPointer(pts.aPos, false, sizeof(glm::vec3), 0);
        gl::EnableVertexAttribArray(pts.aPos);
        gl::BindBuffer(quad_uv_vbo);
        gl::VertexAttribPointer(pts.aTexCoord, false, sizeof(glm::vec2), 0);
        gl::EnableVertexAttribArray(pts.aTexCoord);
        gl::BindVertexArray();
        return rv;
    }();

    // main menu (top bar) states
    ui::main_menu::file_tab::State mm_state;

    // main app logo, blitted to top of the screen
    gl::Texture_2d logo = load_image_resource_into_texture("logo.png");

    // CZI attributation logo, blitted to bottom of screen
    gl::Texture_2d cz_logo = load_image_resource_into_texture("chanzuckerberg_logo.png");

    // TUD attributation logo, blitted to bottom of screen
    gl::Texture_2d tud_logo = load_image_resource_into_texture("tud_logo.png");

    // 3D stuff: for the background image
    Instanced_renderer renderer;
    Instanced_drawlist drawlist = []() {
        glm::mat4x3 mmtx = []() {
            glm::mat4 rv = glm::identity<glm::mat4>();

            // OpenSim: might contain floors at *exactly* Y = 0.0, so shift the chequered
            // floor down *slightly* to prevent Z fighting from planes rendered from the
            // model itself (the contact planes, etc.)
            rv = glm::translate(rv, {0.0f, -0.005f, 0.0f});
            rv = glm::rotate(rv, fpi2, {-1.0, 0.0, 0.0});
            rv = glm::scale(rv, {100.0f, 100.0f, 1.0f});

            return rv;
        }();
        glm::mat3 norm_mtx = normal_matrix(mmtx);
        Rgba32 color = rgba32_from_u32(0xffffffff);
        Instanceable_meshdata md = upload_meshdata_for_instancing(gen_textured_quad());
        std::shared_ptr<gl::Texture_2d> tex = std::make_shared<gl::Texture_2d>(generate_chequered_floor_texture());

        Drawlist_compiler_input inp;
        inp.ninstances = 1;
        inp.model_xforms = &mmtx;
        inp.normal_xforms = &norm_mtx;
        inp.colors = &color;
        inp.meshes = &md;
        inp.textures = &tex;
        inp.rim_intensity = nullptr;

        Instanced_drawlist rv;
        upload_inputs_to_drawlist(inp, rv);
        return rv;
    }();
    Render_params params;
    Polar_perspective_camera camera;

    // top-level UI state that's shared between screens
    std::shared_ptr<Main_editor_state> mes;

    Impl(std::shared_ptr<Main_editor_state> mes_) : mes{std::move(mes_)} {
        log::info("splash screen constructed");
        camera.phi = fpi4;
        camera.radius = 10.0f;
    }

    void draw_quad(glm::mat4 const& mvp, gl::Texture_2d& tex) {
        gl::UseProgram(pts.p);
        gl::Uniform(pts.uMVP, mvp);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(tex);
        gl::Uniform(pts.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::BindVertexArray(quad_pts_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, quad_vbo.sizei());
        gl::BindVertexArray();
    }
};

// private Impl-related functions
namespace {

    void splashscreen_tick(Splash_screen::Impl& impl, float dt) {
        impl.camera.theta += dt * 0.015f;
    }

    void splashscreen_draw(Splash_screen::Impl& impl) {
        gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        constexpr glm::vec2 menu_dims = {700.0f, 500.0f};

        App& app = App::cur();
        glm::ivec2 window_dimsi = app.idims();
        glm::vec2 window_dims{window_dimsi};

        gl::Enable(GL_BLEND);

        // draw chequered floor background
        {
            impl.renderer.set_dims(app.idims());
            impl.renderer.set_msxaa_samples(app.get_samples());
            impl.params.view_matrix = impl.camera.view_matrix();
            impl.params.projection_matrix = impl.camera.projection_matrix(impl.renderer.aspect_ratio());
            impl.params.view_pos = impl.camera.pos();

            impl.renderer.render(impl.params, impl.drawlist);
            impl.draw_quad(glm::mat4{1.0f}, impl.renderer.output_texture());
        }

        // draw logo just above the menu
        {
            glm::vec2 desired_logo_dims = {128.0f, 128.0f};
            glm::vec2 scale = desired_logo_dims / window_dims;
            float y_above_menu = (menu_dims.y + desired_logo_dims.y + 64.0f) / window_dims.y;

            glm::mat4 translate_xform = glm::translate(glm::mat4{1.0f}, {0.0f, y_above_menu, 0.0f});
            glm::mat4 scale_xform = glm::scale(glm::mat4{1.0f}, {scale.x, scale.y, 1.0f});

            glm::mat4 model = translate_xform * scale_xform;

            gl::Disable(GL_DEPTH_TEST);
            gl::Enable(GL_BLEND);
            impl.draw_quad(model, impl.logo);
        }

        if (ImGui::BeginMainMenuBar()) {
            ui::main_menu::file_tab::draw(impl.mm_state, impl.mes);
            ui::main_menu::about_tab::draw();
            ImGui::EndMainMenuBar();
        }

        // center the menu
        {
            glm::vec2 menu_pos = (window_dims - menu_dims) / 2.0f;
            ImGui::SetNextWindowPos(menu_pos);
            ImGui::SetNextWindowSize(ImVec2(menu_dims.x, -1));
            ImGui::SetNextWindowSizeConstraints(menu_dims, menu_dims);
        }

        bool b = true;
        if (ImGui::Begin("Splash screen", &b, ImGuiWindowFlags_NoTitleBar)) {

            // `new` button
            {
                ImGui::PushStyleColor(ImGuiCol_Button, OSC_POSITIVE_RGBA);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, OSC_POSITIVE_HOVERED_RGBA);
                if (ImGui::Button(ICON_FA_FILE_ALT " New Model (Ctrl+N)")) {
                    ui::main_menu::action_new_model(impl.mes);
                }
                ImGui::PopStyleColor(2);
            }

            ImGui::SameLine();

            // `open` button
            {
                if (ImGui::Button(ICON_FA_FOLDER_OPEN " Open Model (Ctrl+O)")) {
                    ui::main_menu::action_open_model(impl.mes);
                }
            }

            ImGui::Dummy(ImVec2{0.0f, 10.0f});

            // de-dupe imgui IDs because these lists may contain duplicate
            // names
            int id = 0;

            ImGui::Columns(2);

            // left column: recent files
            ImGui::TextUnformatted("Recent files:");
            ImGui::Dummy(ImVec2{0.0f, 3.0f});

            if (!impl.mm_state.recent_files.empty()) {
                // iterate in reverse: recent files are stored oldest --> newest
                for (auto it = impl.mm_state.recent_files.rbegin(); it != impl.mm_state.recent_files.rend(); ++it) {
                    Recent_file const& rf = *it;
                    ImGui::PushID(++id);
                    if (ImGui::Button(rf.path.filename().string().c_str())) {
                        app.request_transition<osc::Loading_screen>(impl.mes, rf.path);
                    }
                    ImGui::PopID();
                }
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, OSC_GREYED_RGBA);
                ImGui::TextWrapped("No files opened recently. Try:");
                ImGui::BulletText("Creating a new model (Ctrl+N)");
                ImGui::BulletText("Opening an existing model (Ctrl+O)");
                ImGui::BulletText("Opening an example (right-side)");
                ImGui::PopStyleColor();
            }
            ImGui::NextColumn();

            // right column: example model files
            if (!impl.mm_state.example_osims.empty()) {
                ImGui::TextUnformatted("Example files:");
                ImGui::Dummy(ImVec2{0.0f, 3.0f});

                for (std::filesystem::path const& ex : impl.mm_state.example_osims) {
                    ImGui::PushID(++id);
                    if (ImGui::Button(ex.filename().string().c_str())) {
                        app.request_transition<Loading_screen>(impl.mes, ex);
                    }
                    ImGui::PopID();
                }
            }
            ImGui::NextColumn();

            ImGui::Columns();
        }
        ImGui::End();

        // draw TUD logo below menu, slightly to the left
        {
            glm::vec2 desired_logo_dims = {128.0f, 128.0f};
            glm::vec2 scale = desired_logo_dims / window_dims;
            float x_leftwards_by_logo_width = -desired_logo_dims.x / window_dims.x;
            float y_below_menu = -(25.0f + menu_dims.y + desired_logo_dims.y) / window_dims.y;

            glm::mat4 translate_xform = glm::translate(glm::mat4{1.0f}, {x_leftwards_by_logo_width, y_below_menu, 0.0f});
            glm::mat4 scale_xform = glm::scale(glm::mat4{1.0f}, {scale.x, scale.y, 1.0f});

            glm::mat4 model = translate_xform * scale_xform;

            gl::Enable(GL_BLEND);
            gl::Disable(GL_DEPTH_TEST);
            impl.draw_quad(model, impl.tud_logo);
        }

        // draw CZI logo below menu, slightly to the right
        {
            glm::vec2 desired_logo_dims = {128.0f, 128.0f};
            glm::vec2 scale = desired_logo_dims / window_dims;
            float x_rightwards_by_logo_width = desired_logo_dims.x / window_dims.x;
            float y_below_menu = -(25.0f + menu_dims.y + desired_logo_dims.y) / window_dims.y;

            glm::mat4 translate_xform = glm::translate(glm::mat4{1.0f}, {x_rightwards_by_logo_width, y_below_menu, 0.0f});
            glm::mat4 scale_xform = glm::scale(glm::mat4{1.0f}, {scale.x, scale.y, 1.0f});

            glm::mat4 model = translate_xform * scale_xform;

            gl::Disable(GL_DEPTH_TEST);
            gl::Enable(GL_BLEND);
            impl.draw_quad(model, impl.cz_logo);
        }
    }
}


// public API

osc::Splash_screen::Splash_screen() :
    impl{new Impl{std::make_shared<Main_editor_state>()}} {
}

osc::Splash_screen::Splash_screen(std::shared_ptr<Main_editor_state> mes_) :
    impl{new Impl{std::move(mes_)}} {
}

osc::Splash_screen::~Splash_screen() noexcept = default;

void osc::Splash_screen::on_mount() {
    osc::ImGuiInit();
}

void osc::Splash_screen::on_unmount() {
    osc::ImGuiShutdown();
}

void osc::Splash_screen::on_event(SDL_Event const& e) {
    osc::ImGuiOnEvent(e);
}

void osc::Splash_screen::tick(float dt) {
    ::splashscreen_tick(*impl, dt);
}

void osc::Splash_screen::draw() {
    osc::ImGuiNewFrame();
    ::splashscreen_draw(*impl);
    osc::ImGuiRender();
}
