#include "MeshHittestWithBVHScreen.hpp"

#include "src/Bindings/SimTKHelpers.hpp"
#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Shaders/SolidColorShader.hpp"
#include "src/Graphics/Gl.hpp"
#include "src/Graphics/GlGlm.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshData.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Maths/BVH.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/Platform/App.hpp"
#include "src/Screens/ExperimentsScreen.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>

#include <cstdint>
#include <chrono>
#include <vector>

static gl::VertexArray makeVAO(osc::SolidColorShader& shader,
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

// assumes vertex array is set. Only sets uModel and draws each frame
static void drawBVHRecursive(osc::BVH const& bvh, osc::SolidColorShader& shader, int pos)
{
    osc::BVHNode const& n = bvh.nodes[pos];

    glm::vec3 halfWidths = Dimensions(n.bounds) / 2.0f;
    glm::vec3 center = Midpoint(n.bounds);

    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, halfWidths);
    glm::mat4 mover = glm::translate(glm::mat4{1.0f}, center);
    glm::mat4 mmtx = mover * scaler;

    gl::Uniform(shader.uModel, mmtx);
    gl::DrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);

    if (n.nlhs >= 0)
    {
        // if it's an internal node
        drawBVHRecursive(bvh, shader, pos+1);
        drawBVHRecursive(bvh, shader, pos+n.nlhs+1);
    }
}

class osc::MeshHittestWithBVHScreen::Impl final {
public:

    void onMount()
    {
        osc::ImGuiInit();
        App::upd().disableVsync();
        // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
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

    void tick(float dt)
    {
        App const& app = App::get();
        UpdatePolarCameraFromImGuiUserInput(app.dims(), m_Camera);

        m_Camera.radius *= 1.0f - ImGui::GetIO().MouseWheel/10.0f;

        // handle hittest
        auto raycastStart = std::chrono::high_resolution_clock::now();
        {
            Line cameraRayWorldspace = m_Camera.unprojectTopLeftPosToWorldRay(ImGui::GetMousePos(), app.dims());
            // camera ray in worldspace == camera ray in model space because the model matrix is an identity matrix

            m_IsMousedOver = false;

            if (m_UseBVH)
            {
                BVHCollision res;
                if (BVH_GetClosestRayIndexedTriangleCollision(m_Mesh.getTriangleBVH(),
                    m_Mesh.getVerts(),
                    m_Mesh.getIndices(),
                    cameraRayWorldspace,
                    &res))
                {
                    glm::vec3 const* v = m_Mesh.getVerts().data() + res.primId;
                    m_IsMousedOver = true;
                    m_Tris[0] = v[0];
                    m_Tris[1] = v[1];
                    m_Tris[2] = v[2];
                    m_TriangleVBO.assign(m_Tris, 3);
                }
            }
            else
            {
                nonstd::span<glm::vec3 const> verts = m_Mesh.getVerts();
                for (size_t i = 0; i < verts.size(); i += 3)
                {
                    glm::vec3 tri[3] = {verts[i], verts[i+1], verts[i+2]};
                    RayCollision res = GetRayCollisionTriangle(cameraRayWorldspace, tri);
                    if (res.hit)
                    {
                        m_IsMousedOver = true;

                        // draw triangle for hit
                        m_Tris[0] = tri[0];
                        m_Tris[1] = tri[1];
                        m_Tris[2] = tri[2];
                        m_TriangleVBO.assign(m_Tris, 3);
                        break;
                    }
                }
            }

        }
        auto raycastEnd = std::chrono::high_resolution_clock::now();
        auto raycastDt = raycastEnd - raycastStart;
        m_RaycastDuration = std::chrono::duration_cast<std::chrono::microseconds>(raycastDt);
    }

    void draw()
    {
        App const& app = App::get();
        auto dims = app.idims();
        gl::Viewport(0, 0, dims.x, dims.y);

        osc::ImGuiNewFrame();

        gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // printout stats
        {
            ImGui::Begin("controls");
            ImGui::Text("raycast duration = %lld micros", m_RaycastDuration.count());
            ImGui::Checkbox("use BVH", &m_UseBVH);
            ImGui::End();
        }

        gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl::UseProgram(m_Shader.program);
        gl::Uniform(m_Shader.uModel, gl::identity);
        gl::Uniform(m_Shader.uView, m_Camera.getViewMtx());
        gl::Uniform(m_Shader.uProjection, m_Camera.getProjMtx(app.aspectRatio()));
        gl::Uniform(m_Shader.uColor, m_IsMousedOver ? glm::vec4{0.0f, 1.0f, 0.0f, 1.0f} : glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});

        // draw scene
        if (true)
        {
            gl::BindVertexArray(m_Mesh.GetVertexArray());
            m_Mesh.Draw();
            gl::BindVertexArray();
        }

        // draw hittest triangle debug
        if (m_IsMousedOver)
        {
            gl::Disable(GL_DEPTH_TEST);

            // draw triangle
            gl::Uniform(m_Shader.uModel, gl::identity);
            gl::Uniform(m_Shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});
            gl::BindVertexArray(m_TriangleVAO);
            gl::DrawElements(GL_TRIANGLES, m_TriangleEBO.sizei(), gl::indexType(m_TriangleEBO), nullptr);
            gl::BindVertexArray();

            gl::Enable(GL_DEPTH_TEST);
        }

        // draw BVH
        if (m_UseBVH && !m_Mesh.getTriangleBVH().nodes.empty())
        {
            // uModel is set by the recursive call
            gl::Uniform(m_Shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});
            gl::BindVertexArray(m_CubeVAO);
            drawBVHRecursive(m_Mesh.getTriangleBVH(), m_Shader, 0);
            gl::BindVertexArray();
        }

        osc::ImGuiRender();
    }

private:
    SolidColorShader m_Shader;

    Mesh m_Mesh = LoadMeshViaSimTK(App::resource("geometry/hat_ribs.vtp"));

    // triangle (debug)
    glm::vec3 m_Tris[3];
    gl::ArrayBuffer<glm::vec3> m_TriangleVBO;
    gl::ElementArrayBuffer<uint32_t> m_TriangleEBO = {0, 1, 2};
    gl::VertexArray m_TriangleVAO = makeVAO(m_Shader, m_TriangleVBO, m_TriangleEBO);

    // AABB wireframe
    MeshData m_CubeWireFrame = GenCubeLines();
    gl::ArrayBuffer<glm::vec3> m_CubeWireFrameVBO{m_CubeWireFrame.verts};
    gl::ElementArrayBuffer<uint32_t> m_CubeWireFrameEBO{m_CubeWireFrame.indices};
    gl::VertexArray m_CubeVAO = makeVAO(m_Shader, m_CubeWireFrameVBO, m_CubeWireFrameEBO);

    std::chrono::microseconds m_RaycastDuration{0};
    PolarPerspectiveCamera m_Camera;
    bool m_IsMousedOver = false;
    bool m_UseBVH = true;
};


// public Impl (PIMPL)

osc::MeshHittestWithBVHScreen::MeshHittestWithBVHScreen() :
    m_Impl{new Impl{}}
{
}

osc::MeshHittestWithBVHScreen::MeshHittestWithBVHScreen(MeshHittestWithBVHScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::MeshHittestWithBVHScreen& osc::MeshHittestWithBVHScreen::operator=(MeshHittestWithBVHScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::MeshHittestWithBVHScreen::~MeshHittestWithBVHScreen() noexcept
{
    delete m_Impl;
}

void osc::MeshHittestWithBVHScreen::onMount()
{
    m_Impl->onMount();
}

void osc::MeshHittestWithBVHScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::MeshHittestWithBVHScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::MeshHittestWithBVHScreen::tick(float dt)
{
    m_Impl->tick(std::move(dt));
}

void osc::MeshHittestWithBVHScreen::draw()
{
    m_Impl->draw();
}
