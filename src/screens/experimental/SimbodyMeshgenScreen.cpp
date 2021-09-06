#include "SimbodyMeshgenScreen.hpp"

#include "src/App.hpp"
#include "src/3d/Gl.hpp"
#include "src/3d/GlGlm.hpp"
#include "src/3d/Model.hpp"
#include "src/screens/experimental/ExperimentsScreen.hpp"
#include "src/simtk_bindings/SimTKLoadMesh.hpp"
#include "src/simtk_bindings/SimTKGeometryGenerator.hpp"
#include "src/utils/ImGuiHelpers.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <OpenSim/Simulation/Model/Model.h>

using namespace osc;

static char const g_VertexShader[] = R"(
    #version 330 core

    uniform mat4 uProjMat;
    uniform mat4 uViewMat;
    uniform mat4 uModelMat;

    layout (location = 0) in vec3 aPos;

    void main() {
        gl_Position = uProjMat * uViewMat * uModelMat * vec4(aPos, 1.0);
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
    struct Shader final {
        gl::Program prog = gl::CreateProgramFrom(
            gl::CompileFromSource<gl::VertexShader>(g_VertexShader),
            gl::CompileFromSource<gl::FragmentShader>(g_FragmentShader));

        gl::AttributeVec3 aPos{0};

        gl::UniformMat4 uModel = gl::GetUniformLocation(prog, "uModelMat");
        gl::UniformMat4 uView = gl::GetUniformLocation(prog, "uViewMat");
        gl::UniformMat4 uProjection = gl::GetUniformLocation(prog, "uProjMat");
        gl::UniformVec4 uColor = gl::GetUniformLocation(prog, "uColor");
    };


    gl::VertexArray makeVAO(Shader& shader, gl::ArrayBuffer<glm::vec3>& vbo, gl::ElementArrayBuffer<GLushort>& ebo) {
        gl::VertexArray rv;
        gl::BindVertexArray(rv);
        gl::BindBuffer(vbo);
        gl::BindBuffer(ebo);
        gl::VertexAttribPointer(shader.aPos, false, sizeof(glm::vec3), 0);
        gl::EnableVertexAttribArray(shader.aPos);
        gl::BindVertexArray();
        return rv;
    }

    struct LoadedGeom final {
        Mesh mesh;
        gl::ArrayBuffer<glm::vec3> vbo;
        gl::ElementArrayBuffer<GLushort> ebo;
        gl::VertexArray vao;

        LoadedGeom(Shader& s, Mesh mesh_) :
            mesh{std::move(mesh_)},
            vbo{mesh.verts},
            ebo{mesh.indices},
            vao{makeVAO(s, vbo, ebo)} {
        }
    };
}

namespace meshgen {
    using namespace osc;

    struct LoadedMeshfile final {
        LoadedGeom geom;
        glm::mat4 modelMtx;
        glm::vec4 rgba;

        LoadedMeshfile(Shader& s, SimbodyGeometry::MeshFile const& mf) :
            geom{s, SimTKLoadMesh(*mf.path)},
            modelMtx{mf.modelMtx},
            rgba{mf.rgba} {
        }
    };

    struct Scene_geom final {
        std::vector<SimbodyGeometry::Sphere> spheres;
        std::vector<SimbodyGeometry::Line> lines;
        std::vector<SimbodyGeometry::Cylinder> cylinders;
        std::vector<SimbodyGeometry::Brick> bricks;
        std::vector<SimbodyGeometry::Frame> frames;
        std::vector<LoadedMeshfile> meshes;
        std::vector<SimbodyGeometry::Ellipsoid> ellipsoid;
        std::vector<SimbodyGeometry::Cone> cones;
        std::vector<SimbodyGeometry::Arrow> arrows;
    };

    Scene_geom extractGeometry(Shader& shader, std::filesystem::path const& p) {
        Scene_geom rv;

        OpenSim::Model m{p.string()};
        m.finalizeFromProperties();
        m.finalizeConnections();
        SimTK::State& s = m.initSystem();

        auto onEmit = [&](SimbodyGeometry const& g) {
            switch (g.geometryType) {
            case SimbodyGeometry::Type::Sphere:
                rv.spheres.emplace_back(g.sphere);
                break;
            case SimbodyGeometry::Type::Line:
                rv.lines.emplace_back(g.line);
                break;
            case SimbodyGeometry::Type::Cylinder:
                rv.cylinders.emplace_back(g.cylinder);
                break;
            case SimbodyGeometry::Type::Brick:
                rv.bricks.emplace_back(g.brick);
                break;
            case SimbodyGeometry::Type::MeshFile:
                rv.meshes.emplace_back(shader, g.meshfile);
                break;
            case SimbodyGeometry::Type::Frame:
                rv.frames.emplace_back(g.frame);
                break;
            case SimbodyGeometry::Type::Ellipsoid:
                rv.ellipsoid.emplace_back(g.ellipsoid);
                break;
            case SimbodyGeometry::Type::Cone:
                rv.cones.emplace_back(g.cone);
                break;
            case SimbodyGeometry::Type::Arrow: {
                using osc::operator<<;
                rv.arrows.emplace_back(g.arrow);
                break; }
            default:
                std::cerr << g << std::endl;
                break;
            }
        };

        GeometryGeneratorLambda sgg{m.getMatterSubsystem(), s, onEmit};
        SimTK::Array_<SimTK::DecorativeGeometry> lst;
        auto hints = m.getDisplayHints();
        hints.set_show_frames(true);
        hints.set_show_debug_geometry(true);
        hints.set_show_labels(true);
        hints.set_show_wrap_geometry(true);
        hints.set_show_contact_geometry(true);
        hints.set_show_forces(true);
        hints.set_show_markers(true);

        for (OpenSim::Component const& c : m.getComponentList()) {
            c.generateDecorations(true, hints, s, lst);
            for (SimTK::DecorativeGeometry const& g : lst) {
                g.implementGeometry(sgg);
            }
            lst.clear();
            c.generateDecorations(false, m.getDisplayHints(), s, lst);
            for (SimTK::DecorativeGeometry const& g : lst) {
                g.implementGeometry(sgg);
            }
            lst.clear();
        }

        return rv;
    }
}

struct osc::SimbodyMeshgenScreen::Impl final {
    Shader shader;

    LoadedGeom sphere{shader, GenUntexturedUVSphere(12, 12)};
    LoadedGeom cylinder{shader, GenUntexturedSimbodyCylinder(16)};
    LoadedGeom line{shader, GenYLine()};
    LoadedGeom cube{shader, GenCube()};
    LoadedGeom cone{shader, GenUntexturedSimbodyCone(16)};

    meshgen::Scene_geom geom = meshgen::extractGeometry(shader, App::resource("models/GeometryBackendTest/full.osim"));

    PolarPerspectiveCamera camera;
};

// public Impl

osc::SimbodyMeshgenScreen::SimbodyMeshgenScreen() :
    m_Impl{new Impl{}} {
}

osc::SimbodyMeshgenScreen::~SimbodyMeshgenScreen() noexcept = default;

void osc::SimbodyMeshgenScreen::onMount() {
    osc::ImGuiInit();
}

void osc::SimbodyMeshgenScreen::onUnmount() {
    osc::ImGuiShutdown();
}

void osc::SimbodyMeshgenScreen::onEvent(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        App::cur().requestTransition<ExperimentsScreen>();
        return;
    }
}

void osc::SimbodyMeshgenScreen::tick(float) {
    UpdatePolarCameraFromImGuiUserInput(App::cur().dims(), m_Impl->camera);
}

void osc::SimbodyMeshgenScreen::draw() {
    osc::ImGuiNewFrame();

    Impl& impl = *m_Impl;
    Shader& shader = impl.shader;
    glm::ivec2 dims = App::cur().idims();
    gl::Viewport(0, 0, dims.x, dims.y);
    gl::ClearColor(0.95f, 0.95f, 0.95f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui::Begin("panel");
    ImGui::Text("hello, world");
    ImGui::End();

    gl::UseProgram(shader.prog);
    gl::Uniform(shader.uView, impl.camera.getViewMtx());
    gl::Uniform(shader.uProjection, impl.camera.getProjMtx(App::cur().aspectRatio()));

    gl::Enable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // draw spheres
    gl::BindVertexArray(impl.sphere.vao);
    for (auto const& s : impl.geom.spheres) {
        glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, glm::vec3{s.radius, s.radius, s.radius});
        glm::mat4 translator = glm::translate(glm::mat4{1.0f}, s.pos);
        gl::Uniform(shader.uModel, translator * scaler);
        gl::Uniform(shader.uColor, s.rgba);
        gl::DrawElements(GL_TRIANGLES, impl.sphere.ebo.sizei(), gl::indexType(impl.sphere.ebo), nullptr);
    }
    gl::BindVertexArray();

    // draw ellipsoids
    gl::BindVertexArray(impl.sphere.vao);
    for (auto const& c : impl.geom.ellipsoid) {
        gl::Uniform(shader.uModel, c.modelMtx);
        gl::Uniform(shader.uColor, c.rgba);
        gl::DrawElements(GL_TRIANGLES, impl.sphere.ebo.sizei(), gl::indexType(impl.sphere.ebo), nullptr);
    }
    gl::BindVertexArray();

    // draw lines
    gl::BindVertexArray(impl.cylinder.vao);
    for (auto const& c : impl.geom.lines) {
        Segment cylinder{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
        Segment line{c.p1, c.p2};
        glm::mat4 xform = SegmentToSegmentXform(cylinder, line);
        glm::mat4 radiusRescale = glm::scale(glm::mat4{1.0f}, glm::vec3{0.005f, 1.0f, 0.005f});

        gl::Uniform(shader.uModel, xform * radiusRescale);
        gl::Uniform(shader.uColor, c.rgba);
        gl::DrawElements(GL_TRIANGLES, impl.cylinder.ebo.sizei(), gl::indexType(impl.cylinder.ebo), nullptr);
    }
    gl::BindVertexArray();

    // draw cylinders
    gl::BindVertexArray(impl.cylinder.vao);
    for (auto const& c : impl.geom.cylinders) {
        gl::Uniform(shader.uModel, c.modelMtx);
        gl::Uniform(shader.uColor, c.rgba);
        gl::DrawElements(GL_TRIANGLES, impl.cylinder.ebo.sizei(), gl::indexType(impl.cylinder.ebo), nullptr);
    }
    gl::BindVertexArray();

    // draw cones
    gl::BindVertexArray(impl.cone.vao);
    for (auto const& c : impl.geom.cones) {
        Segment conemesh{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
        Segment cone{c.pos, c.pos + c.direction*c.height};
        glm::mat4 xform = SegmentToSegmentXform(conemesh, cone);
        glm::mat4 radiusRescale = glm::scale(glm::mat4{1.0f}, {c.baseRadius, 1.0f, c.baseRadius});

        gl::Uniform(shader.uModel, xform * radiusRescale);
        gl::Uniform(shader.uColor, c.rgba);
        gl::DrawElements(GL_TRIANGLES, impl.cone.ebo.sizei(), gl::indexType(impl.cone.ebo), nullptr);
    }
    gl::BindVertexArray();

    // draw arrows
    for (auto const& a : impl.geom.arrows) {
        glm::vec3 p1ToP2 = a.p2 - a.p1;
        float len = glm::length(p1ToP2);
        glm::vec3 dir = p1ToP2/len;
        constexpr float conelen = 0.2f;

        Segment meshline{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
        glm::vec3 cylinderStart = a.p1;
        glm::vec3 coneStart = a.p2 - (conelen * len * dir);
        glm::vec3 coneEnd = a.p2;

        // draw cone head
        {
            glm::mat4 coneRadiusRescaler = glm::scale(glm::mat4{1.0f}, {0.02f, 1.0f, 0.02f});
            glm::mat4 xform = SegmentToSegmentXform(meshline, Segment{coneStart, coneEnd});
            gl::Uniform(shader.uModel, xform * coneRadiusRescaler);
            gl::Uniform(shader.uColor, a.rgba);
            gl::BindVertexArray(impl.cone.vao);
            gl::DrawElements(GL_TRIANGLES, impl.cone.ebo.sizei(), gl::indexType(impl.cone.ebo), nullptr);
            gl::BindVertexArray();
        }

        // draw cylinder body
        {
            glm::mat4 cylinderRadiusRescaler = glm::scale(glm::mat4{1.0f}, {0.005f, 1.0f, 0.005f});
            glm::mat4 xform = SegmentToSegmentXform(meshline, Segment{cylinderStart, coneStart});
            gl::Uniform(shader.uModel, xform * cylinderRadiusRescaler);
            gl::Uniform(shader.uColor, a.rgba);
            gl::BindVertexArray(impl.cylinder.vao);
            gl::DrawElements(GL_TRIANGLES, impl.cylinder.ebo.sizei(), gl::indexType(impl.cylinder.ebo), nullptr);
            gl::BindVertexArray();
        }
    }

    // draw bricks
    gl::BindVertexArray(impl.cube.vao);
    for (auto const& b : impl.geom.bricks) {
        gl::Uniform(shader.uModel, b.modelMtx);
        gl::Uniform(shader.uColor, b.rgba);
        gl::DrawElements(GL_TRIANGLES, impl.cube.ebo.sizei(), gl::indexType(impl.cube.ebo), nullptr);
    }
    gl::BindVertexArray();

    // draw meshfiles
    for (auto const& mf : impl.geom.meshes) {
        gl::Uniform(shader.uModel, mf.modelMtx);
        gl::Uniform(shader.uColor, mf.rgba);
        gl::BindVertexArray(mf.geom.vao);
        gl::DrawElements(GL_TRIANGLES, mf.geom.ebo.sizei(), gl::indexType(mf.geom.ebo), nullptr);
        gl::BindVertexArray();
    }

    // draw frames
    for (auto const& f : impl.geom.frames) {
        constexpr float axlenRescale = 0.25f;
        constexpr float axThickness = 0.0025f;

        // draw origin sphere
        {
            Sphere meshSphere{{0.0f, 0.0f, 0.0f}, 1.0f};
            Sphere outputSphere{f.pos, 0.05f * axlenRescale};
            glm::mat4 xform = SphereToSphereXform(meshSphere, outputSphere);
            gl::Uniform(shader.uModel, xform);
            gl::Uniform(shader.uColor, {1.0f, 1.0f, 1.0f, 1.0f});
            gl::BindVertexArray(impl.sphere.vao);
            gl::DrawElements(GL_TRIANGLES, impl.sphere.ebo.sizei(), gl::indexType(impl.sphere.ebo), nullptr);
            gl::BindVertexArray();
        }

        // draw outer lines
        gl::BindVertexArray(impl.cylinder.vao);
        Segment cylinderline{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};

        for (int i = 0; i < 3; ++i) {
            glm::vec3 dir = {0.0f, 0.0f, 0.0f};
            dir[i] = axlenRescale*f.axisLengths[i];
            Segment axisline{f.pos, f.pos + dir};

            glm::vec3 prescale = {axThickness, 1.0f, axThickness};
            glm::mat4 prescaleMtx = glm::scale(glm::mat4{1.0f}, prescale);
            glm::vec4 color{0.0f, 0.0f, 0.0f, 1.0f};
            color[i] = 1.0f;

            glm::mat4 xform = SegmentToSegmentXform(cylinderline, axisline);
            gl::Uniform(shader.uModel, xform * prescaleMtx);
            gl::Uniform(shader.uColor, color);
            gl::DrawElements(GL_TRIANGLES, impl.cylinder.ebo.sizei(), gl::indexType(impl.cylinder.ebo), nullptr);
        }

        gl::BindVertexArray();
    }

    // draw DEBUG axis lines
    if (true) {
        gl::BindVertexArray(impl.line.vao);
        glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, {1000.0f, 1000.0f, 1000.0f});

        // X
        gl::Uniform(shader.uModel, glm::rotate(glm::mat4{1.0f}, static_cast<float>(M_PI)/2.0f, glm::vec3{0.0f, 0.0f, 1.0f}) * scaler);
        gl::Uniform(shader.uColor, {1.0f, 0.0f, 0.0f, 1.0f});
        gl::DrawElements(GL_LINES, impl.line.ebo.sizei(), gl::indexType(impl.line.ebo), nullptr);

        // Y
        gl::Uniform(shader.uModel, scaler);
        gl::Uniform(shader.uColor, {0.0f, 1.0f, 0.0f, 1.0f});
        gl::DrawElements(GL_LINES, impl.line.ebo.sizei(), gl::indexType(impl.line.ebo), nullptr);

        // Z
        gl::Uniform(shader.uModel, glm::rotate(glm::mat4{1.0f}, static_cast<float>(M_PI)/2.0f, glm::vec3{1.0f, 0.0f, 0.0f}) * scaler);
        gl::Uniform(shader.uColor, {0.0f, 0.0f, 1.0f, 1.0f});
        gl::DrawElements(GL_LINES, impl.line.ebo.sizei(), gl::indexType(impl.line.ebo), nullptr);

        gl::BindVertexArray();
    }

    osc::ImGuiRender();
}
