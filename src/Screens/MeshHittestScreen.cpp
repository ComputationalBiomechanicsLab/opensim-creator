#include "MeshHittestScreen.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Bindings/SimTKHelpers.hpp"
#include "src/Graphics/Gl.hpp"
#include "src/Graphics/GlGlm.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshData.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Line.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/Platform/App.hpp"
#include "src/Screens/ExperimentsScreen.hpp"

#include <glm/vec3.hpp>
#include <imgui.h>

#include <cstdint>
#include <chrono>

static char const g_VertexShader[] =
R"(
    #version 330 core

    uniform mat4 uProjMat;
    uniform mat4 uViewMat;
    uniform mat4 uModelMat;

    layout (location = 0) in vec3 aPos;

    void main()
    {
        gl_Position = uProjMat * uViewMat * uModelMat * vec4(aPos, 1.0);
    }
)";

static char const g_FragmentShader[] =
R"(
    #version 330 core

    uniform vec4 uColor;

    out vec4 FragColor;

    void main()
    {
        FragColor = uColor;
    }
)";

namespace
{
    struct BasicShader final {
        gl::Program prog = gl::CreateProgramFrom(
            gl::CompileFromSource<gl::VertexShader>(g_VertexShader),
            gl::CompileFromSource<gl::FragmentShader>(g_FragmentShader));

        gl::AttributeVec3 aPos{0};

        gl::UniformMat4 uModel = gl::GetUniformLocation(prog, "uModelMat");
        gl::UniformMat4 uView = gl::GetUniformLocation(prog, "uViewMat");
        gl::UniformMat4 uProjection = gl::GetUniformLocation(prog, "uProjMat");
        gl::UniformVec4 uColor = gl::GetUniformLocation(prog, "uColor");
    };
}

static gl::VertexArray makeVAO(BasicShader& shader,
                               gl::ArrayBuffer<glm::vec3>& vbo,
                               gl::ElementArrayBuffer<uint32_t>& ebo)
{
    gl::VertexArray rv;
    gl::BindVertexArray(rv);
    gl::BindBuffer(vbo);
    gl::BindBuffer(ebo);
    gl::VertexAttribPointer(shader.aPos, false, sizeof(glm::vec3), 0);
    gl::EnableVertexAttribArray(shader.aPos);
    gl::BindVertexArray();
    return rv;
}

class osc::MeshHittestScreen::Impl final {
public:
    void onMount()
    {
        osc::ImGuiInit();
        App::upd().disableVsync();
        gl::Disable(GL_CULL_FACE);
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
        App const& app = App::get();
        UpdatePolarCameraFromImGuiUserInput(app.dims(), m_Camera);

        // handle hittest
        auto raycastStart = std::chrono::high_resolution_clock::now();
        {

            m_Ray = m_Camera.unprojectTopLeftPosToWorldRay(ImGui::GetIO().MousePos, app.dims());

            m_IsMousedOver = false;
            nonstd::span<glm::vec3 const> tris = m_Mesh.getVerts();
            for (size_t i = 0; i < tris.size(); i += 3)
            {
                RayCollision res = GetRayCollisionTriangle(m_Ray, tris.data() + i);
                if (res.hit)
                {
                    m_HitPos = m_Ray.origin + res.distance*m_Ray.dir;
                    m_IsMousedOver = true;
                    m_Tris[0] = tris[i];
                    m_Tris[1] = tris[i+1];
                    m_Tris[2] = tris[i+2];
                    m_TriangleVBO.assign(m_Tris, 3);

                    glm::vec3 lineverts[2] = {m_Ray.origin, m_Ray.origin + 100.0f*m_Ray.dir};
                    m_LineVBO.assign(lineverts, 2);

                    break;
                }
            }
        }
        auto raycastEnd = std::chrono::high_resolution_clock::now();
        auto raycastDt = raycastEnd - raycastStart;
        m_RaycastDur = std::chrono::duration_cast<std::chrono::microseconds>(raycastDt);
    }

    void onDraw()
    {
        osc::ImGuiNewFrame();

        // printout stats
        {
            ImGui::Begin("controls");
            ImGui::Text("%ld microseconds", static_cast<long>(m_RaycastDur.count()));
            auto r = m_Ray;
            ImGui::Text("camerapos = (%.2f, %.2f, %.2f)", m_Camera.getPos().x, m_Camera.getPos().y, m_Camera.getPos().z);
            ImGui::Text("origin = (%.2f, %.2f, %.2f), dir = (%.2f, %.2f, %.2f)", r.origin.x, r.origin.y, r.origin.z, r.dir.x, r.dir.y, r.dir.z);
            if (m_IsMousedOver)
            {
                ImGui::Text("hit = (%.2f, %.2f, %.2f)", m_HitPos.x, m_HitPos.y, m_HitPos.z);
                ImGui::Text("p1 = (%.2f, %.2f, %.2f)", m_Tris[0].x, m_Tris[0].y, m_Tris[0].z);
                ImGui::Text("p2 = (%.2f, %.2f, %.2f)", m_Tris[1].x, m_Tris[1].y, m_Tris[1].z);
                ImGui::Text("p3 = (%.2f, %.2f, %.2f)", m_Tris[2].x, m_Tris[2].y, m_Tris[2].z);

            }
            ImGui::End();
        }

        App const& app = App::get();
        gl::Viewport(0, 0, app.idims().x, app.idims().y);
        gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl::UseProgram(m_Shader.prog);
        gl::Uniform(m_Shader.uModel, gl::identity);
        gl::Uniform(m_Shader.uView, m_Camera.getViewMtx());
        gl::Uniform(m_Shader.uProjection, m_Camera.getProjMtx(app.aspectRatio()));
        gl::Uniform(m_Shader.uColor, m_IsMousedOver ? glm::vec4{0.0f, 1.0f, 0.0f, 1.0f} : glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
        if (true)
        {
            gl::BindVertexArray(m_Mesh.GetVertexArray());
            m_Mesh.Draw();
            gl::BindVertexArray();
        }


        if (m_IsMousedOver)
        {
            gl::Disable(GL_DEPTH_TEST);

            // draw sphere
            gl::Uniform(m_Shader.uModel, glm::translate(glm::mat4{1.0f}, m_HitPos) * glm::scale(glm::mat4{1.0f}, {0.01f, 0.01f, 0.01f}));
            gl::Uniform(m_Shader.uColor, {1.0f, 1.0f, 0.0f, 1.0f});
            gl::BindVertexArray(m_SphereVAO);
            gl::DrawElements(GL_TRIANGLES, m_SphereEBO.sizei(), gl::indexType(m_SphereEBO), nullptr);
            gl::BindVertexArray();

            // draw triangle
            gl::Uniform(m_Shader.uModel, gl::identity);
            gl::Uniform(m_Shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});
            gl::BindVertexArray(m_TriangleVAO);
            gl::DrawElements(GL_TRIANGLES, m_TriangleEBO.sizei(), gl::indexType(m_TriangleEBO), nullptr);
            gl::BindVertexArray();

            // draw line
            gl::Uniform(m_Shader.uModel, gl::identity);
            gl::Uniform(m_Shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});
            gl::BindVertexArray(m_LineVAO);
            gl::DrawElements(GL_LINES, m_LineEBO.sizei(), gl::indexType(m_LineEBO), nullptr);
            gl::BindVertexArray();

            gl::Enable(GL_DEPTH_TEST);
        }

        osc::ImGuiRender();
    }

private:
    BasicShader m_Shader;

    Mesh m_Mesh = LoadMeshViaSimTK(App::resource("geometry/hat_ribs.vtp"));

    // sphere (debug)
    MeshData m_Sphere = GenUntexturedUVSphere(12, 12);
    gl::ArrayBuffer<glm::vec3> m_SphereVBO{m_Sphere.verts};
    gl::ElementArrayBuffer<uint32_t> m_SphereEBO{m_Sphere.indices};
    gl::VertexArray m_SphereVAO = makeVAO(m_Shader, m_SphereVBO, m_SphereEBO);

    // triangle (debug)
    glm::vec3 m_Tris[3];
    gl::ArrayBuffer<glm::vec3> m_TriangleVBO;
    gl::ElementArrayBuffer<uint32_t> m_TriangleEBO = {0, 1, 2};
    gl::VertexArray m_TriangleVAO = makeVAO(m_Shader, m_TriangleVBO, m_TriangleEBO);

    // line (debug)
    gl::ArrayBuffer<glm::vec3> m_LineVBO;
    gl::ElementArrayBuffer<uint32_t> m_LineEBO = {0, 1};
    gl::VertexArray m_LineVAO = makeVAO(m_Shader, m_LineVBO, m_LineEBO);

    std::chrono::microseconds m_RaycastDur{0};
    PolarPerspectiveCamera m_Camera;
    bool m_IsMousedOver = false;
    glm::vec3 m_HitPos = {0.0f, 0.0f, 0.0f};

    Line m_Ray;
};


// public Impl (PIMPL)

osc::MeshHittestScreen::MeshHittestScreen() :
    m_Impl{new Impl{}}
{
}

osc::MeshHittestScreen::MeshHittestScreen(MeshHittestScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::MeshHittestScreen& osc::MeshHittestScreen::operator=(MeshHittestScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::MeshHittestScreen::~MeshHittestScreen() noexcept
{
    delete m_Impl;
}

void osc::MeshHittestScreen::onMount()
{
    m_Impl->onMount();
}

void osc::MeshHittestScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::MeshHittestScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::MeshHittestScreen::onTick()
{
    m_Impl->onTick();
}

void osc::MeshHittestScreen::onDraw()
{
    m_Impl->onDraw();
}
