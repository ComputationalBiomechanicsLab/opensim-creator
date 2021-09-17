#include "HittestScreen.hpp"

#include "src/App.hpp"
#include "src/Log.hpp"
#include "src/3D/Constants.hpp"
#include "src/3D/Gl.hpp"
#include "src/3D/GlGlm.hpp"
#include "src/3D/Model.hpp"
#include "src/Screens/Experimental/ExperimentsScreen.hpp"
#include "src/Utils/IoPoller.hpp"

#include <glm/vec3.hpp>

#include <algorithm>
#include <array>

static char const g_VertexShader[] = R"(
    #version 330 core

    uniform mat4 uModel;
    uniform mat4 uView;
    uniform mat4 uProjection;

    layout (location = 0) in vec3 aPos;

    void main() {
        gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    }
)";

static char const g_FragmentShader[] = R"(
    #version 330 core

    uniform vec4 uColor;

    out vec4 FragColor;

    void main() {
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

        SceneSphere(glm::vec3 pos_) : pos{pos_} {
        }
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
static gl::VertexArray makeVAO(BasicShader& shader, gl::ArrayBuffer<glm::vec3>& vbo) {
    gl::VertexArray rv;
    gl::BindVertexArray(rv);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(shader.aPos, false, sizeof(glm::vec3), 0);
    gl::EnableVertexAttribArray(shader.aPos);
    gl::BindVertexArray();
    return rv;
}

static std::vector<SceneSphere> generateSceneSpheres() {
    constexpr int min = -30;
    constexpr int max = 30;
    constexpr int step = 6;

    std::vector<SceneSphere> rv;
    for (int x = min; x <= max; x += step) {
        for (int y = min; y <= max; y += step) {
            for (int z = min; z <= max; z += step) {
                rv.emplace_back(glm::vec3{x, 50.0f + 2.0f*y, z});
            }
        }
    }
    return rv;
}

// screen impl.
struct osc::HittestScreen::Impl final {
    IoPoller io;

    BasicShader shader;

    // sphere datas
    std::vector<glm::vec3> sphereVerts = GenUntexturedUVSphere(12, 12).verts;
    AABB sphereAABBs = AABBFromVerts(sphereVerts.data(), sphereVerts.size());
    Sphere sphereBoundingSphere = BoundingSphereFromVerts(sphereVerts.data(), sphereVerts.size());
    gl::ArrayBuffer<glm::vec3> sphereVBO{sphereVerts};
    gl::VertexArray sphereVAO = makeVAO(shader, sphereVBO);

    // sphere instances
    std::vector<SceneSphere> spheres = generateSceneSpheres();

    // crosshair
    gl::ArrayBuffer<glm::vec3> crosshairVBO{g_CrosshairVerts};
    gl::VertexArray crosshairVAO = makeVAO(shader, crosshairVBO);

    // wireframe cube
    gl::ArrayBuffer<glm::vec3> cubeWireframeVBO{GenCubeLines().verts};
    gl::VertexArray cubeWireframeVAO = makeVAO(shader, cubeWireframeVBO);

    // circle
    gl::ArrayBuffer<glm::vec3> circleVBO{GenCircle(36).verts};
    gl::VertexArray circleVAO = makeVAO(shader, circleVBO);

    // triangle
    std::array<glm::vec3, 3> triangle = {{
        {-10.0f, -10.0f, 0.0f},
        {+0.0f, +10.0f, 0.0f},
        {+10.0f, -10.0f, 0.0f},
    }};
    gl::ArrayBuffer<glm::vec3> triangleVBO{triangle};
    gl::VertexArray triangleVAO = makeVAO(shader, triangleVBO);

    EulerPerspectiveCamera camera;
    bool showAABBs = true;
};


// public API

osc::HittestScreen::HittestScreen() :
    m_Impl{new Impl{}} {
}

osc::HittestScreen::~HittestScreen() noexcept = default;

void osc::HittestScreen::onMount() {
    App::cur().setRelativeMouseMode(true);
    gl::Disable(GL_CULL_FACE);
}

void osc::HittestScreen::onUnmount() {
    App::cur().setRelativeMouseMode(false);
    gl::Enable(GL_CULL_FACE);
}

void osc::HittestScreen::onEvent(SDL_Event const& e) {
    m_Impl->io.onEvent(e);

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        App::cur().requestTransition<ExperimentsScreen>();
    }
}

void osc::HittestScreen::tick(float) {
    auto& camera = m_Impl->camera;
    auto& io = m_Impl->io;
    io.onUpdate();

    float speed = 10.0f;
    float sensitivity = 0.005f;

    if (io.KeysDown[SDL_SCANCODE_ESCAPE]) {
        App::cur().requestTransition<ExperimentsScreen>();
    }

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
    camera.pitch = std::clamp(camera.pitch, -fpi2 + 0.1f, fpi2 - 0.1f);
    io.WantMousePosWarpTo = true;
    io.MousePosWarpTo = io.DisplaySize/2.0f;


    // compute hits

    Line cameraRay;
    cameraRay.origin = camera.pos;
    cameraRay.dir = camera.getFront();

    float closestEl = FLT_MAX;
    SceneSphere* closestSceneSphere = nullptr;

    for (SceneSphere& ss : m_Impl->spheres) {
        ss.isHovered = false;

        Sphere s;
        s.origin = ss.pos;
        s.radius = m_Impl->sphereBoundingSphere.radius;

        RayCollision res = GetRayCollisionSphere(cameraRay, s);
        if (res.hit && res.distance >= 0.0f && res.distance < closestEl) {
            closestEl = res.distance;
            closestSceneSphere = &ss;
        }
    }

    if (closestSceneSphere) {
        closestSceneSphere->isHovered = true;
    }

}

void osc::HittestScreen::draw() {
    App& app = App::cur();
    Impl& impl = *m_Impl;
    auto& shader = impl.shader;

    Line cameraRay;
    cameraRay.dir = impl.camera.getFront();
    cameraRay.origin = impl.camera.pos;

    gl::Viewport(0, 0, App::cur().idims().x, App::cur().idims().y);
    gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::UseProgram(shader.prog);
    gl::Uniform(shader.uView, impl.camera.getViewMtx());
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(app.aspectRatio()));

    // main scene render
    if (true) {
        gl::BindVertexArray(impl.sphereVAO);
        for (SceneSphere const& s : impl.spheres) {
            glm::vec4 color = s.isHovered ?
                glm::vec4{0.0f, 0.0f, 1.0f, 1.0f} :
                glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};

            gl::Uniform(shader.uColor, color);
            gl::Uniform(shader.uModel, glm::translate(glm::mat4{1.0f}, s.pos));
            gl::DrawArrays(GL_TRIANGLES, 0, impl.sphereVBO.sizei());
        }
        gl::BindVertexArray();
    }

    // AABBs
    if (impl.showAABBs) {
        gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});

        glm::vec3 halfWidths = AABBDims(impl.sphereAABBs) / 2.0f;
        glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, halfWidths);

        gl::BindVertexArray(impl.cubeWireframeVAO);
        for (SceneSphere const& s : impl.spheres) {
            glm::mat4 mover = glm::translate(glm::mat4{1.0f}, s.pos);
            gl::Uniform(shader.uModel, mover * scaler);
            gl::DrawArrays(GL_LINES, 0, impl.cubeWireframeVBO.sizei());
        }
        gl::BindVertexArray();
    }

    // draw disc
    if (true) {
        Disc d;
        d.origin = {0.0f, 0.0f, 0.0f};
        d.normal = {0.0f, 1.0f, 0.0f};
        d.radius = {10.0f};

        RayCollision res = GetRayCollisionDisc(cameraRay, d);

        glm::vec4 color = res.hit ?
            glm::vec4{0.0f, 0.0f, 1.0f, 1.0f} :
            glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};

        Disc meshDisc{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 1.0f};

        gl::Uniform(shader.uModel, DiscToDiscXform(meshDisc, d));
        gl::Uniform(shader.uColor, color);
        gl::BindVertexArray(impl.circleVAO);
        gl::DrawArrays(GL_TRIANGLES, 0, impl.circleVBO.sizei());
        gl::BindVertexArray();
    }

    // draw triangle
    if (true) {
        RayCollision res = GetRayCollisionTriangle(cameraRay, impl.triangle.data());

        glm::vec4 color = res.hit ?
            glm::vec4{0.0f, 0.0f, 1.0f, 1.0f} :
            glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};

        gl::Uniform(shader.uModel, gl::identity);
        gl::Uniform(shader.uColor, color);
        gl::BindVertexArray(impl.triangleVAO);
        gl::DrawArrays(GL_TRIANGLES, 0, impl.triangleVBO.sizei());
        gl::BindVertexArray();
    }

    // crosshair
    if (true) {
        gl::Uniform(shader.uModel, gl::identity);
        gl::Uniform(shader.uView, gl::identity);
        gl::Uniform(shader.uProjection, gl::identity);
        gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});
        gl::BindVertexArray(impl.crosshairVAO);
        gl::DrawArrays(GL_LINES, 0, impl.crosshairVBO.sizei());
        gl::BindVertexArray();
    }
}
