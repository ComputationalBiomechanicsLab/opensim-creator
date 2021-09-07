#include "SceneGeneratorNewScreen.hpp"

#include "src/3D/Shaders/GouraudMrtShader.hpp"
#include "src/3D/Gl.hpp"
#include "src/3D/GlGlm.hpp"
#include "src/3D/Model.hpp"
#include "src/3D/SceneMesh.hpp"
#include "src/OpenSimBindings/UiTypes.hpp"
#include "src/SimTKBindings/SceneGeneratorNew.hpp"
#include "src/Utils/ImGuiHelpers.hpp"
#include "src/App.hpp"
#include "src/Assertions.hpp"

#include <imgui.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <algorithm>
#include <memory>
#include <vector>

using namespace osc;

static void getSceneElements(
        std::shared_ptr<ThreadsafeMeshCache> meshCache,
        OpenSim::Model const& m,
        SimTK::State const& st,
        std::vector<SceneElement>& rv) {
    rv.clear();

    auto onEmit = [&](SceneElement const& se) {
        rv.push_back(se);
    };

    SceneGeneratorLambda visitor{meshCache, m.getSystem().getMatterSubsystem(), st, 1.0f, onEmit};

    OpenSim::ModelDisplayHints mdh = m.getDisplayHints();

    SimTK::Array_<SimTK::DecorativeGeometry> geomList;
    for (OpenSim::Component const& c : m.getComponentList()) {
        c.generateDecorations(true, mdh, st, geomList);
        for (SimTK::DecorativeGeometry const& dg : geomList) {
            dg.implementGeometry(visitor);
        }
        geomList.clear();

        c.generateDecorations(false, mdh, st, geomList);
        for (SimTK::DecorativeGeometry const& dg : geomList) {
            dg.implementGeometry(visitor);
        }
        geomList.clear();
    }
}

namespace {
    struct SceneGPUElementData final {
        glm::vec3 pos;
        glm::vec3 norm;
    };

    gl::ArrayBuffer<SceneGPUElementData> uploadMeshToGPU(Mesh const& m) {
        OSC_ASSERT_ALWAYS(m.verts.size() == m.normals.size());

        std::vector<SceneGPUElementData> buf;
        buf.reserve(m.verts.size());

        for (size_t i = 0; i < m.verts.size(); ++i) {
            buf.push_back({m.verts[i], m.normals[i]});
        }

        return {buf};
    }

    gl::VertexArray makeVAO(gl::ArrayBuffer<SceneGPUElementData>& vbo,
                            gl::ElementArrayBuffer<GLushort>& ebo) {

        gl::VertexArray rv;
        gl::BindVertexArray(rv);
        gl::BindBuffer(vbo);
        gl::BindBuffer(ebo);
        gl::VertexAttribPointer(gl::AttributeVec3{0}, false, sizeof(SceneGPUElementData), offsetof(SceneGPUElementData, pos));
        gl::EnableVertexAttribArray(gl::AttributeVec3{0});
        gl::VertexAttribPointer(gl::AttributeVec3{2}, false, sizeof(SceneGPUElementData), offsetof(SceneGPUElementData, norm));
        gl::EnableVertexAttribArray(gl::AttributeVec3{2});
        return rv;
    }

    struct SceneGPUMesh final {
        gl::ArrayBuffer<SceneGPUElementData> data;
        gl::ElementArrayBuffer<GLushort> indices;
        gl::VertexArray vao;

        SceneGPUMesh(Mesh const& m) :
            data{uploadMeshToGPU(m)},
            indices{m.indices},
            vao{makeVAO(data, indices)} {
        }
    };

    struct SceneGPUInstanceData final {
        glm::mat4x3 modelMtx;
        glm::mat3 normalMtx;
        glm::vec4 rgba;
        float rimIntensity;
    };

    gl::ArrayBuffer<SceneGPUInstanceData> uploadInstances(SceneElement const* first, size_t n) {
        std::vector<SceneGPUInstanceData> buf;
        buf.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            SceneElement const& se = first[i];
            buf.push_back({se.modelMtx, se.normalMtx, se.color, 0.0f});
        }
        return {buf};
    }
}

struct osc::SceneGeneratorNewScreen::Impl final {
    std::shared_ptr<ThreadsafeMeshCache> meshCache = std::make_shared<ThreadsafeMeshCache>();
    std::unordered_map<SceneMesh::IdType, std::unique_ptr<SceneGPUMesh>> gpuCache;

    std::string modelPath = App::resource("models/RajagopalModel/Rajagopal2015.osim").string();

    UiModel uim{modelPath};

    PolarPerspectiveCamera camera;
    glm::vec3 lightDir = {-0.34f, -0.25f, 0.05f};
    glm::vec3 lightCol = {248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
    glm::vec4 backgroundCol = {0.89f, 0.89f, 0.89f, 1.0f};
    glm::vec4 rimCol = {1.0f, 0.4f, 0.0f, 0.85f};

    GouraudMrtShader shader;

    Impl() {
    }

    SceneGPUMesh& getGPUMeshCached(SceneMesh const& se) {
        auto [it, inserted] = gpuCache.try_emplace(se.getID(), nullptr);
        if (inserted) {
            it->second = std::make_unique<SceneGPUMesh>(se.getMesh());
        }
        return *it->second;
    }
};

// public API

osc::SceneGeneratorNewScreen::SceneGeneratorNewScreen() :
    m_Impl{new Impl{}} {
}

osc::SceneGeneratorNewScreen::~SceneGeneratorNewScreen() noexcept = default;

void osc::SceneGeneratorNewScreen::onMount() {
    // called when app receives the screen, but before it starts pumping events
    // into it, ticking it, drawing it, etc.

    App::cur().disableVsync();
    osc::ImGuiInit();  // boot up ImGui support
}

void osc::SceneGeneratorNewScreen::onUnmount() {
    // called when the app is going to stop pumping events/ticks/draws into this
    // screen (e.g. because the app is quitting, or transitioning to some other screen)

    osc::ImGuiShutdown();  // shutdown ImGui support
}

void osc::SceneGeneratorNewScreen::onEvent(SDL_Event const& e) {
    // called when the app receives an event from the operating system

    // pump event into ImGui, if using it:
    if (osc::ImGuiOnEvent(e)) {
        return;  // ImGui handled this particular event
    }
}

void osc::SceneGeneratorNewScreen::tick(float) {
    UpdatePolarCameraFromImGuiUserInput(App::cur().dims(), m_Impl->camera);
}

void osc::SceneGeneratorNewScreen::draw() {
    osc::ImGuiNewFrame();  // tell ImGui you're about to start drawing a new frame

    gl::ClearColor(1.0f, 1.0f, 1.0f, 0.0f);  // set app window bg color
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // clear app window with bg color

    GouraudMrtShader& shader = m_Impl->shader;
    gl::UseProgram(shader.program);
    gl::Uniform(shader.uProjMat, m_Impl->camera.getProjMtx(App::cur().aspectRatio()));
    gl::Uniform(shader.uViewMat, m_Impl->camera.getViewMtx());
    gl::Uniform(shader.uLightDir, m_Impl->lightDir);
    gl::Uniform(shader.uLightColor, m_Impl->lightCol);
    gl::Uniform(shader.uViewPos, m_Impl->camera.getPos());

    // upload all instances to the GPU;
    gl::ArrayBuffer instanceBuf = uploadInstances(m_Impl->uim.decorations.data(), m_Impl->uim.decorations.size());

    size_t pos = 0;
    size_t ninstances = m_Impl->uim.decorations.size();
    auto const& els = m_Impl->uim.decorations;

    while (pos < ninstances) {
        SceneElement const& se = els[pos];

        // batch
        size_t end = pos + 1;
        while (end < ninstances && els[end].mesh.get() == se.mesh.get()) {
            ++end;
        }

        // lookup/populate GPU data for mesh
        SceneGPUMesh const& gpuMesh = m_Impl->getGPUMeshCached(*se.mesh);

        gl::BindVertexArray(gpuMesh.vao);
        gl::VertexAttribPointer(GouraudMrtShader::aModelMat, false, sizeof(SceneGPUInstanceData), sizeof(SceneGPUInstanceData)*pos + offsetof(SceneGPUInstanceData, modelMtx));
        gl::VertexAttribDivisor(GouraudMrtShader::aModelMat, 1);
        gl::EnableVertexAttribArray(GouraudMrtShader::aModelMat);
        gl::VertexAttribPointer(GouraudMrtShader::aNormalMat, false, sizeof(SceneGPUInstanceData), sizeof(SceneGPUInstanceData)*pos + offsetof(SceneGPUInstanceData, normalMtx));
        gl::VertexAttribDivisor(GouraudMrtShader::aNormalMat, 1);
        gl::EnableVertexAttribArray(GouraudMrtShader::aNormalMat);
        gl::VertexAttribPointer(GouraudMrtShader::aDiffuseColor, false, sizeof(SceneGPUInstanceData), sizeof(SceneGPUInstanceData)*pos + offsetof(SceneGPUInstanceData, rgba));
        gl::VertexAttribDivisor(GouraudMrtShader::aDiffuseColor, 1);
        gl::EnableVertexAttribArray(GouraudMrtShader::aDiffuseColor);
        gl::VertexAttribPointer(GouraudMrtShader::aRimIntensity, false, sizeof(SceneGPUInstanceData), sizeof(SceneGPUInstanceData)*pos + offsetof(SceneGPUInstanceData, rimIntensity));
        gl::VertexAttribDivisor(GouraudMrtShader::aRimIntensity, 1);
        gl::EnableVertexAttribArray(GouraudMrtShader::aRimIntensity);
        glDrawElementsInstanced(GL_TRIANGLES, gpuMesh.indices.sizei(), gl::indexType(gpuMesh.indices), nullptr, static_cast<GLsizei>(end-pos));
        gl::BindVertexArray();

        pos = end;
    }

    ImGui::Begin("cookiecutter panel");
    ImGui::Text("%.2f", ImGui::GetIO().Framerate);

    osc::ImGuiRender();  // tell ImGui to render any ImGui widgets since calling ImGuiNewFrame();
}
