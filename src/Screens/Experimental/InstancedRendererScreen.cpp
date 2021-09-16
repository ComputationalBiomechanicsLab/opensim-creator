#include "InstancedRendererScreen.hpp"

#include "src/App.hpp"
#include "src/3D/Constants.hpp"
#include "src/3D/Gl.hpp"
#include "src/3D/GlGlm.hpp"
#include "src/3D/InstancedRenderer.hpp"
#include "src/3D/Model.hpp"
#include "src/3D/Shaders/ColormappedPlainTextureShader.hpp"
#include "src/Screens/Experimental/ExperimentsScreen.hpp"

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include <algorithm>
#include <vector>

using namespace osc;

static InstancedDrawlist makeDrawlist(int rows, int cols) {
    InstanceableMeshdata cube = uploadMeshdataForInstancing(GenCube());

    std::vector<glm::mat4x3> modelMtxs;
    std::vector<glm::mat3x3> normalMtxs;
    std::vector<Rgba32> colors;
    std::vector<InstanceableMeshdata> meshes;
    std::vector<unsigned char> rims;

    // add a scaled cube instance that indexes the cube meshdata
    int n = 0;
    int total = rows * cols;
    float rimPerStep = 255.0f / static_cast<float>(total);
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
            glm::mat4 normalMtx = NormalMatrix(xform);

            modelMtxs.push_back(xform);
            normalMtxs.push_back(normalMtx);
            colors.push_back({0xff, 0x00, 0x00, 0xff});
            meshes.push_back(cube);
            rims.push_back(static_cast<unsigned char>(n++ * rimPerStep));
        }
    }

    DrawlistCompilerInput inp{};
    inp.ninstances = modelMtxs.size();
    inp.modelMtxs = modelMtxs.data();
    inp.normalMtxs = normalMtxs.data();
    inp.colors = colors.data();
    inp.meshes = meshes.data();
    inp.rimIntensities = rims.data();

    InstancedDrawlist rv;
    uploadInputsToDrawlist(inp, rv);
    return rv;
}

struct osc::InstancedRendererScreen::Impl final {
    InstancedRenderer renderer;

    int rows = 512;
    int cols = 512;
    InstancedDrawlist drawlist = makeDrawlist(rows, cols);
    InstancedRendererParams params;

    ColormappedPlainTextureShader cpt;

    CPUMesh quadMesh = GenTexturedQuad();
    gl::ArrayBuffer<glm::vec3> quadPositions{quadMesh.verts};
    gl::ArrayBuffer<glm::vec2> quadTexCoords{quadMesh.texcoords};
    gl::VertexArray quadVAO = [this]() {
        gl::VertexArray rv;
        gl::BindVertexArray(rv);
        gl::BindBuffer(quadPositions);
        gl::VertexAttribPointer(cpt.aPos, false, sizeof(glm::vec3), 0);
        gl::EnableVertexAttribArray(cpt.aPos);
        gl::BindBuffer(quadTexCoords);
        gl::VertexAttribPointer(cpt.aTexCoord, false, sizeof(glm::vec2), 0);
        gl::EnableVertexAttribArray(cpt.aTexCoord);
        gl::BindVertexArray();
        return rv;
    }();

    EulerPerspectiveCamera camera;

    bool drawRims = true;
};

osc::InstancedRendererScreen::InstancedRendererScreen() :
    m_Impl{new Impl{}} {

    App::cur().disableVsync();
    App::cur().enableDebugMode();
}

osc::InstancedRendererScreen::~InstancedRendererScreen() noexcept = default;

void osc::InstancedRendererScreen::onMount() {
    osc::ImGuiInit();
}

void osc::InstancedRendererScreen::onUnmount() {
    osc::ImGuiShutdown();
}

void osc::InstancedRendererScreen::onEvent(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        App::cur().requestTransition<ExperimentsScreen>();
        return;
    }
}

void osc::InstancedRendererScreen::tick(float) {
    // connect input state to an euler (first-person-shooter style)
    // camera

    auto& camera = m_Impl->camera;
    auto& io = ImGui::GetIO();

    float speed = 0.1f;
    float sensitivity = 0.01f;

    if (io.KeysDown[SDL_SCANCODE_W]) {
        camera.pos += speed * camera.getFront() * io.DeltaTime;
    }

    if (io.KeysDown[SDL_SCANCODE_S]) {
        camera.pos -= speed * camera.getFront() * io.DeltaTime;
    }

    if (io.KeysDown[SDL_SCANCODE_A]) {
        camera.pos -= speed * camera.getRight() * io.DeltaTime;
    }

    if (io.KeysDown[SDL_SCANCODE_D]) {
        camera.pos += speed * camera.getRight() * io.DeltaTime;
    }

    if (io.KeysDown[SDL_SCANCODE_SPACE]) {
        camera.pos += speed * camera.getUp() * io.DeltaTime;
    }

    if (io.KeyCtrl) {
        camera.pos -= speed * camera.getUp() * io.DeltaTime;
    }

    camera.yaw += sensitivity * io.MouseDelta.x;
    camera.pitch  -= sensitivity * io.MouseDelta.y;
    camera.pitch = std::clamp(camera.pitch, -fpi2 + 0.5f, fpi2 - 0.5f);
}

void osc::InstancedRendererScreen::draw() {
    osc::ImGuiNewFrame();

    {
        ImGui::Begin("frame");
        ImGui::Text("%.1f", ImGui::GetIO().Framerate);
        int rows = m_Impl->rows;
        int cols = m_Impl->cols;
        if (ImGui::InputInt("rows", &rows, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (rows > 0 && rows != m_Impl->rows) {
                m_Impl->rows = rows;
                m_Impl->drawlist = makeDrawlist(m_Impl->rows, m_Impl->cols);
            }
        }
        if (ImGui::InputInt("cols", &cols, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (cols > 0 && cols != m_Impl->cols) {
                m_Impl->cols = cols;
                m_Impl->drawlist = makeDrawlist(m_Impl->cols, m_Impl->cols);
            }
        }
        ImGui::Checkbox("rims", &m_Impl->drawRims);
        ImGui::End();
    }

    Impl& impl = *m_Impl;
    InstancedRenderer& renderer = impl.renderer;

    // ensure renderer output matches window
    renderer.setDims(App::cur().idims());
    renderer.setMsxaaSamples(App::cur().getSamples());

    gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspectRatio = App::cur().dims().x / App::cur().dims().y;
    impl.camera.znear = 0.01f;
    impl.camera.zfar = 1.0f;
    impl.params.viewMtx = impl.camera.getViewMtx();
    impl.params.projMtx = impl.camera.getProjMtx(aspectRatio);
    if (impl.drawRims) {
        impl.params.flags |= InstancedRendererFlags_DrawRims;
    } else {
        impl.params.flags &= ~InstancedRendererFlags_DrawRims;
    }

    renderer.render(impl.params, impl.drawlist);

    gl::Texture2D& tex = renderer.getOutputTexture();
    gl::UseProgram(impl.cpt.program);
    gl::Uniform(impl.cpt.uMVP, gl::identity);
    gl::ActiveTexture(GL_TEXTURE0);
    gl::BindTexture(tex);
    gl::Uniform(impl.cpt.uSamplerAlbedo, gl::textureIndex<GL_TEXTURE0>());
    gl::Uniform(impl.cpt.uSamplerMultiplier, gl::identity);
    gl::BindVertexArray(impl.quadVAO);
    gl::DrawArrays(GL_TRIANGLES, 0, impl.quadPositions.sizei());
    gl::BindVertexArray();

    osc::ImGuiRender();
}
