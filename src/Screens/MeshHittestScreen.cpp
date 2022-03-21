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

namespace {
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

struct osc::MeshHittestScreen::Impl final {
    BasicShader shader;

    Mesh mesh = LoadMeshViaSimTK(App::resource("geometry/hat_ribs.vtp"));

    // sphere (debug)
    MeshData sphere = GenUntexturedUVSphere(12, 12);
    gl::ArrayBuffer<glm::vec3> sphereVBO{sphere.verts};
    gl::ElementArrayBuffer<uint32_t> sphereEBO{sphere.indices};
    gl::VertexArray sphereVAO = makeVAO(shader, sphereVBO, sphereEBO);

    // triangle (debug)
    glm::vec3 tris[3];
    gl::ArrayBuffer<glm::vec3> triangleVBO;
    gl::ElementArrayBuffer<uint32_t> triangleEBO = {0, 1, 2};
    gl::VertexArray triangleVAO = makeVAO(shader, triangleVBO, triangleEBO);

    // line (debug)
    gl::ArrayBuffer<glm::vec3> lineVBO;
    gl::ElementArrayBuffer<uint32_t> lineEBO = {0, 1};
    gl::VertexArray lineVAO = makeVAO(shader, lineVBO, lineEBO);

    std::chrono::microseconds raycastDur{0};
    PolarPerspectiveCamera camera;
    bool isMousedOver = false;
    glm::vec3 hitpos = {0.0f, 0.0f, 0.0f};

    Line ray;
};


// public Impl.

osc::MeshHittestScreen::MeshHittestScreen() :
    m_Impl{new Impl{}}
{
}

osc::MeshHittestScreen::~MeshHittestScreen() noexcept = default;

void osc::MeshHittestScreen::onMount()
{
    osc::ImGuiInit();
    App::cur().disableVsync();
}

void osc::MeshHittestScreen::onUnmount()
{
    osc::ImGuiShutdown();
}

void osc::MeshHittestScreen::onEvent(SDL_Event const& e)
{
    if (osc::ImGuiOnEvent(e))
    {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
    {
        App::cur().requestTransition<ExperimentsScreen>();
        return;
    }
}

void osc::MeshHittestScreen::tick(float)
{
    Impl& impl = *m_Impl;

    UpdatePolarCameraFromImGuiUserInput(App::cur().dims(), impl.camera);

    // handle hittest
    auto raycastStart = std::chrono::high_resolution_clock::now();
    {

        impl.ray = impl.camera.unprojectTopLeftPosToWorldRay(ImGui::GetIO().MousePos, App::cur().dims());

        impl.isMousedOver = false;
        nonstd::span<glm::vec3 const> tris = impl.mesh.getVerts();
        for (size_t i = 0; i < tris.size(); i += 3)
        {
            RayCollision res = GetRayCollisionTriangle(impl.ray, tris.data() + i);
            if (res.hit)
            {
                impl.hitpos = impl.ray.origin + res.distance*impl.ray.dir;
                impl.isMousedOver = true;
                impl.tris[0] = tris[i];
                impl.tris[1] = tris[i+1];
                impl.tris[2] = tris[i+2];
                impl.triangleVBO.assign(impl.tris, 3);

                glm::vec3 lineverts[2] = {impl.ray.origin, impl.ray.origin + 100.0f*impl.ray.dir};
                impl.lineVBO.assign(lineverts, 2);

                break;
            }
        }
    }
    auto raycastEnd = std::chrono::high_resolution_clock::now();
    auto raycastDt = raycastEnd - raycastStart;
    impl.raycastDur = std::chrono::duration_cast<std::chrono::microseconds>(raycastDt);
}

void osc::MeshHittestScreen::draw()
{
    osc::ImGuiNewFrame();

    Impl& impl = *m_Impl;
    auto& shader = impl.shader;

    // printout stats
    {
        ImGui::Begin("controls");
        ImGui::Text("%lld microseconds", impl.raycastDur.count());
        auto r = impl.ray;
        ImGui::Text("camerapos = (%.2f, %.2f, %.2f)", impl.camera.getPos().x, impl.camera.getPos().y, impl.camera.getPos().z);
        ImGui::Text("origin = (%.2f, %.2f, %.2f), dir = (%.2f, %.2f, %.2f)", r.origin.x, r.origin.y, r.origin.z, r.dir.x, r.dir.y, r.dir.z);
        if (impl.isMousedOver)
        {
            ImGui::Text("hit = (%.2f, %.2f, %.2f)", impl.hitpos.x, impl.hitpos.y, impl.hitpos.z);
            ImGui::Text("p1 = (%.2f, %.2f, %.2f)", impl.tris[0].x, impl.tris[0].y, impl.tris[0].z);
            ImGui::Text("p2 = (%.2f, %.2f, %.2f)", impl.tris[1].x, impl.tris[1].y, impl.tris[1].z);
            ImGui::Text("p3 = (%.2f, %.2f, %.2f)", impl.tris[2].x, impl.tris[2].y, impl.tris[2].z);

        }
        ImGui::End();
    }

    gl::Viewport(0, 0, App::cur().idims().x, App::cur().idims().y);
    gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::UseProgram(shader.prog);
    gl::Uniform(shader.uModel, gl::identity);
    gl::Uniform(shader.uView, impl.camera.getViewMtx());
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(App::cur().aspectRatio()));
    gl::Uniform(shader.uColor, impl.isMousedOver ? glm::vec4{0.0f, 1.0f, 0.0f, 1.0f} : glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
    if (true)
    {
        gl::BindVertexArray(impl.mesh.GetVertexArray());
        impl.mesh.Draw();
        gl::BindVertexArray();
    }


    if (impl.isMousedOver)
    {
        gl::Disable(GL_DEPTH_TEST);

        // draw sphere
        gl::Uniform(shader.uModel, glm::translate(glm::mat4{1.0f}, impl.hitpos) * glm::scale(glm::mat4{1.0f}, {0.01f, 0.01f, 0.01f}));
        gl::Uniform(shader.uColor, {1.0f, 1.0f, 0.0f, 1.0f});
        gl::BindVertexArray(impl.sphereVAO);
        gl::DrawElements(GL_TRIANGLES, impl.sphereEBO.sizei(), gl::indexType(impl.sphereEBO), nullptr);
        gl::BindVertexArray();

        // draw triangle
        gl::Uniform(shader.uModel, gl::identity);
        gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});
        gl::BindVertexArray(impl.triangleVAO);
        gl::DrawElements(GL_TRIANGLES, impl.triangleEBO.sizei(), gl::indexType(impl.triangleEBO), nullptr);
        gl::BindVertexArray();

        // draw line
        gl::Uniform(shader.uModel, gl::identity);
        gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});
        gl::BindVertexArray(impl.lineVAO);
        gl::DrawElements(GL_LINES, impl.lineEBO.sizei(), gl::indexType(impl.lineEBO), nullptr);
        gl::BindVertexArray();

        gl::Enable(GL_DEPTH_TEST);
    }

    osc::ImGuiRender();
}

