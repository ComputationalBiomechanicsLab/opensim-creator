#include "InstancedRendererScreen.hpp"

#include "src/Graphics/Color.hpp"
#include "src/Graphics/Gl.hpp"
#include "src/Graphics/GlGlm.hpp"
#include "src/Graphics/InstancedRenderer.hpp"
#include "src/Graphics/MeshData.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Shaders/ColormappedPlainTextureShader.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/EulerPerspectiveCamera.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Platform/App.hpp"
#include "src/Screens/ExperimentsScreen.hpp"

#include <GL/glew.h>
#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_scancode.h>

#include <algorithm>
#include <utility>
#include <vector>

static osc::InstancedDrawlist makeDrawlist(int rows, int cols)
{
    osc::InstanceableMeshdata cube = osc::uploadMeshdataForInstancing(osc::GenCube());

    std::vector<glm::mat4x3> modelMtxs;
    std::vector<glm::mat3x3> normalMtxs;
    std::vector<osc::Rgba32> colors;
    std::vector<osc::InstanceableMeshdata> meshes;
    std::vector<unsigned char> rims;

    // add a scaled cube instance that indexes the cube meshdata
    int n = 0;
    int total = rows * cols;
    float rimPerStep = 255.0f / static_cast<float>(total);
    for (int col = 0; col < cols; ++col)
    {
        for (int row = 0; row < rows; ++row)
        {
            float x = (2.0f * (static_cast<float>(col) / static_cast<float>(cols))) - 1.0f;
            float y = (2.0f * (static_cast<float>(row) / static_cast<float>(rows))) - 1.0f;

            float w = 0.5f / static_cast<float>(cols);
            float h = 0.5f / static_cast<float>(rows);
            float d = w;

            glm::mat4 translate = glm::translate(glm::mat4{1.0f}, {x, y, 0.0f});
            glm::mat4 scale = glm::scale(glm::mat4{1.0f}, glm::vec3{w, h, d});
            glm::mat4 xform = translate * scale;
            glm::mat4 normalMtx = osc::ToNormalMatrix(xform);

            modelMtxs.push_back(xform);
            normalMtxs.push_back(normalMtx);
            colors.push_back({0xff, 0x00, 0x00, 0xff});
            meshes.push_back(cube);
            rims.push_back(static_cast<unsigned char>(n++ * rimPerStep));
        }
    }

    osc::DrawlistCompilerInput inp{};
    inp.ninstances = modelMtxs.size();
    inp.modelMtxs = modelMtxs.data();
    inp.normalMtxs = normalMtxs.data();
    inp.colors = colors.data();
    inp.meshes = meshes.data();
    inp.rimIntensities = rims.data();

    osc::InstancedDrawlist rv;
    uploadInputsToDrawlist(inp, rv);
    return rv;
}

class osc::InstancedRendererScreen::Impl final {
public:

    void onMount()
    {
        App& app = App::upd();
        app.disableVsync();
        app.enableDebugMode();
        osc::ImGuiInit();
    }

    void onUnmount()
    {
        osc::ImGuiShutdown();
    }

    void onEvent(SDL_Event const& e)
    {
        if (e.type == SDL_QUIT)
        {
            App::upd().requestQuit();
            return;
        }
        else if (osc::ImGuiOnEvent(e))
        {
            return;
        }
        else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        {
            App::upd().requestTransition<ExperimentsScreen>();
            return;
        }
    }

    void onTick()
    {
        // connect input state to an euler (first-person-shooter style)
        // camera

        auto& io = ImGui::GetIO();

        float speed = 0.1f;
        float sensitivity = 0.01f;

        if (io.KeysDown[SDL_SCANCODE_W])
        {
            m_Camera.pos += speed * m_Camera.getFront() * io.DeltaTime;
        }

        if (io.KeysDown[SDL_SCANCODE_S])
        {
            m_Camera.pos -= speed * m_Camera.getFront() * io.DeltaTime;
        }

        if (io.KeysDown[SDL_SCANCODE_A])
        {
            m_Camera.pos -= speed * m_Camera.getRight() * io.DeltaTime;
        }

        if (io.KeysDown[SDL_SCANCODE_D])
        {
            m_Camera.pos += speed * m_Camera.getRight() * io.DeltaTime;
        }

        if (io.KeysDown[SDL_SCANCODE_SPACE])
        {
            m_Camera.pos += speed * m_Camera.getUp() * io.DeltaTime;
        }

        if (io.KeyCtrl)
        {
            m_Camera.pos -= speed * m_Camera.getUp() * io.DeltaTime;
        }

        m_Camera.yaw += sensitivity * io.MouseDelta.x;
        m_Camera.pitch  -= sensitivity * io.MouseDelta.y;
        m_Camera.pitch = std::clamp(m_Camera.pitch, -fpi2 + 0.5f, fpi2 - 0.5f);
    }

    void onDraw()
    {
        osc::ImGuiNewFrame();

        {
            ImGui::Begin("frame");
            ImGui::Text("%.1f", ImGui::GetIO().Framerate);
            int rows = m_Rows;
            int cols = m_Cols;
            if (ImGui::InputInt("rows", &rows, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                if (rows > 0 && rows != m_Rows)
                {
                    m_Rows = rows;
                    m_Drawlist = makeDrawlist(m_Rows, m_Cols);
                }
            }
            if (ImGui::InputInt("cols", &cols, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                if (cols > 0 && cols != m_Cols)
                {
                    m_Cols = cols;
                    m_Drawlist = makeDrawlist(m_Rows, m_Cols);
                }
            }
            ImGui::Checkbox("rims", &m_DrawRims);
            ImGui::End();
        }

        // ensure renderer output matches window
        App const& app = App::get();
        m_Renderer.setDims(app.idims());
        m_Renderer.setMsxaaSamples(app.getMSXAASamplesRecommended());

        gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float aspectRatio = app.dims().x / app.dims().y;
        m_Camera.znear = 0.01f;
        m_Camera.zfar = 1.0f;
        m_Params.viewMtx = m_Camera.getViewMtx();
        m_Params.projMtx = m_Camera.getProjMtx(aspectRatio);
        if (m_DrawRims)
        {
            m_Params.flags |= InstancedRendererFlags_DrawRims;
        }
        else
        {
            m_Params.flags &= ~InstancedRendererFlags_DrawRims;
        }

        m_Renderer.render(m_Params, m_Drawlist);

        gl::Texture2D& tex = m_Renderer.getOutputTexture();
        gl::UseProgram(m_Shader.program);
        gl::Uniform(m_Shader.uMVP, gl::identity);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(tex);
        gl::Uniform(m_Shader.uSamplerAlbedo, gl::textureIndex<GL_TEXTURE0>());
        gl::Uniform(m_Shader.uSamplerMultiplier, gl::identity);
        gl::BindVertexArray(m_QuadVAO);
        gl::DrawArrays(GL_TRIANGLES, 0, m_QuadPositions.sizei());
        gl::BindVertexArray();

        osc::ImGuiRender();
    }

private:
    InstancedRenderer m_Renderer;

    int m_Rows = 512;
    int m_Cols = 512;
    InstancedDrawlist m_Drawlist = makeDrawlist(m_Rows, m_Cols);
    InstancedRendererParams m_Params;

    ColormappedPlainTextureShader m_Shader;

    MeshData m_QuadMesh = GenTexturedQuad();
    gl::ArrayBuffer<glm::vec3> m_QuadPositions{m_QuadMesh.verts};
    gl::ArrayBuffer<glm::vec2> m_QuadTexCoords{m_QuadMesh.texcoords};
    gl::VertexArray m_QuadVAO = [this]()
    {
        gl::VertexArray rv;
        gl::BindVertexArray(rv);
        gl::BindBuffer(m_QuadPositions);
        gl::VertexAttribPointer(m_Shader.aPos, false, sizeof(glm::vec3), 0);
        gl::EnableVertexAttribArray(m_Shader.aPos);
        gl::BindBuffer(m_QuadTexCoords);
        gl::VertexAttribPointer(m_Shader.aTexCoord, false, sizeof(glm::vec2), 0);
        gl::EnableVertexAttribArray(m_Shader.aTexCoord);
        gl::BindVertexArray();
        return rv;
    }();

    EulerPerspectiveCamera m_Camera;

    bool m_DrawRims = true;
};


// public API (PIMPL)

osc::InstancedRendererScreen::InstancedRendererScreen() :
    m_Impl{new Impl{}}
{
}

osc::InstancedRendererScreen::InstancedRendererScreen(InstancedRendererScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::InstancedRendererScreen& osc::InstancedRendererScreen::operator=(InstancedRendererScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::InstancedRendererScreen::~InstancedRendererScreen() noexcept
{
    delete m_Impl;
}

void osc::InstancedRendererScreen::onMount()
{
    m_Impl->onMount();
}

void osc::InstancedRendererScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::InstancedRendererScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::InstancedRendererScreen::onTick()
{
    m_Impl->onTick();
}

void osc::InstancedRendererScreen::onDraw()
{
    m_Impl->onDraw();
}
