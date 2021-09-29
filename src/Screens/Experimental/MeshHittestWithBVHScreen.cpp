#include "MeshHittestWithBVHScreen.hpp"

#include "src/App.hpp"
#include "src/3D/BVH.hpp"
#include "src/3D/Gl.hpp"
#include "src/3D/GlGlm.hpp"
#include "src/3D/Model.hpp"
#include "src/3D/Shaders/SolidColorShader.hpp"
#include "src/Screens/Experimental/ExperimentsScreen.hpp"
#include "src/SimTKBindings/SimTKLoadMesh.hpp"
#include "src/Utils/ImGuiHelpers.hpp"

#include <imgui.h>

#include <vector>
#include <chrono>

using namespace osc;

static gl::VertexArray makeVAO(SolidColorShader& shader, gl::ArrayBuffer<glm::vec3>& vbo, gl::ElementArrayBuffer<uint32_t>& ebo) {
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
static void drawBVHRecursive(BVH const& bvh, SolidColorShader& shader, int pos) {
    BVHNode const& n = bvh.nodes[pos];

    glm::vec3 halfWidths = AABBDims(n.bounds) / 2.0f;
    glm::vec3 center = AABBCenter(n.bounds);

    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, halfWidths);
    glm::mat4 mover = glm::translate(glm::mat4{1.0f}, center);
    glm::mat4 mmtx = mover * scaler;

    gl::Uniform(shader.uModel, mmtx);
    gl::DrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);

    if (n.nlhs >= 0) {  // if it's an internal node
        drawBVHRecursive(bvh, shader, pos+1);
        drawBVHRecursive(bvh, shader, pos+n.nlhs+1);
    }
}

struct osc::MeshHittestWithBVHScreen::Impl final {
    SolidColorShader shader;

    MeshData mesh = SimTKLoadMesh(App::resource("geometry/hat_ribs.vtp"));
    gl::ArrayBuffer<glm::vec3> meshVBO{mesh.verts};
    gl::ElementArrayBuffer<uint32_t> meshEBO{mesh.indices};
    gl::VertexArray meshVAO = makeVAO(shader, meshVBO, meshEBO);
    BVH meshBVH = BVH_CreateFromTriangles(mesh.verts.data(), mesh.verts.size());

    // triangle (debug)
    glm::vec3 tris[3];
    gl::ArrayBuffer<glm::vec3> triangleVBO;
    gl::ElementArrayBuffer<uint32_t> triangleEBO = {0, 1, 2};
    gl::VertexArray triangleVAO = makeVAO(shader, triangleVBO, triangleEBO);

    // AABB wireframe
    MeshData cubeWireframe = GenCubeLines();
    gl::ArrayBuffer<glm::vec3> cubeWireframeVBO{cubeWireframe.verts};
    gl::ElementArrayBuffer<uint32_t> cubeWireframeEBO{cubeWireframe.indices};
    gl::VertexArray cubeVAO = makeVAO(shader, cubeWireframeVBO, cubeWireframeEBO);

    std::chrono::microseconds raycastDuration{0};
    PolarPerspectiveCamera camera;
    bool isMousedOver = false;
    bool useBVH = true;
};

// public Impl

osc::MeshHittestWithBVHScreen::MeshHittestWithBVHScreen() :
    m_Impl{new Impl{}} {
}

osc::MeshHittestWithBVHScreen::~MeshHittestWithBVHScreen() noexcept = default;

void osc::MeshHittestWithBVHScreen::onMount() {
    osc::ImGuiInit();
    App::cur().disableVsync();
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void osc::MeshHittestWithBVHScreen::onUnmount() {
    osc::ImGuiShutdown();
}

void osc::MeshHittestWithBVHScreen::onEvent(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        App::cur().requestTransition<ExperimentsScreen>();
        return;
    }
}

void osc::MeshHittestWithBVHScreen::tick(float) {
    Impl& impl = *m_Impl;

    UpdatePolarCameraFromImGuiUserInput(App::cur().dims(), impl.camera);

    impl.camera.radius *= 1.0f - ImGui::GetIO().MouseWheel/10.0f;

    // handle hittest
    auto raycastStart = std::chrono::high_resolution_clock::now();
    {
        Line cameraRayWorldspace = impl.camera.unprojectScreenposToWorldRay(ImGui::GetMousePos(), App::cur().dims());
        // camera ray in worldspace == camera ray in model space because the model matrix is an identity matrix

        impl.isMousedOver = false;

        if (impl.useBVH) {
            BVHCollision res;
            if (BVH_GetClosestRayTriangleCollision(impl.meshBVH, impl.mesh.verts.data(), impl.mesh.verts.size(), cameraRayWorldspace, &res)) {
                glm::vec3 const* v = impl.mesh.verts.data() + res.primId;
                impl.isMousedOver = true;
                impl.tris[0] = v[0];
                impl.tris[1] = v[1];
                impl.tris[2] = v[2];
                impl.triangleVBO.assign(impl.tris, 3);
            }

        } else {
            auto const& verts = impl.mesh.verts;
            for (size_t i = 0; i < verts.size(); i += 3) {
                glm::vec3 tri[3] = {verts[i], verts[i+1], verts[i+2]};
                RayCollision res = GetRayCollisionTriangle(cameraRayWorldspace, tri);
                if (res.hit) {
                    impl.isMousedOver = true;

                    // draw triangle for hit
                    impl.tris[0] = tri[0];
                    impl.tris[1] = tri[1];
                    impl.tris[2] = tri[2];
                    impl.triangleVBO.assign(impl.tris, 3);
                    break;
                }
            }
        }

    }
    auto raycastEnd = std::chrono::high_resolution_clock::now();
    auto raycastDt = raycastEnd - raycastStart;
    impl.raycastDuration = std::chrono::duration_cast<std::chrono::microseconds>(raycastDt);
}

void osc::MeshHittestWithBVHScreen::draw() {
    auto dims = App::cur().idims();
    gl::Viewport(0, 0, dims.x, dims.y);

    osc::ImGuiNewFrame();

    Impl& impl = *m_Impl;
    SolidColorShader& shader = impl.shader;

    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // printout stats
    {
        ImGui::Begin("controls");
        ImGui::Text("raycast duration = %lld micros", impl.raycastDuration.count());
        ImGui::Checkbox("use BVH", &impl.useBVH);
        ImGui::End();
    }

    gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::UseProgram(shader.program);
    gl::Uniform(shader.uModel, gl::identity);
    gl::Uniform(shader.uView, impl.camera.getViewMtx());
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(App::cur().aspectRatio()));
    gl::Uniform(shader.uColor, impl.isMousedOver ? glm::vec4{0.0f, 1.0f, 0.0f, 1.0f} : glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
    if (true) {  // draw scene
        gl::BindVertexArray(impl.meshVAO);
        gl::DrawElements(GL_TRIANGLES, impl.meshEBO.sizei(), gl::indexType(impl.meshEBO), nullptr);
        gl::BindVertexArray();
    }

    // draw hittest triangle debug
    if (impl.isMousedOver) {
        gl::Disable(GL_DEPTH_TEST);

        // draw triangle
        gl::Uniform(shader.uModel, gl::identity);
        gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});
        gl::BindVertexArray(impl.triangleVAO);
        gl::DrawElements(GL_TRIANGLES, impl.triangleEBO.sizei(), gl::indexType(impl.triangleEBO), nullptr);
        gl::BindVertexArray();

        gl::Enable(GL_DEPTH_TEST);
    }

    // draw BVH
    if (impl.useBVH && !impl.meshBVH.nodes.empty()) {
        // uModel is set by the recursive call

        gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});
        gl::BindVertexArray(impl.cubeVAO);
        drawBVHRecursive(impl.meshBVH, impl.shader, 0);
        gl::BindVertexArray();
    }

    osc::ImGuiRender();
}
