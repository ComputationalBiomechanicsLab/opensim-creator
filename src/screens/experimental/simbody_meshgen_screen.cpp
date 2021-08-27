#include "simbody_meshgen_screen.hpp"

#include "src/app.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/gl_glm.hpp"
#include "src/3d/model.hpp"
#include "src/screens/experimental/experiments_screen.hpp"
#include "src/simtk_bindings/stk_meshloader.hpp"
#include "src/simtk_bindings/stk_geometry_generator.hpp"

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
            gl::CompileFromSource<gl::Vertex_shader>(g_VertexShader),
            gl::CompileFromSource<gl::Fragment_shader>(g_FragmentShader));

        gl::Attribute_vec3 aPos{0};

        gl::Uniform_mat4 uModel = gl::GetUniformLocation(prog, "uModelMat");
        gl::Uniform_mat4 uView = gl::GetUniformLocation(prog, "uViewMat");
        gl::Uniform_mat4 uProjection = gl::GetUniformLocation(prog, "uProjMat");
        gl::Uniform_vec4 uColor = gl::GetUniformLocation(prog, "uColor");
    };


    gl::Vertex_array make_vao(Shader& shader, gl::Array_buffer<glm::vec3>& vbo, gl::Element_array_buffer<GLushort>& ebo) {
        gl::Vertex_array rv;
        gl::BindVertexArray(rv);
        gl::BindBuffer(vbo);
        gl::BindBuffer(ebo);
        gl::VertexAttribPointer(shader.aPos, false, sizeof(glm::vec3), 0);
        gl::EnableVertexAttribArray(shader.aPos);
        gl::BindVertexArray();
        return rv;
    }

    struct Loaded_geom final {
        NewMesh mesh;
        gl::Array_buffer<glm::vec3> vbo;
        gl::Element_array_buffer<GLushort> ebo;
        gl::Vertex_array vao;

        Loaded_geom(Shader& s, NewMesh mesh_) :
            mesh{std::move(mesh_)},
            vbo{mesh.verts},
            ebo{mesh.indices},
            vao{make_vao(s, vbo, ebo)} {
        }
    };
}

namespace meshgen {
    using namespace osc;

    struct Loaded_meshfile final {
        Loaded_geom geom;
        glm::mat4 model_mtx;
        glm::vec4 rgba;

        Loaded_meshfile(Shader& s, Simbody_geometry::MeshFile const& mf) :
            geom{s, stk_load_mesh(*mf.path)},
            model_mtx{mf.model_mtx},
            rgba{mf.rgba} {
        }
    };

    struct Scene_geom final {
        std::vector<Simbody_geometry::Sphere> spheres;
        std::vector<Simbody_geometry::Line> lines;
        std::vector<Simbody_geometry::Cylinder> cylinders;
        std::vector<Simbody_geometry::Brick> bricks;
        std::vector<Simbody_geometry::Frame> frames;
        std::vector<Loaded_meshfile> meshes;
        std::vector<Simbody_geometry::Ellipsoid> ellipsoid;
        std::vector<Simbody_geometry::Cone> cones;
        std::vector<Simbody_geometry::Arrow> arrows;
    };

    Scene_geom extract_geom(Shader& shader, std::filesystem::path const& p) {
        Scene_geom rv;

        OpenSim::Model m{p.string()};
        m.finalizeFromProperties();
        m.finalizeConnections();
        SimTK::State& s = m.initSystem();

        auto on_emit = [&](Simbody_geometry const& g) {
            switch (g.geom_type) {
            case Simbody_geometry::Type::Sphere:
                rv.spheres.emplace_back(g.sphere);
                break;
            case Simbody_geometry::Type::Line:
                rv.lines.emplace_back(g.line);
                break;
            case Simbody_geometry::Type::Cylinder:
                rv.cylinders.emplace_back(g.cylinder);
                break;
            case Simbody_geometry::Type::Brick:
                rv.bricks.emplace_back(g.brick);
                break;
            case Simbody_geometry::Type::MeshFile:
                rv.meshes.emplace_back(shader, g.meshfile);
                break;
            case Simbody_geometry::Type::Frame:
                rv.frames.emplace_back(g.frame);
                break;
            case Simbody_geometry::Type::Ellipsoid:
                rv.ellipsoid.emplace_back(g.ellipsoid);
                break;
            case Simbody_geometry::Type::Cone:
                rv.cones.emplace_back(g.cone);
                break;
            case Simbody_geometry::Type::Arrow: {
                using osc::operator<<;
                rv.arrows.emplace_back(g.arrow);
                break; }
            default:
                std::cerr << g << std::endl;
                break;
            }
        };

        Geometry_generator_lambda sgg{m.getMatterSubsystem(), s, on_emit};
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

struct osc::Simbody_meshgen_screen::Impl final {
    Shader shader;

    Loaded_geom sphere{shader, gen_untextured_uv_sphere(12, 12)};
    Loaded_geom cylinder{shader, gen_untextured_simbody_cylinder(16)};
    Loaded_geom line{shader, gen_y_line()};
    Loaded_geom cube{shader, gen_cube()};
    Loaded_geom cone{shader, gen_untextured_simbody_cone(16)};

    meshgen::Scene_geom geom = meshgen::extract_geom(shader, App::resource("models/GeometryBackendTest/full.osim"));

    Polar_perspective_camera camera;
};

// public Impl

osc::Simbody_meshgen_screen::Simbody_meshgen_screen() :
    m_Impl{new Impl{}} {
}

osc::Simbody_meshgen_screen::~Simbody_meshgen_screen() noexcept = default;

void osc::Simbody_meshgen_screen::on_mount() {
    osc::ImGuiInit();
}

void osc::Simbody_meshgen_screen::on_unmount() {
    osc::ImGuiShutdown();
}

void osc::Simbody_meshgen_screen::on_event(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        App::cur().request_transition<Experiments_screen>();
        return;
    }
}

void osc::Simbody_meshgen_screen::tick(float) {
    Impl& impl = *m_Impl;

    impl.camera.radius *= 1.0f - ImGui::GetIO().MouseWheel/10.0f;

    // handle panning/zooming/dragging with middle mouse
    if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {

        // in pixels, e.g. [800, 600]
        glm::vec2 screendims = App::cur().dims();

        // in pixels, e.g. [-80, 30]
        glm::vec2 mouse_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle, 0.0f);
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);

        // as a screensize-independent ratio, e.g. [-0.1, 0.05]
        glm::vec2 relative_delta = mouse_delta / screendims;

        if (ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT)) {
            // shift + middle-mouse performs a pan
            float aspect_ratio = screendims.x / screendims.y;
            impl.camera.do_pan(aspect_ratio, relative_delta);
        } else if (ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL)) {
            // shift + middle-mouse performs a zoom
            impl.camera.radius *= 1.0f + relative_delta.y;
        } else {
            // just middle-mouse performs a mouse drag
            impl.camera.do_drag(relative_delta);
        }
    }
}

void osc::Simbody_meshgen_screen::draw() {
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
    gl::Uniform(shader.uView, impl.camera.view_matrix());
    gl::Uniform(shader.uProjection, impl.camera.projection_matrix(App::cur().aspect_ratio()));

    gl::Enable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // draw spheres
    gl::BindVertexArray(impl.sphere.vao);
    for (auto const& s : impl.geom.spheres) {
        glm::mat4 scaler = glm::scale(glm::mat4{1.0f}, glm::vec3{s.radius, s.radius, s.radius});
        glm::mat4 translator = glm::translate(glm::mat4{1.0f}, s.pos);
        gl::Uniform(shader.uModel, translator * scaler);
        gl::Uniform(shader.uColor, s.rgba);
        gl::DrawElements(GL_TRIANGLES, impl.sphere.ebo.sizei(), gl::index_type(impl.sphere.ebo), nullptr);
    }
    gl::BindVertexArray();

    // draw ellipsoids
    gl::BindVertexArray(impl.sphere.vao);
    for (auto const& c : impl.geom.ellipsoid) {
        gl::Uniform(shader.uModel, c.model_mtx);
        gl::Uniform(shader.uColor, c.rgba);
        gl::DrawElements(GL_TRIANGLES, impl.sphere.ebo.sizei(), gl::index_type(impl.sphere.ebo), nullptr);
    }
    gl::BindVertexArray();

    // draw lines
    gl::BindVertexArray(impl.cylinder.vao);
    for (auto const& c : impl.geom.lines) {
        Segment cylinder{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
        Segment line{c.p1, c.p2};
        glm::mat4 xform = segment_to_segment_xform(cylinder, line);
        glm::mat4 radius_rescale = glm::scale(glm::mat4{1.0f}, glm::vec3{0.005f, 1.0f, 0.005f});

        gl::Uniform(shader.uModel, xform * radius_rescale);
        gl::Uniform(shader.uColor, c.rgba);
        gl::DrawElements(GL_TRIANGLES, impl.cylinder.ebo.sizei(), gl::index_type(impl.cylinder.ebo), nullptr);
    }
    gl::BindVertexArray();

    // draw cylinders
    gl::BindVertexArray(impl.cylinder.vao);
    for (auto const& c : impl.geom.cylinders) {
        gl::Uniform(shader.uModel, c.model_mtx);
        gl::Uniform(shader.uColor, c.rgba);
        gl::DrawElements(GL_TRIANGLES, impl.cylinder.ebo.sizei(), gl::index_type(impl.cylinder.ebo), nullptr);
    }
    gl::BindVertexArray();

    // draw cones
    gl::BindVertexArray(impl.cone.vao);
    for (auto const& c : impl.geom.cones) {
        Segment conemesh{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
        Segment cone{c.pos, c.pos + c.direction*c.height};
        glm::mat4 xform = segment_to_segment_xform(conemesh, cone);
        glm::mat4 radius_rescale = glm::scale(glm::mat4{1.0f}, {c.base_radius, 1.0f, c.base_radius});

        gl::Uniform(shader.uModel, xform * radius_rescale);
        gl::Uniform(shader.uColor, c.rgba);
        gl::DrawElements(GL_TRIANGLES, impl.cone.ebo.sizei(), gl::index_type(impl.cone.ebo), nullptr);
    }
    gl::BindVertexArray();

    // draw arrows
    for (auto const& a : impl.geom.arrows) {
        glm::vec3 p1_to_p2 = a.p2 - a.p1;
        float len = glm::length(p1_to_p2);
        glm::vec3 dir = p1_to_p2/len;
        constexpr float conelen = 0.2f;

        Segment meshline{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};
        glm::vec3 cylinder_start = a.p1;
        glm::vec3 cone_start = a.p2 - (conelen * len * dir);
        glm::vec3 cone_end = a.p2;

        // draw cone head
        {
            glm::mat4 cone_radius_rescaler = glm::scale(glm::mat4{1.0f}, {0.02f, 1.0f, 0.02f});
            glm::mat4 xform = segment_to_segment_xform(meshline, Segment{cone_start, cone_end});
            gl::Uniform(shader.uModel, xform * cone_radius_rescaler);
            gl::Uniform(shader.uColor, a.rgba);
            gl::BindVertexArray(impl.cone.vao);
            gl::DrawElements(GL_TRIANGLES, impl.cone.ebo.sizei(), gl::index_type(impl.cone.ebo), nullptr);
            gl::BindVertexArray();
        }

        // draw cylinder body
        {
            glm::mat4 cylinder_radius_rescaler = glm::scale(glm::mat4{1.0f}, {0.005f, 1.0f, 0.005f});
            glm::mat4 xform = segment_to_segment_xform(meshline, Segment{cylinder_start, cone_start});
            gl::Uniform(shader.uModel, xform * cylinder_radius_rescaler);
            gl::Uniform(shader.uColor, a.rgba);
            gl::BindVertexArray(impl.cylinder.vao);
            gl::DrawElements(GL_TRIANGLES, impl.cylinder.ebo.sizei(), gl::index_type(impl.cylinder.ebo), nullptr);
            gl::BindVertexArray();
        }
    }

    // draw bricks
    gl::BindVertexArray(impl.cube.vao);
    for (auto const& b : impl.geom.bricks) {
        gl::Uniform(shader.uModel, b.model_mtx);
        gl::Uniform(shader.uColor, b.rgba);
        gl::DrawElements(GL_TRIANGLES, impl.cube.ebo.sizei(), gl::index_type(impl.cube.ebo), nullptr);
    }
    gl::BindVertexArray();

    // draw meshfiles
    for (auto const& mf : impl.geom.meshes) {
        gl::Uniform(shader.uModel, mf.model_mtx);
        gl::Uniform(shader.uColor, mf.rgba);
        gl::BindVertexArray(mf.geom.vao);
        gl::DrawElements(GL_TRIANGLES, mf.geom.ebo.sizei(), gl::index_type(mf.geom.ebo), nullptr);
        gl::BindVertexArray();
    }

    // draw frames
    for (auto const& f : impl.geom.frames) {
        constexpr float axlen_rescale = 0.25f;
        constexpr float ax_thickness = 0.0025f;

        // draw origin sphere
        {
            Sphere mesh_sphere{{0.0f, 0.0f, 0.0f}, 1.0f};
            Sphere output_sphere{f.pos, 0.05f * axlen_rescale};
            glm::mat4 xform = sphere_to_sphere_xform(mesh_sphere, output_sphere);
            gl::Uniform(shader.uModel, xform);
            gl::Uniform(shader.uColor, {1.0f, 1.0f, 1.0f, 1.0f});
            gl::BindVertexArray(impl.sphere.vao);
            gl::DrawElements(GL_TRIANGLES, impl.sphere.ebo.sizei(), gl::index_type(impl.sphere.ebo), nullptr);
            gl::BindVertexArray();
        }

        // draw outer lines
        gl::BindVertexArray(impl.cylinder.vao);
        Segment cylinderline{{0.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}};

        for (int i = 0; i < 3; ++i) {
            glm::vec3 dir = {0.0f, 0.0f, 0.0f};
            dir[i] = axlen_rescale*f.axis_lengths[i];
            Segment axisline{f.pos, f.pos + dir};

            glm::vec3 prescale = {ax_thickness, 1.0f, ax_thickness};
            glm::mat4 prescale_mtx = glm::scale(glm::mat4{1.0f}, prescale);
            glm::vec4 color{0.0f, 0.0f, 0.0f, 1.0f};
            color[i] = 1.0f;

            glm::mat4 xform = segment_to_segment_xform(cylinderline, axisline);
            gl::Uniform(shader.uModel, xform * prescale_mtx);
            gl::Uniform(shader.uColor, color);
            gl::DrawElements(GL_TRIANGLES, impl.cylinder.ebo.sizei(), gl::index_type(impl.cylinder.ebo), nullptr);
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
        gl::DrawElements(GL_LINES, impl.line.ebo.sizei(), gl::index_type(impl.line.ebo), nullptr);

        // Y
        gl::Uniform(shader.uModel, scaler);
        gl::Uniform(shader.uColor, {0.0f, 1.0f, 0.0f, 1.0f});
        gl::DrawElements(GL_LINES, impl.line.ebo.sizei(), gl::index_type(impl.line.ebo), nullptr);

        // Z
        gl::Uniform(shader.uModel, glm::rotate(glm::mat4{1.0f}, static_cast<float>(M_PI)/2.0f, glm::vec3{1.0f, 0.0f, 0.0f}) * scaler);
        gl::Uniform(shader.uColor, {0.0f, 0.0f, 1.0f, 1.0f});
        gl::DrawElements(GL_LINES, impl.line.ebo.sizei(), gl::index_type(impl.line.ebo), nullptr);

        gl::BindVertexArray();
    }

    osc::ImGuiRender();
}
