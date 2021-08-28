#include "instanced_renderer_screen.hpp"

#include "src/app.hpp"
#include "src/3d/constants.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/gl_glm.hpp"
#include "src/3d/instanced_renderer.hpp"
#include "src/3d/model.hpp"
#include "src/3d/shaders/colormapped_plain_texture_shader.hpp"
#include "src/screens/experimental/experiments_screen.hpp"

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

using namespace osc;

static Instanced_drawlist make_drawlist(int rows, int cols) {
    Instanceable_meshdata cube = upload_meshdata_for_instancing(gen_cube());

    std::vector<glm::mat4x3> model_xforms;
    std::vector<glm::mat3x3> normal_xforms;
    std::vector<Rgba32> colors;
    std::vector<Instanceable_meshdata> meshes;
    std::vector<unsigned char> rims;

    // add a scaled cube instance that indexes the cube meshdata
    int n = 0;
    int total = rows * cols;
    float rim_per_step = 255.0f / static_cast<float>(total);
    for (int col = 0; col < cols; ++col) {
        for (int row = 0; row < rows; ++row) {
            float x = (2.0f * (static_cast<float>(col) / static_cast<float>(cols))) - 1.0f;
            float y = (2.0f * (static_cast<float>(row) / static_cast<float>(rows))) - 1.0f;

            float w = 0.5f / static_cast<float>(cols);
            float h = 0.5f / static_cast<float>(rows);
            float d = w;

            glm::mat4 translate = glm::translate(glm::mat4{1.0f}, {x, y, 0.0f});
            glm::mat4 scale = glm::scale(glm::mat4{1.0f}, glm::vec3{w, h, d});
            glm::mat4 xform = translate * scale;
            glm::mat4 normal_mtx = normal_matrix(xform);

            model_xforms.push_back(xform);
            normal_xforms.push_back(normal_mtx);
            colors.push_back({0xff, 0x00, 0x00, 0xff});
            meshes.push_back(cube);
            rims.push_back(static_cast<unsigned char>(n++ * rim_per_step));
        }
    }

    Drawlist_compiler_input inp{};
    inp.ninstances = model_xforms.size();
    inp.model_xforms = model_xforms.data();
    inp.normal_xforms = normal_xforms.data();
    inp.colors = colors.data();
    inp.meshes = meshes.data();
    inp.rim_intensity = rims.data();

    Instanced_drawlist rv;
    upload_inputs_to_drawlist(inp, rv);
    return rv;
}

struct osc::Instanced_render_screen::Impl final {
    Instanced_renderer renderer;

    int rows = 512;
    int cols = 512;
    Instanced_drawlist drawlist = make_drawlist(rows, cols);
    Render_params params;

    Colormapped_plain_texture_shader cpt;

    NewMesh quad_mesh = gen_textured_quad();
    gl::Array_buffer<glm::vec3> quad_positions{quad_mesh.verts};
    gl::Array_buffer<glm::vec2> quad_texcoords{quad_mesh.texcoords};
    gl::Vertex_array quad_vao = [this]() {
        gl::Vertex_array rv;
        gl::BindVertexArray(rv);
        gl::BindBuffer(quad_positions);
        gl::VertexAttribPointer(cpt.aPos, false, sizeof(glm::vec3), 0);
        gl::EnableVertexAttribArray(cpt.aPos);
        gl::BindBuffer(quad_texcoords);
        gl::VertexAttribPointer(cpt.aTexCoord, false, sizeof(glm::vec2), 0);
        gl::EnableVertexAttribArray(cpt.aTexCoord);
        gl::BindVertexArray();
        return rv;
    }();

    Euler_perspective_camera camera;

    bool draw_rims = true;
};

osc::Instanced_render_screen::Instanced_render_screen() :
    m_Impl{new Impl{}} {

    App::cur().disable_vsync();
    App::cur().enable_debug_mode();
}

osc::Instanced_render_screen::~Instanced_render_screen() noexcept = default;

void osc::Instanced_render_screen::on_mount() {
    osc::ImGuiInit();
}

void osc::Instanced_render_screen::on_unmount() {
    osc::ImGuiShutdown();
}

void osc::Instanced_render_screen::on_event(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        App::cur().request_transition<Experiments_screen>();
        return;
    }
}

void osc::Instanced_render_screen::tick(float) {
    // connect input state to an euler (first-person-shooter style)
    // camera

    auto& camera = m_Impl->camera;
    auto& io = ImGui::GetIO();

    float speed = 0.1f;
    float sensitivity = 0.01f;

    if (io.KeysDown[SDL_SCANCODE_W]) {
        camera.pos += speed * camera.front() * io.DeltaTime;
    }

    if (io.KeysDown[SDL_SCANCODE_S]) {
        camera.pos -= speed * camera.front() * io.DeltaTime;
    }

    if (io.KeysDown[SDL_SCANCODE_A]) {
        camera.pos -= speed * camera.right() * io.DeltaTime;
    }

    if (io.KeysDown[SDL_SCANCODE_D]) {
        camera.pos += speed * camera.right() * io.DeltaTime;
    }

    if (io.KeysDown[SDL_SCANCODE_SPACE]) {
        camera.pos += speed * camera.up() * io.DeltaTime;
    }

    if (io.KeyCtrl) {
        camera.pos -= speed * camera.up() * io.DeltaTime;
    }

    camera.yaw += sensitivity * io.MouseDelta.x;
    camera.pitch  -= sensitivity * io.MouseDelta.y;
    camera.pitch = std::clamp(camera.pitch, -fpi2 + 0.5f, fpi2 - 0.5f);
}

void osc::Instanced_render_screen::draw() {
    osc::ImGuiNewFrame();

    {
        ImGui::Begin("frame");
        ImGui::Text("%.1f", ImGui::GetIO().Framerate);
        int rows = m_Impl->rows;
        int cols = m_Impl->cols;
        if (ImGui::InputInt("rows", &rows, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (rows > 0 && rows != m_Impl->rows) {
                m_Impl->rows = rows;
                m_Impl->drawlist = make_drawlist(m_Impl->rows, m_Impl->cols);
            }
        }
        if (ImGui::InputInt("cols", &cols, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (cols > 0 && cols != m_Impl->cols) {
                m_Impl->cols = cols;
                m_Impl->drawlist = make_drawlist(m_Impl->cols, m_Impl->cols);
            }
        }
        ImGui::Checkbox("rims", &m_Impl->draw_rims);
        ImGui::End();
    }

    Impl& impl = *m_Impl;
    Instanced_renderer& renderer = impl.renderer;

    // ensure renderer output matches window
    renderer.set_dims(App::cur().idims());
    renderer.set_msxaa_samples(App::cur().get_samples());

    gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect_ratio = App::cur().dims().x / App::cur().dims().y;
    impl.camera.znear = 0.01f;
    impl.camera.zfar = 1.0f;
    impl.params.view_matrix = impl.camera.view_matrix();
    impl.params.projection_matrix = impl.camera.projection_matrix(aspect_ratio);
    if (impl.draw_rims) {
        impl.params.flags |= DrawcallFlags_DrawRims;
    } else {
        impl.params.flags &= ~DrawcallFlags_DrawRims;
    }

    renderer.render(impl.params, impl.drawlist);

    gl::Texture_2d& tex = renderer.output_texture();
    gl::UseProgram(impl.cpt.program);
    gl::Uniform(impl.cpt.uMVP, gl::identity_val);
    gl::ActiveTexture(GL_TEXTURE0);
    gl::BindTexture(tex);
    gl::Uniform(impl.cpt.uSamplerAlbedo, gl::texture_index<GL_TEXTURE0>());
    gl::Uniform(impl.cpt.uSamplerMultiplier, gl::identity_val);
    gl::BindVertexArray(impl.quad_vao);
    gl::DrawArrays(GL_TRIANGLES, 0, impl.quad_positions.sizei());
    gl::BindVertexArray();

    osc::ImGuiRender();
}
