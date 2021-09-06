#include "Component3DViewer.hpp"

#include "src/App.hpp"
#include "src/3D/Constants.hpp"
#include "src/3D/Gl.hpp"
#include "src/3D/GlGlm.hpp"
#include "src/3D/InstancedRenderer.hpp"
#include "src/3D/Model.hpp"
#include "src/3D/Texturing.hpp"
#include "src/3D/Shaders/SolidColorShader.hpp"
#include "src/OpenSimBindings/SceneGenerator.hpp"
#include "src/Utils/ImGuiHelpers.hpp"
#include "src/Utils/ScopeGuard.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <vector>
#include <limits>

using namespace osc;

static CPUMesh generateFloorMesh() {
    CPUMesh rv;
    rv.data = GenTexturedQuad();
    for (auto& coord : rv.data.texcoords) {
        coord *= 200.0f;
    }
    rv.aabb = AABBFromVerts(rv.data.verts.data(), rv.data.verts.size());
    BVH_BuildFromTriangles(rv.triangleBVH, rv.data.verts.data(), rv.data.verts.size());
    return rv;
}

static gl::VertexArray makeVAO(SolidColorShader& shader, gl::ArrayBuffer<glm::vec3>& vbo) {
    gl::VertexArray rv;
    gl::BindVertexArray(rv);
    gl::BindBuffer(vbo);
    gl::VertexAttribPointer(shader.aPos, false, sizeof(glm::vec3), 0);
    gl::EnableVertexAttribArray(shader.aPos);
    return rv;
}

struct osc::Component3DViewer::Impl final {
    Component3DViewerFlags flags;

    SceneGenerator sg;
    SceneDecorations decorations;

    InstancedRenderer renderer;
    InstancedDrawlist drawlist;
    InstancedRendererParams rendererParams;

    PolarPerspectiveCamera camera;

    // floor data
    std::shared_ptr<CPUMesh> floorMesh = std::make_shared<CPUMesh>(generateFloorMesh());
    std::shared_ptr<gl::Texture2D> chequerTex = std::make_shared<gl::Texture2D>(genChequeredFloorTexture());
    InstanceableMeshdata floorMeshdata = uploadMeshdataForInstancing(floorMesh->data);

    // plain shader for drawing flat overlay elements
    SolidColorShader solidColorShader;

    // grid data
    gl::ArrayBuffer<glm::vec3> gridVBO{GenNbyNGrid(100).verts};
    gl::VertexArray gridVAO = makeVAO(solidColorShader, gridVBO);

    // line data
    gl::ArrayBuffer<glm::vec3> lineVBO{GenYLine().verts};
    gl::VertexArray lineVAO = makeVAO(solidColorShader, lineVBO);

    // aabb data
    gl::ArrayBuffer<glm::vec3> cubewireVBO{GenCubeLines().verts};
    gl::VertexArray cubewireVAO = makeVAO(solidColorShader, cubewireVBO);

    std::vector<unsigned char> rims;
    std::vector<std::shared_ptr<gl::Texture2D>> textures;
    std::vector<BVHCollision> sceneHittestResults;
    std::vector<BVHCollision> triangleHittestResults;

    glm::vec2 renderDims = {0.0f, 0.0f};
    bool renderHovered = false;
    bool renderLeftClicked = false;
    bool renderRightClicked = false;

    // scale factor for all non-mesh, non-overlay scene elements (e.g.
    // the floor, bodies)
    //
    // this is necessary because some meshes can be extremely small/large and
    // scene elements need to be scaled accordingly (e.g. without this, a body
    // sphere end up being much larger than a mesh instance). Imagine if the
    // mesh was the leg of a fly, in meters.
    float fixupScaleFactor = 1.0f;

    Impl(Component3DViewerFlags flags_) : flags{flags_} {
    }
};

static void drawOptionsMenu(osc::Component3DViewer::Impl& impl) {

    ImGui::CheckboxFlags(
        "draw dynamic geometry", &impl.flags, Component3DViewerFlags_DrawDynamicDecorations);

    ImGui::CheckboxFlags(
        "draw static geometry", &impl.flags, Component3DViewerFlags_DrawStaticDecorations);

    ImGui::CheckboxFlags("draw frames", &impl.flags, Component3DViewerFlags_DrawFrames);

    ImGui::CheckboxFlags("draw debug geometry", &impl.flags, Component3DViewerFlags_DrawDebugGeometry);
    ImGui::CheckboxFlags("draw labels", &impl.flags, Component3DViewerFlags_DrawLabels);

    ImGui::Separator();

    ImGui::Text("Graphical Options:");

    ImGui::CheckboxFlags("wireframe mode", &impl.rendererParams.flags, InstancedRendererFlags_WireframeMode);
    ImGui::CheckboxFlags("show normals", &impl.rendererParams.flags, InstancedRendererFlags_ShowMeshNormals);
    ImGui::CheckboxFlags("draw rims", &impl.rendererParams.flags, InstancedRendererFlags_DrawRims);
    ImGui::CheckboxFlags("draw scene geometry", &impl.rendererParams.flags, InstancedRendererFlags_DrawSceneGeometry);
    ImGui::CheckboxFlags("show XZ grid", &impl.flags, Component3DViewerFlags_DrawXZGrid);
    ImGui::CheckboxFlags("show XY grid", &impl.flags, Component3DViewerFlags_DrawXYGrid);
    ImGui::CheckboxFlags("show YZ grid", &impl.flags, Component3DViewerFlags_DrawYZGrid);
    ImGui::CheckboxFlags("show alignment axes", &impl.flags, Component3DViewerFlags_DrawAlignmentAxes);
    ImGui::CheckboxFlags("show grid lines", &impl.flags, Component3DViewerFlags_DrawAxisLines);
    ImGui::CheckboxFlags("show AABBs", &impl.flags, Component3DViewerFlags_DrawAABBs);
    ImGui::CheckboxFlags("show BVH", &impl.flags, Component3DViewerFlags_DrawBVH);
}

static void drawSceneMenu(osc::Component3DViewer::Impl& impl) {
    if (ImGui::Button("Top")) {
        impl.camera.theta = 0.0f;
        impl.camera.phi = fpi2;
    }

    if (ImGui::Button("Left")) {
        // assumes models tend to point upwards in Y and forwards in +X
        // (so sidewards is theta == 0 or PI)
        impl.camera.theta = fpi;
        impl.camera.phi = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Right")) {
        // assumes models tend to point upwards in Y and forwards in +X
        // (so sidewards is theta == 0 or PI)
        impl.camera.theta = 0.0f;
        impl.camera.phi = 0.0f;
    }

    if (ImGui::Button("Bottom")) {
        impl.camera.theta = 0.0f;
        impl.camera.phi = 3.0f * fpi2;
    }

    ImGui::NewLine();

    ImGui::SliderFloat("radius", &impl.camera.radius, 0.0f, 10.0f);
    ImGui::SliderFloat("theta", &impl.camera.theta, 0.0f, 2.0f * fpi);
    ImGui::SliderFloat("phi", &impl.camera.phi, 0.0f, 2.0f * fpi);
    ImGui::InputFloat("fov", &impl.camera.fov);
    ImGui::InputFloat("znear", &impl.camera.znear);
    ImGui::InputFloat("zfar", &impl.camera.zfar);
    ImGui::NewLine();
    ImGui::SliderFloat("pan_x", &impl.camera.focusPoint.x, -100.0f, 100.0f);
    ImGui::SliderFloat("pan_y", &impl.camera.focusPoint.y, -100.0f, 100.0f);
    ImGui::SliderFloat("pan_z", &impl.camera.focusPoint.z, -100.0f, 100.0f);

    ImGui::Separator();

    ImGui::SliderFloat("light_dir_x", &impl.rendererParams.lightDir.x, -1.0f, 1.0f);
    ImGui::SliderFloat("light_dir_y", &impl.rendererParams.lightDir.y, -1.0f, 1.0f);
    ImGui::SliderFloat("light_dir_z", &impl.rendererParams.lightDir.z, -1.0f, 1.0f);
    ImGui::ColorEdit3("light_color", reinterpret_cast<float*>(&impl.rendererParams.lightCol));
    ImGui::ColorEdit3("background color", reinterpret_cast<float*>(&impl.rendererParams.backgroundCol));

    ImGui::Separator();

    ImGui::InputFloat("fixup scale factor", &impl.fixupScaleFactor);
}

static void drawMainMenuContents(osc::Component3DViewer::Impl& impl) {
    if (ImGui::BeginMenu("Options")) {
        drawOptionsMenu(impl);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Scene")) {
        drawSceneMenu(impl);
        ImGui::EndMenu();
    }
}

static OpenSim::Component const* hittestSceneDecorations(osc::Component3DViewer::Impl& impl) {
    if (!impl.renderHovered) {
        return nullptr;
    }

    // figure out mouse pos in panel's NDC system
    glm::vec2 windowScreenPos = ImGui::GetWindowPos();  // where current ImGui window is in the screen
    glm::vec2 mouseScreenPos = ImGui::GetMousePos();  // where mouse is in the screen
    glm::vec2 mouseWindowPos = mouseScreenPos - windowScreenPos;  // where mouse is in current window
    glm::vec2 cursorWindowPos = ImGui::GetCursorPos();  // where cursor is in current window
    glm::vec2 mouseItemPos = mouseWindowPos - cursorWindowPos;  // where mouse is in current item
    glm::vec2 itemDims = ImGui::GetContentRegionAvail();  // how big current window will be

    // un-project the mouse position as a ray in worldspace
    Line cameraRay = impl.camera.unprojectScreenposToWorldRay(mouseItemPos, itemDims);

    // use scene BVH to intersect that ray with the scene
    impl.sceneHittestResults.clear();
    BVH_GetRayAABBCollisions(impl.decorations.sceneBVH, cameraRay, &impl.sceneHittestResults);

    // go through triangle BVHes to figure out which, if any, triangle is closest intersecting
    int closestIdx = -1;
    float closestDistance = std::numeric_limits<float>::max();

    // iterate through each scene-level hit and perform a triangle-level hittest
    for (BVHCollision const& c : impl.sceneHittestResults) {
        int instanceIdx = c.primId;
        glm::mat4 const& instanceMmtx = impl.decorations.modelMtxs[instanceIdx];
        CPUMesh const& instanceMesh = *impl.decorations.cpuMeshes[instanceIdx];

        Line cameraRayModelspace = LineApplyXform(cameraRay, glm::inverse(instanceMmtx));

        // perform ray-triangle BVH hittest
        impl.triangleHittestResults.clear();
        BVH_GetRayTriangleCollisions(
                    instanceMesh.triangleBVH,
                    instanceMesh.data.verts.data(),
                    instanceMesh.data.verts.size(),
                    cameraRayModelspace,
                    &impl.triangleHittestResults);

        // check each triangle collision and take the closest
        for (BVHCollision const& tc : impl.triangleHittestResults) {
            if (tc.distance < closestDistance) {
                closestIdx = instanceIdx;
                closestDistance = tc.distance;
            }
        }
    }

    return closestIdx >= 0 ? impl.decorations.components[closestIdx] : nullptr;
}

static void drawXZGrid(osc::Component3DViewer::Impl& impl) {
    SolidColorShader& shader = impl.solidColorShader;

    gl::UseProgram(shader.program);
    gl::Uniform(shader.uModel, glm::scale(glm::rotate(glm::mat4{1.0f}, fpi2, {1.0f, 0.0f, 0.0f}), {5.0f, 5.0f, 1.0f}));
    gl::Uniform(shader.uColor, {0.7f, 0.7f, 0.7f, 0.15f});
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(impl.renderDims.x / impl.renderDims.y));
    gl::Uniform(shader.uView, impl.camera.getViewMtx());
    gl::BindVertexArray(impl.gridVAO);
    gl::DrawArrays(GL_LINES, 0, impl.gridVBO.sizei());
    gl::BindVertexArray();
}

static void drawXYGrid(osc::Component3DViewer::Impl& impl) {
    SolidColorShader& shader = impl.solidColorShader;

    gl::UseProgram(shader.program);
    gl::Uniform(shader.uModel, glm::scale(glm::mat4{1.0f}, {5.0f, 5.0f, 1.0f}));
    gl::Uniform(shader.uColor, {0.7f, 0.7f, 0.7f, 0.15f});
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(impl.renderDims.x / impl.renderDims.y));
    gl::Uniform(shader.uView, impl.camera.getViewMtx());
    gl::BindVertexArray(impl.gridVAO);
    gl::DrawArrays(GL_LINES, 0, impl.gridVBO.sizei());
    gl::BindVertexArray();
}

static void drawYZGrid(osc::Component3DViewer::Impl& impl) {
    SolidColorShader& shader = impl.solidColorShader;

    gl::UseProgram(shader.program);
    gl::Uniform(shader.uModel, glm::scale(glm::rotate(glm::mat4{1.0f}, fpi2, {0.0f, 1.0f, 0.0f}), {5.0f, 5.0f, 1.0f}));
    gl::Uniform(shader.uColor, {0.7f, 0.7f, 0.7f, 0.15f});
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(impl.renderDims.x / impl.renderDims.y));
    gl::Uniform(shader.uView, impl.camera.getViewMtx());
    gl::BindVertexArray(impl.gridVAO);
    gl::DrawArrays(GL_LINES, 0, impl.gridVBO.sizei());
    gl::BindVertexArray();
}

static void drawAlignmentAxes(osc::Component3DViewer::Impl& impl) {
    glm::mat4 model2view = impl.camera.getViewMtx();

    // we only care about rotation of the axes, not translation
    model2view[3] = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f};

    // rescale + translate the y-line vertices
    glm::mat4 makeLineOneSided = glm::translate(glm::identity<glm::mat4>(), glm::vec3{0.0f, 1.0f, 0.0f});
    glm::mat4 scaler = glm::scale(glm::identity<glm::mat4>(), glm::vec3{0.025f});
    glm::mat4 translator = glm::translate(glm::identity<glm::mat4>(), glm::vec3{-0.95f, -0.95f, 0.0f});
    glm::mat4 baseModelMtx = translator * scaler * model2view;

    SolidColorShader& shader = impl.solidColorShader;

    // common shader stuff
    gl::UseProgram(shader.program);
    gl::Uniform(shader.uProjection, gl::identity);
    gl::Uniform(shader.uView, gl::identity);
    gl::BindVertexArray(impl.lineVAO);
    gl::Disable(GL_DEPTH_TEST);

    // y axis
    {
        gl::Uniform(shader.uColor, {0.0f, 1.0f, 0.0f, 1.0f});
        gl::Uniform(shader.uModel, baseModelMtx * makeLineOneSided);
        gl::DrawArrays(GL_LINES, 0, impl.gridVBO.sizei());
    }

    // x axis
    {
        glm::mat4 rotateYtoX =
            glm::rotate(glm::identity<glm::mat4>(), fpi2, glm::vec3{0.0f, 0.0f, -1.0f});

        gl::Uniform(shader.uColor, {1.0f, 0.0f, 0.0f, 1.0f});
        gl::Uniform(shader.uModel, baseModelMtx * rotateYtoX * makeLineOneSided);
        gl::DrawArrays(GL_LINES, 0, impl.gridVBO.sizei());
    }

    // z axis
    {
        glm::mat4 rotateYtoZ =
            glm::rotate(glm::identity<glm::mat4>(), fpi2, glm::vec3{1.0f, 0.0f, 0.0f});

        gl::Uniform(shader.uColor, {0.0f, 0.0f, 1.0f, 1.0f});
        gl::Uniform(shader.uModel, baseModelMtx * rotateYtoZ * makeLineOneSided);
        gl::DrawArrays(GL_LINES, 0, impl.gridVBO.sizei());
    }

    gl::BindVertexArray();
    gl::Enable(GL_DEPTH_TEST);
}

static void drawFloorAxesLines(osc::Component3DViewer::Impl& impl) {
    SolidColorShader& shader = impl.solidColorShader;

    // common stuff
    gl::UseProgram(shader.program);
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(impl.renderDims.x / impl.renderDims.y));
    gl::Uniform(shader.uView, impl.camera.getViewMtx());
    gl::BindVertexArray(impl.lineVAO);

    // X
    gl::Uniform(shader.uModel, glm::rotate(glm::mat4{1.0f}, fpi2, {0.0f, 0.0f, 1.0f}));
    gl::Uniform(shader.uColor, {1.0f, 0.0f, 0.0f, 1.0f});
    gl::DrawArrays(GL_LINES, 0, impl.gridVBO.sizei());

    // Z
    gl::Uniform(shader.uModel, glm::rotate(glm::mat4{1.0f}, fpi2, {1.0f, 0.0f, 0.0f}));
    gl::Uniform(shader.uColor, {0.0f, 0.0f, 1.0f, 1.0f});
    gl::DrawArrays(GL_LINES, 0, impl.gridVBO.sizei());

    gl::BindVertexArray();
}

static void drawAABBs(osc::Component3DViewer::Impl& impl) {
    SolidColorShader& shader = impl.solidColorShader;

    // common stuff
    gl::UseProgram(shader.program);
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(impl.renderDims.x / impl.renderDims.y));
    gl::Uniform(shader.uView, impl.camera.getViewMtx());
    gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});

    gl::BindVertexArray(impl.cubewireVAO);
    for (AABB const& aabb : impl.decorations.aabbs) {
        glm::vec3 halfWidths = AABBDims(aabb) / 2.0f;
        glm::vec3 center = AABBCenter(aabb);

        glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, halfWidths);
        glm::mat4 mover = glm::translate(glm::mat4{1.0f}, center);
        glm::mat4 mmtx = mover * scaler;

        gl::Uniform(shader.uModel, mmtx);
        gl::DrawArrays(GL_LINES, 0, impl.cubewireVBO.sizei());
    }
    gl::BindVertexArray();
}

// assumes `pos` is in-bounds
static void drawBVHRecursive(osc::Component3DViewer::Impl& impl, int pos) {
    BVHNode const& n = impl.decorations.sceneBVH.nodes[pos];

    glm::vec3 halfWidths = AABBDims(n.bounds) / 2.0f;
    glm::vec3 center = AABBCenter(n.bounds);

    glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, halfWidths);
    glm::mat4 mover = glm::translate(glm::mat4{1.0f}, center);
    glm::mat4 mmtx = mover * scaler;
    gl::Uniform(impl.solidColorShader.uModel, mmtx);
    gl::DrawArrays(GL_LINES, 0, impl.cubewireVBO.sizei());

    if (n.nlhs >= 0) {  // if it's an internal node
        drawBVHRecursive(impl, pos+1);
        drawBVHRecursive(impl, pos+n.nlhs+1);
    }
}

static void drawBVH(osc::Component3DViewer::Impl& impl) {
    if (impl.decorations.sceneBVH.nodes.empty()) {
        return;
    }

    SolidColorShader& shader = impl.solidColorShader;

    // common stuff
    gl::UseProgram(shader.program);
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(impl.renderDims.x / impl.renderDims.y));
    gl::Uniform(shader.uView, impl.camera.getViewMtx());
    gl::Uniform(shader.uColor, {0.0f, 0.0f, 0.0f, 1.0f});

    gl::BindVertexArray(impl.cubewireVAO);
    drawBVHRecursive(impl, 0);
    gl::BindVertexArray();
}

static void drawOverlays(osc::Component3DViewer::Impl& impl) {
    gl::BindFramebuffer(GL_FRAMEBUFFER, impl.renderer.getOutputFbo());

    if (impl.flags & Component3DViewerFlags_DrawXZGrid) {
        drawXZGrid(impl);
    }

    if (impl.flags & Component3DViewerFlags_DrawXYGrid) {
        drawXYGrid(impl);
    }

    if (impl.flags & Component3DViewerFlags_DrawYZGrid) {
        drawYZGrid(impl);
    }

    if (impl.flags & Component3DViewerFlags_DrawAlignmentAxes) {
        drawAlignmentAxes(impl);
    }

    if (impl.flags & Component3DViewerFlags_DrawAxisLines) {
        drawFloorAxesLines(impl);
    }

    if (impl.flags & Component3DViewerFlags_DrawAABBs) {
        drawAABBs(impl);
    }

    if (impl.flags & Component3DViewerFlags_DrawBVH) {
        drawBVH(impl);
    }

    gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
}

// public API

osc::Component3DViewer::Component3DViewer(Component3DViewerFlags flags) :
    m_Impl{new Impl{flags}} {
}

osc::Component3DViewer::~Component3DViewer() noexcept = default;

bool osc::Component3DViewer::isMousedOver() const noexcept {
    return m_Impl->renderHovered;
}

Component3DViewerResponse osc::Component3DViewer::draw(
        char const* panelName,
        OpenSim::Component const& c,
        OpenSim::ModelDisplayHints const& mdh,
        SimTK::State const& st,
        OpenSim::Component const* currentSelection,
        OpenSim::Component const* currentHover) {

    Impl& impl = *m_Impl;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
    OSC_SCOPE_GUARD({ ImGui::PopStyleVar(); });

    // try to start drawing the main panel, but return early if it's closed
    // to prevent the UI from having to do redundant work
    bool opened = ImGui::Begin(panelName, nullptr, ImGuiWindowFlags_MenuBar);
    OSC_SCOPE_GUARD({ ImGui::End(); });

    if (!opened) {
        return {};  // main panel is closed, so skip the rest of the rendering etc.
    }

    if (impl.renderHovered) {
        UpdatePolarCameraFromImGuiUserInput(App::cur().dims(), impl.camera);
    }

    // draw panel menu
    if (ImGui::BeginMenuBar()) {
        drawMainMenuContents(impl);
        ImGui::EndMenuBar();
    }

    // put 3D scene in an undraggable child panel, to prevent accidental panel
    // dragging when the user drags their mouse over the scene
    if (!ImGui::BeginChild("##child", {0.0f, 0.0f}, false, ImGuiWindowFlags_NoMove)) {
        return {};  // child not visible
    }
    OSC_SCOPE_GUARD({ ImGui::EndChild(); });

    // generate scene decorations from the model + state
    {
        SceneGeneratorFlags flags = SceneGeneratorFlags_Default;
        OpenSim::ModelDisplayHints mdhcpy{mdh};

        // map component flags onto renderer/generator flags

        if (impl.flags & Component3DViewerFlags_DrawDebugGeometry) {
            mdhcpy.set_show_debug_geometry(true);
        } else {
            mdhcpy.set_show_debug_geometry(false);
        }

        if (impl.flags & Component3DViewerFlags_DrawDynamicDecorations) {
            flags |= SceneGeneratorFlags_GenerateDynamicDecorations;
        } else {
            flags &= ~SceneGeneratorFlags_GenerateDynamicDecorations;
        }

        if (impl.flags & Component3DViewerFlags_DrawFrames) {
            mdhcpy.set_show_frames(true);
        } else {
            mdhcpy.set_show_frames(false);
        }

        if (impl.flags & Component3DViewerFlags_DrawLabels) {
            mdhcpy.set_show_labels(true);
        } else {
            mdhcpy.set_show_labels(false);
        }

        if (impl.flags & Component3DViewerFlags_DrawStaticDecorations) {
            flags |= SceneGeneratorFlags_GenerateStaticDecorations;
        } else {
            flags &= ~SceneGeneratorFlags_GenerateStaticDecorations;
        }

        impl.sg.generate(
            c,
            st,
            mdhcpy,
            flags,
            impl.fixupScaleFactor,
            impl.decorations);
    }

    // append the floor to the decorations list (the floor is "in" the scene, rather than an overlay)
    {
        SceneDecorations& decs = impl.decorations;

        glm::mat4x3& modelXform = decs.modelMtxs.emplace_back();
        modelXform = [&impl]() {
            // rotate from XY (+Z dir) to ZY (+Y dir)
            glm::mat4 rv = glm::rotate(glm::mat4{1.0f}, -fpi2, {1.0f, 0.0f, 0.0f});

            // make floor extend far in all directions
            rv = glm::scale(glm::mat4{1.0f}, {impl.fixupScaleFactor * 100.0f, 1.0f, impl.fixupScaleFactor * 100.0f}) * rv;

            // lower slightly, so that it doesn't conflict with OpenSim model planes
            // that happen to lie at Z==0
            rv = glm::translate(glm::mat4{1.0f}, {0.0f, -0.0001f, 0.0f}) * rv;

            return glm::mat4x3{rv};
        }();

        decs.normalMtxs.push_back(NormalMatrix(modelXform));
        decs.cols.push_back(Rgba32{0x00, 0x00, 0x00, 0x00});
        decs.gpuMeshes.push_back(impl.floorMeshdata);
        decs.cpuMeshes.push_back(impl.floorMesh);
        decs.aabbs.push_back(AABBApplyXform(impl.floorMesh->aabb, modelXform));
        decs.components.push_back(nullptr);

        impl.textures.clear();
        impl.textures.resize(decs.modelMtxs.size(), nullptr);
        impl.textures.back() = impl.chequerTex;
    }

    // make rim highlights reflect selection/hover state
    impl.rims.clear();
    impl.rims.resize(impl.decorations.modelMtxs.size(), 0x00);
    for (size_t i = 0; i < impl.decorations.modelMtxs.size(); ++i) {
        OpenSim::Component const* assocComponent = impl.decorations.components[i];

        if (!assocComponent) {
            continue;  // no association to rim highlight
        }

        while (assocComponent) {
            if (assocComponent == currentSelection) {
                impl.rims[i] = 0xff;
                break;
            } else if (assocComponent == currentHover) {
                impl.rims[i] = 0x66;
                break;
            } else if (!assocComponent->hasOwner()) {
                break;
            } else {
                assocComponent = &assocComponent->getOwner();
            }
        }
    }

    // upload scene decorations to a drawlist
    {
        DrawlistCompilerInput dci;
        dci.ninstances = impl.decorations.modelMtxs.size();
        dci.modelMtxs = impl.decorations.modelMtxs.data();
        dci.normalMtxs = impl.decorations.normalMtxs.data();
        dci.colors = impl.decorations.cols.data();
        dci.meshes = impl.decorations.gpuMeshes.data();
        dci.textures = impl.textures.data();
        dci.rimIntensities = impl.rims.data();
        uploadInputsToDrawlist(dci, impl.drawlist);
    }

    // render scene with renderer
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    if (contentRegion.x >= 1.0f && contentRegion.y >= 1.0f) {

        // ensure renderer dims match the panel's dims
        glm::ivec2 dims{static_cast<int>(contentRegion.x), static_cast<int>(contentRegion.y)};
        impl.renderer.setDims(dims);
        impl.renderer.setMsxaaSamples(App::cur().getSamples());

        // render scene to texture (the texture is used later, by ImGui::Image)
        impl.rendererParams.projMtx = impl.camera.getProjMtx(contentRegion.x/contentRegion.y);
        impl.rendererParams.viewMtx = impl.camera.getViewMtx();
        impl.rendererParams.viewPos = impl.camera.getPos();

        impl.renderer.render(impl.rendererParams, impl.drawlist);
    }

    // render overlay elements (grid axes, lines, etc.)
    drawOverlays(impl);

    // perform hittest (AABB raycast, triangle raycast, BVH accelerated)
    OpenSim::Component const* hittestResult = hittestSceneDecorations(impl);

    // blit scene render (texture) to screen with ImGui::Image
    {
        GLint texGLHandle = impl.renderer.getOutputTexture().get();
        void* texImGuiHandle = reinterpret_cast<void*>(static_cast<uintptr_t>(texGLHandle));

        ImVec2 imgDims = ImGui::GetContentRegionAvail();
        ImVec2 texcoordUV0 = {0.0f, 1.0f};
        ImVec2 texcoordUV1 = {1.0f, 0.0f};

        ImGui::Image(texImGuiHandle, imgDims, texcoordUV0, texcoordUV1);

        impl.renderDims = ImGui::GetItemRectSize();
        impl.renderHovered = ImGui::IsItemHovered();
        impl.renderLeftClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
        impl.renderRightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
    }

    Component3DViewerResponse resp;
    resp.hovertestResult = hittestResult;
    resp.isMousedOver = impl.renderHovered;
    resp.isLeftClicked = impl.renderLeftClicked;
    resp.isRightClicked = impl.renderRightClicked;
    return resp;
}

Component3DViewerResponse osc::Component3DViewer::draw(
        char const* panelName,
        OpenSim::Model const& model,
        SimTK::State const& state,
        OpenSim::Component const* currentSelection,
        OpenSim::Component const* currentHover) {

    return draw(panelName,
                model,
                model.getDisplayHints(),
                state,
                currentSelection,
                currentHover);
}
