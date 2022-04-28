#include "HittestScreen.hpp"

#include "src/Graphics/Gl.hpp"
#include "src/Graphics/GlGlm.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Disc.hpp"
#include "src/Maths/EulerPerspectiveCamera.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Line.hpp"
#include "src/Maths/RayCollision.hpp"
#include "src/Maths/Sphere.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/IoPoller.hpp"
#include "src/Platform/Log.hpp"
#include "src/Screens/ExperimentsScreen.hpp"

#include <glm/vec3.hpp>

#include <algorithm>
#include <array>
#include <iostream>

static char const g_VertexShader[] = R"(
    #version 330 core

    uniform mat4 uModel;
    uniform mat4 uView;
    uniform mat4 uProjection;

    layout (location = 0) in vec3 aPos;

    void main()
    {
        gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    }
)";

static char const g_FragmentShader[] = R"(
    #version 330 core

    uniform vec4 uColor;

    out vec4 FragColor;

    void main()
    {
        FragColor = uColor;
    }
)";

namespace {

    // basic shader that just colors the geometry in
    struct BasicShader final {
        gl::Program prog = gl::CreateProgramFrom(
            gl::CompileFromSource<gl::VertexShader>(g_VertexShader),
            gl::CompileFromSource<gl::FragmentShader>(g_FragmentShader));

        gl::AttributeVec3 aPos{0};

        gl::UniformMat4 uModel = gl::GetUniformLocation(prog, "uModel");
        gl::UniformMat4 uView = gl::GetUniformLocation(prog, "uView");
        gl::UniformMat4 uProjection = gl::GetUniformLocation(prog, "uProjection");
        gl::UniformVec4 uColor = gl::GetUniformLocation(prog, "uColor");
    };

    struct SceneSphere final {
        glm::vec3 pos;
        bool isHovered = false;

        SceneSphere(glm::vec3 pos_) : pos{pos_} {}
    };
}

static constexpr std::array<glm::vec3, 4> g_CrosshairVerts = {{
    // -X to +X
    {-0.05f, 0.0f, 0.0f},
    {+0.05f, 0.0f, 0.0f},

    // -Y to +Y
    {0.0f, -0.05f, 0.0f},
    {0.0f, +0.05f, 0.0f}
}};

// make a VAO for the basic shader
static gl::VertexArray makeVAO(BasicShader& shader, gl::ArrayBuffer<glm::vec3>& vbo)
{
    gl::VertexArray rv;
    gl::BindVertexArray(rv);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(shader.aPos, false, sizeof(glm::vec3), 0);
    gl::EnableVertexAttribArray(shader.aPos);
    gl::BindVertexArray();
    return rv;
}

static std::vector<SceneSphere> generateSceneSpheres()
{
    constexpr int min = -30;
    constexpr int max = 30;
    constexpr int step = 6;

    std::vector<SceneSphere> rv;
    for (int x = min; x <= max; x += step)
    {
        for (int y = min; y <= max; y += step)
        {
            for (int z = min; z <= max; z += step)
            {
                rv.emplace_back(glm::vec3{x, 50.0f + 2.0f*y, z});
            }
        }
    }
    return rv;
}

// screen impl.
class osc::HittestScreen::Impl final {
public:

    void onMount()
    {
        App::cur().setShowCursor(false);
        gl::Disable(GL_CULL_FACE);
    }

    void onUnmount()
    {
        App::cur().setShowCursor(true);
        gl::Enable(GL_CULL_FACE);
    }

    void onEvent(SDL_Event const& e)
    {
        m_IoPoller.onEvent(e);  // feed the IoPoller

        if (e.type == SDL_QUIT)
        {
            App::cur().requestQuit();
            return;
        }
        else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        {
            App::cur().requestTransition<ExperimentsScreen>();
            return;
        }
    }

    void tick(float dt)
    {
        m_IoPoller.onUpdate();

        float speed = 10.0f;
        float sensitivity = 0.005f;

        if (m_IoPoller.KeysDown[SDL_SCANCODE_ESCAPE])
        {
            App::cur().requestTransition<ExperimentsScreen>();
        }

        if (m_IoPoller.KeysDown[SDL_SCANCODE_W])
        {
            m_SceneCamera.pos += speed * m_SceneCamera.getFront() * m_IoPoller.DeltaTime;
        }

        if (m_IoPoller.KeysDown[SDL_SCANCODE_S])
        {
            m_SceneCamera.pos -= speed * m_SceneCamera.getFront() * m_IoPoller.DeltaTime;
        }

        if (m_IoPoller.KeysDown[SDL_SCANCODE_A])
        {
            m_SceneCamera.pos -= speed * m_SceneCamera.getRight() * m_IoPoller.DeltaTime;
        }

        if (m_IoPoller.KeysDown[SDL_SCANCODE_D])
        {
            m_SceneCamera.pos += speed * m_SceneCamera.getRight() * m_IoPoller.DeltaTime;
        }

        if (m_IoPoller.KeysDown[SDL_SCANCODE_SPACE])
        {
            m_SceneCamera.pos += speed * m_SceneCamera.getUp() * m_IoPoller.DeltaTime;
        }

        if (m_IoPoller.KeyCtrl)
        {
            m_SceneCamera.pos -= speed * m_SceneCamera.getUp() * m_IoPoller.DeltaTime;
        }

        m_SceneCamera.yaw += sensitivity * m_IoPoller.MouseDelta.x;
        m_SceneCamera.pitch  -= sensitivity * m_IoPoller.MouseDelta.y;
        m_SceneCamera.pitch = std::clamp(m_SceneCamera.pitch, -fpi2 + 0.1f, fpi2 - 0.1f);
        m_IoPoller.WantMousePosWarpTo = true;
        m_IoPoller.MousePosWarpTo = m_IoPoller.DisplaySize/2.0f;

        // compute hits

        Line cameraRay;
        cameraRay.origin = m_SceneCamera.pos;
        cameraRay.dir = m_SceneCamera.getFront();

        float closestEl = FLT_MAX;
        SceneSphere* closestSceneSphere = nullptr;

        for (SceneSphere& ss : m_Spheres)
        {
            ss.isHovered = false;

            Sphere s;
            s.origin = ss.pos;
            s.radius = m_SphereBoundingSphere.radius;

            RayCollision res = GetRayCollisionSphere(cameraRay, s);
            if (res.hit && res.distance >= 0.0f && res.distance < closestEl)
            {
                closestEl = res.distance;
                closestSceneSphere = &ss;
            }
        }

        if (closestSceneSphere)
        {
            closestSceneSphere->isHovered = true;
        }
    }

    void draw()
    {
        App& app = App::cur();

        Line cameraRay;
        cameraRay.dir = m_SceneCamera.getFront();
        cameraRay.origin = m_SceneCamera.pos;

        gl::Viewport(0, 0, App::cur().idims().x, App::cur().idims().y);
        gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl::UseProgram(m_Shader.prog);
        gl::Uniform(m_Shader.uView, m_SceneCamera.getViewMtx());
        gl::Uniform(m_Shader.uProjection, m_SceneCamera.getProjMtx(app.aspectRatio()));

        // main scene render
        {
            gl::BindVertexArray(m_SphereVAO);
            for (SceneSphere const& s : m_Spheres)
            {
                glm::vec4 color = s.isHovered ?
                    glm::vec4{0.0f, 0.0f, 1.0f, 1.0f} :
                    glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};

                gl::Uniform(m_Shader.uColor, color);
                gl::Uniform(m_Shader.uModel, glm::translate(glm::mat4{1.0f}, s.pos));
                gl::DrawArrays(GL_TRIANGLES, 0, m_SphereVBO.sizei());
            }
            gl::BindVertexArray();
        }

        // AABBs
        if (m_ShowingAABBs)
        {
            gl::Uniform(m_Shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});

            glm::vec3 halfWidths = Dimensions(m_SphereAABBs) / 2.0f;
            glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, halfWidths);

            gl::BindVertexArray(m_CubeWireframeVAO);
            for (SceneSphere const& s : m_Spheres)
            {
                glm::mat4 mover = glm::translate(glm::mat4{1.0f}, s.pos);
                gl::Uniform(m_Shader.uModel, mover * scaler);
                gl::DrawArrays(GL_LINES, 0, m_CubeWireframeVBO.sizei());
            }
            gl::BindVertexArray();
        }

        // draw disc
        {
            Disc d;
            d.origin = {0.0f, 0.0f, 0.0f};
            d.normal = {0.0f, 1.0f, 0.0f};
            d.radius = {10.0f};

            RayCollision res = GetRayCollisionDisc(cameraRay, d);

            glm::vec4 color = res.hit ?
                glm::vec4{0.0f, 0.0f, 1.0f, 1.0f} :
                glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};

            Disc meshDisc{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 1.0f};

            gl::Uniform(m_Shader.uModel, DiscToDiscMat4(meshDisc, d));
            gl::Uniform(m_Shader.uColor, color);
            gl::BindVertexArray(m_CircleVAO);
            gl::DrawArrays(GL_TRIANGLES, 0, m_CircleVBO.sizei());
            gl::BindVertexArray();
        }

        // triangle
        {
            RayCollision res = GetRayCollisionTriangle(cameraRay, m_TriangleVerts.data());

            glm::vec4 color = res.hit ?
                glm::vec4{0.0f, 0.0f, 1.0f, 1.0f} :
                glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};

            gl::Uniform(m_Shader.uModel, gl::identity);
            gl::Uniform(m_Shader.uColor, color);
            gl::BindVertexArray(m_TriangleVAO);
            gl::DrawArrays(GL_TRIANGLES, 0, m_TriangleVBO.sizei());
            gl::BindVertexArray();
        }

        // crosshair
        {
            gl::Uniform(m_Shader.uModel, gl::identity);
            gl::Uniform(m_Shader.uView, gl::identity);
            gl::Uniform(m_Shader.uProjection, gl::identity);
            gl::Uniform(m_Shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});
            gl::BindVertexArray(m_CrosshairVAO);
            gl::DrawArrays(GL_LINES, 0, m_CrosshairVBO.sizei());
            gl::BindVertexArray();
        }
    }

private:
    IoPoller m_IoPoller;

    BasicShader m_Shader;

    // sphere datas
    std::vector<glm::vec3> m_SphereVerts = GenUntexturedUVSphere(12, 12).verts;
    AABB m_SphereAABBs = AABBFromVerts(m_SphereVerts.data(), m_SphereVerts.size());
    Sphere m_SphereBoundingSphere = BoundingSphereOf(m_SphereVerts.data(), m_SphereVerts.size());
    gl::ArrayBuffer<glm::vec3> m_SphereVBO{m_SphereVerts};
    gl::VertexArray m_SphereVAO = makeVAO(m_Shader, m_SphereVBO);

    // sphere instances
    std::vector<SceneSphere> m_Spheres = generateSceneSpheres();

    // crosshair
    gl::ArrayBuffer<glm::vec3> m_CrosshairVBO{g_CrosshairVerts};
    gl::VertexArray m_CrosshairVAO = makeVAO(m_Shader, m_CrosshairVBO);

    // wireframe cube
    gl::ArrayBuffer<glm::vec3> m_CubeWireframeVBO{GenCubeLines().verts};
    gl::VertexArray m_CubeWireframeVAO = makeVAO(m_Shader, m_CubeWireframeVBO);

    // circle
    gl::ArrayBuffer<glm::vec3> m_CircleVBO{GenCircle(36).verts};
    gl::VertexArray m_CircleVAO = makeVAO(m_Shader, m_CircleVBO);

    // triangle
    std::array<glm::vec3, 3> m_TriangleVerts = {{
        {-10.0f, -10.0f, 0.0f},
        {+0.0f, +10.0f, 0.0f},
        {+10.0f, -10.0f, 0.0f},
    }};
    gl::ArrayBuffer<glm::vec3> m_TriangleVBO{m_TriangleVerts};
    gl::VertexArray m_TriangleVAO = makeVAO(m_Shader, m_TriangleVBO);

    EulerPerspectiveCamera m_SceneCamera;
    bool m_ShowingAABBs = true;
};


// public API (PIMPL)

osc::HittestScreen::HittestScreen() :
    m_Impl{new Impl{}}
{
}

osc::HittestScreen::HittestScreen(HittestScreen&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::HittestScreen& osc::HittestScreen::operator=(HittestScreen&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::HittestScreen::~HittestScreen() noexcept
{
    delete m_Impl;
}

void osc::HittestScreen::onMount()
{
    m_Impl->onMount();
}

void osc::HittestScreen::onUnmount()
{
    m_Impl->onUnmount();
}

void osc::HittestScreen::onEvent(SDL_Event const& e)
{
    m_Impl->onEvent(e);
}

void osc::HittestScreen::tick(float dt)
{
    m_Impl->tick(std::move(dt));
}

void osc::HittestScreen::draw()
{
    m_Impl->draw();
}
