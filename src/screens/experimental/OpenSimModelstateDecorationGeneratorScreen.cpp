#include "OpenSimModelstateDecorationGeneratorScreen.hpp"

#include "src/App.hpp"
#include "src/3d/Gl.hpp"
#include "src/3d/GlGlm.hpp"
#include "src/3d/InstancedRenderer.hpp"
#include "src/3d/shaders/SolidColorShader.hpp"
#include "src/screens/experimental/ExperimentsScreen.hpp"
#include "src/opensim_bindings/SceneGenerator.hpp"
#include "src/utils/ImGuiHelpers.hpp"
#include "src/utils/Perf.hpp"

#include <imgui.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SimTKcommon.h>

using namespace osc;

static gl::VertexArray makeVAO(SolidColorShader& scs, gl::ArrayBuffer<glm::vec3>& vbo) {
    gl::VertexArray rv;
    gl::BindVertexArray(rv);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(scs.aPos, false, sizeof(glm::vec3), 0);
    gl::EnableVertexAttribArray(scs.aPos);
    gl::BindVertexArray();
    return rv;
}

struct osc::OpenSimModelstateDecorationGeneratorScreen::Impl final {
    InstancedRenderer renderer;
    InstancedDrawlist drawlist;
    InstancedRendererParams renderParams;

    SceneGenerator generator;
    SceneDecorations sceneDecorations;
    std::vector<unsigned char> rimHighlights;

    //OpenSim::Model model{App::resource("models/RajagopalModel/Rajagopal2015.osim").string()};
    //OpenSim::Model model{App::resource("models/GeometryBackendTest/full.osim").string()};
    OpenSim::Model model{App::resource("models/Arm26/arm26.osim").string()};
    //OpenSim::Model model{App::resource("models/ToyLanding/ToyLandingModel.osim").string()};
    SimTK::State state = [this]() {
        model.finalizeFromProperties();
        model.finalizeConnections();
        SimTK::State s = model.initSystem();
        model.realizeReport(s);
        return s;
    }();
    PolarPerspectiveCamera camera;

    BasicPerfTimer timerMeshgen;
    BasicPerfTimer timerSort;
    BasicPerfTimer timerRender;
    BasicPerfTimer timerBlit;
    BasicPerfTimer timerSceneHittest;
    BasicPerfTimer timerTriangleHittest;
    BasicPerfTimer timerE2E;

    SolidColorShader scs;
    Mesh wireframeMesh = GenCubeLines();
    gl::ArrayBuffer<glm::vec3> wireframeVBO{wireframeMesh.verts};
    gl::VertexArray wireframeVAO = makeVAO(scs, wireframeVBO);

    gl::ArrayBuffer<glm::vec3> triangleVBO;
    gl::VertexArray triangleVAO = makeVAO(scs, triangleVBO);

    std::vector<BVHCollision> hitAABBs;
    std::vector<BVHCollision> hitTrisBVHCache;
    struct TriangleCollision {
        int instanceidx;
        BVHCollision collision;
    };
    std::vector<TriangleCollision> hitTris;

    bool generateDecorationsOnEachFrame = true;
    bool optimizeDrawOrder = true;
    bool drawScene = true;
    bool drawRims = false;
    bool drawAABBs = false;
    bool doSceneHittest = true;
    bool doTriangleHittest = true;
    bool drawTriangleIntersection = true;
};

// public API

osc::OpenSimModelstateDecorationGeneratorScreen::OpenSimModelstateDecorationGeneratorScreen() :
    m_Impl{new Impl{}} {

    App::cur().disableVsync();
    m_Impl->model.updDisplayHints().set_show_frames(true);
    m_Impl->model.updDisplayHints().set_show_wrap_geometry(true);
    m_Impl->generator.generate(
        m_Impl->model,
        m_Impl->state,
        m_Impl->model.getDisplayHints(),
        SceneGeneratorFlags_Default,
        1.0f,
        m_Impl->sceneDecorations);
}

osc::OpenSimModelstateDecorationGeneratorScreen::~OpenSimModelstateDecorationGeneratorScreen() noexcept = default;

void osc::OpenSimModelstateDecorationGeneratorScreen::onMount() {
    osc::ImGuiInit();
}

void osc::OpenSimModelstateDecorationGeneratorScreen::onUnmount() {
    osc::ImGuiShutdown();
}

void osc::OpenSimModelstateDecorationGeneratorScreen::onEvent(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        App::cur().requestTransition<ExperimentsScreen>();
        return;
    }
}

void osc::OpenSimModelstateDecorationGeneratorScreen::draw() {
    auto guard = m_Impl->timerE2E.measure();

    osc::ImGuiNewFrame();

    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Impl& s = *m_Impl;

    UpdatePolarCameraFromImGuiUserInput(App::cur().dims(), m_Impl->camera);

    // decoration generation
    if (s.generateDecorationsOnEachFrame) {
        auto meshgenTimerGuard = s.timerMeshgen.measure();
        s.generator.generate(
            s.model,
            s.state,
            s.model.getDisplayHints(),
            SceneGeneratorFlags_Default,
            1.0f,
            s.sceneDecorations);
    }

    // do scene hittest
    s.hitAABBs.clear();
    if (s.doSceneHittest) {
        auto sceneHittestGuard = s.timerSceneHittest.measure();
        Line rayWorldspace = s.camera.unprojectScreenposToWorldRay(ImGui::GetIO().MousePos, App::cur().dims());
        SceneDecorations const& decs = s.sceneDecorations;
        BVH_GetRayAABBCollisions(decs.sceneBVH, rayWorldspace, &s.hitAABBs);
    }

    // do triangle hittest
    s.hitTris.clear();
    if (s.doSceneHittest && s.doTriangleHittest) {
        auto triangleHittestGuard = s.timerTriangleHittest.measure();

        Line rayWorldspace = s.camera.unprojectScreenposToWorldRay(ImGui::GetIO().MousePos, App::cur().dims());

        // can just iterate through the scene-level hittest result
        for (BVHCollision const& c : s.hitAABBs) {

            // then use a triangle-level BVH to figure out which triangles intersect
            glm::mat4 const& modelMtx = s.sceneDecorations.modelMtxs[c.primId];
            CPUMesh const& mesh = *s.sceneDecorations.cpuMeshes[c.primId];

            Line rayModelspace = LineApplyXform(rayWorldspace, glm::inverse(modelMtx));

            if (BVH_GetRayTriangleCollisions(mesh.triangleBVH, mesh.data.verts.data(), mesh.data.verts.size(), rayModelspace, &s.hitTrisBVHCache)) {
                for (BVHCollision const& tri_collision : s.hitTrisBVHCache) {
                    s.hitTris.push_back(Impl::TriangleCollision{c.primId, tri_collision});
                }
                s.hitTrisBVHCache.clear();
            }
        }
    }

    // perform object highlighting
    if (s.drawTriangleIntersection && !s.hitTris.empty()) {
        // get closest triangle
        auto& ts = s.hitTris;
        auto isClosestCollision = [](Impl::TriangleCollision const& a, Impl::TriangleCollision const& b) {
            return a.collision.distance < b.collision.distance;
        };
        std::sort(ts.begin(), ts.end(), isClosestCollision);
        Impl::TriangleCollision closest = ts.front();

        // populate rim highlights
        s.rimHighlights.clear();
        s.rimHighlights.resize(s.sceneDecorations.modelMtxs.size(), 0x00);
        s.rimHighlights[closest.instanceidx] = 0xff;
    }

    // GPU upload, with object highlighting
    {
        auto sortGuard = s.timerSort.measure();

        DrawlistCompilerInput inp;
        inp.ninstances = s.sceneDecorations.modelMtxs.size();
        inp.modelMtxs = s.sceneDecorations.modelMtxs.data();
        inp.normalMtxs = s.sceneDecorations.normalMtxs.data();
        inp.colors = s.sceneDecorations.cols.data();
        inp.meshes = s.sceneDecorations.gpuMeshes.data();
        inp.textures = nullptr;
        s.rimHighlights.resize(s.sceneDecorations.modelMtxs.size());
        inp.rimIntensities = s.rimHighlights.data();

        uploadInputsToDrawlist(inp, s.drawlist);
    }

    // draw the scene
    if (s.drawScene) {
        // render the decorations into the renderer's texture
        s.renderer.setDims(App::cur().idims());
        s.renderer.setMsxaaSamples(App::cur().getSamples());
        s.renderParams.projMtx = s.camera.getProjMtx(s.renderer.getAspectRatio());
        s.renderParams.viewMtx = s.camera.getViewMtx();
        s.renderParams.viewPos = s.camera.getPos();
        if (s.drawRims) {
            s.renderParams.flags |= InstancedRendererFlags_DrawRims;
        } else {
            s.renderParams.flags &= ~InstancedRendererFlags_DrawRims;
        }

        auto renderGuard = s.timerRender.measure();
        s.renderer.render(s.renderParams, s.drawlist);

        glFlush();
    }

    // draw the AABBs
    if (s.drawAABBs) {
        gl::BindFramebuffer(GL_FRAMEBUFFER, s.renderer.getOutputFbo());

        gl::UseProgram(s.scs.program);
        gl::BindVertexArray(s.wireframeVAO);
        gl::Uniform(s.scs.uProjection, s.camera.getProjMtx(s.renderer.getAspectRatio()));
        gl::Uniform(s.scs.uView, s.camera.getViewMtx());
        for (size_t i = 0; i < s.sceneDecorations.aabbs.size(); ++i) {
            AABB const& aabb = s.sceneDecorations.aabbs[i];

            glm::vec3 halfWidths = AABBDims(aabb) / 2.0f;
            glm::vec3 center = AABBCenter(aabb);

            glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, halfWidths);
            glm::mat4 mover = glm::translate(glm::mat4{1.0f}, center);
            glm::mat4 mmtx = mover * scaler;

            bool hit = std::find_if(s.hitAABBs.begin(), s.hitAABBs.end(), [id = static_cast<int>(i)](BVHCollision const& c) {
                return c.primId == id;
            }) != s.hitAABBs.end();

            if (hit) {
                gl::Uniform(s.scs.uColor, {1.0f, 0.0f, 0.0f, 1.0f});
            } else {
                gl::Uniform(s.scs.uColor, {0.0f, 0.0f, 0.0f, 1.0f});
            }
            gl::Uniform(s.scs.uModel, mmtx);

            gl::DrawArrays(GL_LINES, 0, s.wireframeVBO.sizei());
        }
        gl::BindVertexArray();

        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
    }

    // draw closest triangle intersection
    if (s.drawTriangleIntersection && !s.hitTris.empty()) {
        // get closest triangle
        auto& ts = s.hitTris;
        auto isClosestCollision = [](Impl::TriangleCollision const& a, Impl::TriangleCollision const& b) {
            return a.collision.distance < b.collision.distance;
        };
        std::sort(ts.begin(), ts.end(), isClosestCollision);
        Impl::TriangleCollision closest = ts.front();

        // upload triangle to GPU
        CPUMesh const& m = *s.sceneDecorations.cpuMeshes[closest.instanceidx];
        glm::vec3 const* tristart = m.data.verts.data() + closest.collision.primId;
        glm::mat4 model2world = s.sceneDecorations.modelMtxs[closest.instanceidx];
        glm::vec3 triWorldpsace[] = {
            model2world * glm::vec4{tristart[0], 1.0f},
            model2world * glm::vec4{tristart[1], 1.0f},
            model2world * glm::vec4{tristart[2], 1.0f},
        };
        s.triangleVBO.assign(triWorldpsace, 3);

        // draw the triangle
        gl::BindFramebuffer(GL_FRAMEBUFFER, s.renderer.getOutputFbo());
        gl::UseProgram(s.scs.program);
        gl::Uniform(s.scs.uColor, {0.0f, 0.0f, 0.0f, 1.0f});
        gl::Uniform(s.scs.uProjection, s.camera.getProjMtx(s.renderer.getAspectRatio()));
        gl::Uniform(s.scs.uView, s.camera.getViewMtx());
        gl::Uniform(s.scs.uModel, gl::identity);
        gl::BindVertexArray(s.triangleVAO);
        gl::Disable(GL_DEPTH_TEST);
        gl::DrawArrays(GL_TRIANGLES, 0, 3);
        gl::Enable(GL_DEPTH_TEST);
        gl::BindVertexArray();
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
    }

    // blit the scene
    {
        auto blitGuard = s.timerBlit.measure();

        gl::Texture2D& render = s.renderer.getOutputTexture();

        // draw texture using ImGui::Image
        ImGui::SetNextWindowPos({0.0f, 0.0f});
        ImGui::SetNextWindowSize(App::cur().dims());
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, {1.0f, 0.0f, 0.0f, 1.0f});
        ImGui::Begin("render output", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoSavedSettings);
        void* texture_handle = reinterpret_cast<void*>(static_cast<uintptr_t>(render.get()));
        ImVec2 image_dimensions{App::cur().dims()};
        ImVec2 uv0{0, 1};
        ImVec2 uv1{1, 0};
        ImGui::Image(texture_handle, image_dimensions, uv0, uv1);
        ImGui::SetCursorPos({0.0f, 0.0f});
        ImGui::End();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();
    }

    // draw debug panels/controls
    {
        ImGui::Begin("controls");
        ImGui::Text("FPS = %.2f", ImGui::GetIO().Framerate);
        ImGui::Text("decoration generation (us) = %.1f", s.timerMeshgen.micros());
        ImGui::Text("instance batching sort (us) = %.1f", s.timerSort.micros());
        ImGui::Text("scene-level BVHed hittest (us) = %.1f", s.timerSceneHittest.micros());
        ImGui::Text("mesh-level triangle hittest (us) = %.1f", s.timerTriangleHittest.micros());
        ImGui::Text("instanced render call (us) = %.1f", s.timerRender.micros());
        ImGui::Text("texture blit (us) = %.1f", s.timerBlit.micros());
        ImGui::Text("e2e (us) = %.1f", s.timerE2E.micros());
        ImGui::Checkbox("generate decorations each frame", &s.generateDecorationsOnEachFrame);
        ImGui::Checkbox("optimize draw order", &s.optimizeDrawOrder);
        ImGui::Checkbox("draw scene", &s.drawScene);
        ImGui::Checkbox("draw rims", &s.drawRims);
        ImGui::Checkbox("draw AABBs", &s.drawAABBs);
        ImGui::Checkbox("do hittest", &s.doSceneHittest);
        ImGui::End();
    }

    osc::ImGuiRender();
}
