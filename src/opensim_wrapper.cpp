#include "opensim_wrapper.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include "cfg.hpp"

#include <OpenSim/OpenSim.h>
#include <iostream>

using namespace SimTK;
using namespace OpenSim;

static constexpr osmv::Mesh_id sphere_meshid = 0;
static constexpr osmv::Mesh_id cylinder_meshid = 1;
static constexpr size_t num_reserved_meshids = 2;  // count of above
static std::mutex g_mesh_cache_mutex;
static std::unordered_map<std::string, std::vector<osmv::Untextured_vert>> g_mesh_cache;

namespace osmv {
    struct Geometry_loader_impl final {
        // this swap space prevents the geometry loader from having to allocate
        // space every time geometry is requested
        //Array_<DecorativeGeometry> decorative_geom_swap;
        PolygonalMesh pm_swap;

        Array_<DecorativeGeometry> dg_swp;

        // two-way lookup to establish a meshid-to-path mappings. This is so
        // that the renderer can just opaquely handle ID ints
        std::vector<std::string> meshid_to_str = std::vector<std::string>(num_reserved_meshids);
        std::unordered_map<std::string, osmv::Mesh_id> str_to_meshid;
    };
}

// create an xform that transforms the unit cylinder into a line between
// two points
static glm::mat4 cylinder_to_line_xform(float line_width,
                                 glm::vec3 const& p1,
                                 glm::vec3 const& p2) {
    glm::vec3 p1_to_p2 = p2 - p1;
    glm::vec3 c1_to_c2 = glm::vec3{0.0f, 2.0f, 0.0f};
    auto rotation =
            glm::rotate(glm::identity<glm::mat4>(),
                        glm::acos(glm::dot(glm::normalize(c1_to_c2), glm::normalize(p1_to_p2))),
                        glm::cross(glm::normalize(c1_to_c2), glm::normalize(p1_to_p2)));
    float scale = glm::length(p1_to_p2)/glm::length(c1_to_c2);
    auto scale_xform = glm::scale(glm::identity<glm::mat4>(), glm::vec3{line_width, scale, line_width});
    auto translation = glm::translate(glm::identity<glm::mat4>(), p1 + p1_to_p2/2.0f);
    return translation * rotation * scale_xform;
}

// load a SimTK PolygonalMesh into a more generic Untextured_mesh struct
static void load_mesh_data(PolygonalMesh const& mesh, std::vector<osmv::Untextured_vert>& triangles) {

    // helper function: gets a vertex for a face
    auto get_face_vert_pos = [&](int face, int vert) {
        SimTK::Vec3 pos = mesh.getVertexPosition(mesh.getFaceVertex(face, vert));
        return glm::vec3{pos[0], pos[1], pos[2]};
    };

    auto make_normal = [](glm::vec3 const& p1, glm::vec3 const& p2, glm::vec3 const& p3) {
        return glm::cross(p2 - p1, p3 - p1);
    };

    triangles.clear();

    for (auto face = 0; face < mesh.getNumFaces(); ++face) {
        auto num_vertices = mesh.getNumVerticesForFace(face);

        if (num_vertices < 3) {
            // line?: ignore
        } else if (num_vertices == 3) {
            // triangle: use as-is
            glm::vec3 p1 = get_face_vert_pos(face, 0);
            glm::vec3 p2 = get_face_vert_pos(face, 1);
            glm::vec3 p3 = get_face_vert_pos(face, 2);
            glm::vec3 normal = make_normal(p1, p2, p3);

            triangles.push_back({p1, normal});
            triangles.push_back({p2, normal});
            triangles.push_back({p3, normal});
        } else if (num_vertices == 4) {
            // quad: split into two triangles

            glm::vec3 p1 = get_face_vert_pos(face, 0);
            glm::vec3 p2 = get_face_vert_pos(face, 1);
            glm::vec3 p3 = get_face_vert_pos(face, 2);
            glm::vec3 p4 = get_face_vert_pos(face, 3);

            glm::vec3 t1_norm = make_normal(p1, p2, p3);
            glm::vec3 t2_norm = make_normal(p3, p4, p1);

            triangles.push_back({p1, t1_norm});
            triangles.push_back({p2, t1_norm});
            triangles.push_back({p3, t1_norm});

            triangles.push_back({p3, t2_norm});
            triangles.push_back({p4, t2_norm});
            triangles.push_back({p1, t2_norm});
        } else {
            // polygon (>3 edges):
            //
            // create a vertex at the average center point and attach
            // every two verices to the center as triangles.

            auto center = glm::vec3{0.0f, 0.0f, 0.0f};
            for (int vert = 0; vert < num_vertices; ++vert) {
                center += get_face_vert_pos(face, vert);
            }
            center /= num_vertices;

            for (int vert = 0; vert < num_vertices-1; ++vert) {
                glm::vec3 p1 = get_face_vert_pos(face, vert);
                glm::vec3 p2 = get_face_vert_pos(face, vert+1);
                glm::vec3 normal = make_normal(p1, p2, center);

                triangles.push_back({p1, normal});
                triangles.push_back({p2, normal});
                triangles.push_back({center, normal});
            }

            // complete the polygon loop
            glm::vec3 p1 = get_face_vert_pos(face, num_vertices-1);
            glm::vec3 p2 = get_face_vert_pos(face, 0);
            glm::vec3 normal = make_normal(p1, p2, center);

            triangles.push_back({p1, normal});
            triangles.push_back({p2, normal});
            triangles.push_back({center, normal});
        }
    }
}

// A hacky decoration generator that just always generates all geometry,
// even if it's static.
struct DynamicDecorationGenerator : public DecorationGenerator {
    Model* _model;
    DynamicDecorationGenerator(Model* model) : _model{model} {
        assert(_model != nullptr);
    }
    void useModel(Model* newModel) {
        assert(newModel != nullptr);
        _model = newModel;
    }

    void generateDecorations(const State& state, Array_<DecorativeGeometry>& geometry) override {
        _model->generateDecorations(true, _model->getDisplayHints(), state, geometry);
        _model->generateDecorations(false, _model->getDisplayHints(), state, geometry);
    }
};

struct Geometry_visitor final : public DecorativeGeometryImplementation {
    Model& model;
    State const& state;
    osmv::Geometry_loader_impl& impl;
    osmv::State_geometry& out;


    Geometry_visitor(Model& _model,
                     State const& _state,
                     osmv::Geometry_loader_impl& _impl,
                     osmv::State_geometry& _out) :
        model{_model},
        state{_state},
        impl{_impl},
        out{_out} {
    }

    Transform ground_to_decoration_xform(DecorativeGeometry const& geom) {
        SimbodyMatterSubsystem const& ms = model.getSystem().getMatterSubsystem();
        MobilizedBody const& mobod = ms.getMobilizedBody(MobilizedBodyIndex(geom.getBodyId()));
        Transform const& ground_to_body_xform = mobod.getBodyTransform(state);
        Transform const& body_to_decoration_xform = geom.getTransform();

        return ground_to_body_xform * body_to_decoration_xform;
    }

    glm::mat4 transform(DecorativeGeometry const& geom) {
        Transform t = ground_to_decoration_xform(geom);
        glm::mat4 m = glm::identity<glm::mat4>();

        // glm::mat4 is column major:
        //     see: https://glm.g-truc.net/0.9.2/api/a00001.html
        //     (and just Google "glm column major?")
        //
        // SimTK is whoknowswtf-major (actually, row), carefully read the
        // sourcecode for `SimTK::Transform`.

        // x
        m[0][0] = t.R().row(0)[0];
        m[0][1] = t.R().row(1)[0];
        m[0][2] = t.R().row(2)[0];
        m[0][3] = 0.0f;

        // y
        m[1][0] = t.R().row(0)[1];
        m[1][1] = t.R().row(1)[1];
        m[1][2] = t.R().row(2)[1];
        m[1][3] = 0.0f;

        // z
        m[2][0] = t.R().row(0)[2];
        m[2][1] = t.R().row(1)[2];
        m[2][2] = t.R().row(2)[2];
        m[2][3] = 0.0f;

        // w
        m[3][0] = t.p()[0];
        m[3][1] = t.p()[1];
        m[3][2] = t.p()[2];
        m[3][3] = 1.0f;

        return m;
    }

    glm::vec3 scale_factors(DecorativeGeometry const& geom) {
        Vec3 sf = geom.getScaleFactors();
        for (int i = 0; i < 3; ++i) {
            sf[i] = sf[i] <= 0 ? 1.0 : sf[i];
        }
        return glm::vec3{sf[0], sf[1], sf[2]};
    }

    glm::vec4 rgba(DecorativeGeometry const& geom) {
        Vec3 const& rgb = geom.getColor();
        Real a = geom.getOpacity();
        return {rgb[0], rgb[1], rgb[2], a < 0.0f ? 1.0f : a};
    }

    glm::vec4 to_vec4(Vec3 const& v, float w = 1.0f) {
        return glm::vec4{v[0], v[1], v[2], w};
    }

    void implementPointGeometry(const DecorativePoint&) override {
    }
    void implementLineGeometry(const DecorativeLine& geom) override {
        // a line is essentially a thin cylinder that connects two points
        // in space. This code eagerly performs that transformation

        glm::mat4 xform = transform(geom);
        glm::vec3 p1 = xform * to_vec4(geom.getPoint1());
        glm::vec3 p2 = xform * to_vec4(geom.getPoint2());

        glm::mat4 cylinder_xform = cylinder_to_line_xform(0.005, p1, p2);
        glm::mat4 normal_mtx = glm::transpose(glm::inverse(cylinder_xform));


        out.mesh_instances.push_back({ cylinder_xform, normal_mtx, rgba(geom), cylinder_meshid });
    }
    void implementBrickGeometry(const DecorativeBrick&) override {
    }
    void implementCylinderGeometry(const DecorativeCylinder& geom) override {
        glm::mat4 m = transform(geom);
        glm::vec3 s = scale_factors(geom);
        s.x *= geom.getRadius();
        s.y *= geom.getHalfHeight();
        s.z *= geom.getRadius();

        glm::mat4 xform = glm::scale(m, s);
        glm::mat4 normal_mtx = glm::transpose(glm::inverse(xform));

        out.mesh_instances.push_back({ xform, normal_mtx, rgba(geom), cylinder_meshid });
    }
    void implementCircleGeometry(const DecorativeCircle&) override {
    }
    void implementSphereGeometry(const DecorativeSphere& geom) override {
        float r = geom.getRadius();
        glm::mat4 xform = glm::scale(transform(geom), glm::vec3{r, r, r});
        glm::mat4 normal_mtx = glm::transpose(glm::inverse(xform));


        out.mesh_instances.push_back({ xform, normal_mtx, rgba(geom), sphere_meshid });
    }
    void implementEllipsoidGeometry(const DecorativeEllipsoid&) override {
    }
    void implementFrameGeometry(const DecorativeFrame&) override {
    }
    void implementTextGeometry(const DecorativeText&) override {
    }
    void implementMeshGeometry(const DecorativeMesh&) override {
    }
    void implementMeshFileGeometry(const DecorativeMeshFile& m) override {
        auto xform = glm::scale(transform(m), scale_factors(m));
        std::string const& path = m.getMeshFile();

        // HACK: globally cache the loaded meshes
        //
        // this is here because OpenSim "helpfully" pre-loads the fucking
        // meshes in the main thread, so the loading code is currently
        // redundantly re-loading the meshes that OpenSim loads
        //
        // this will be fixed once the Model crawling is customized to step
        // around OpenSim's shitty implementation (which can't be easily
        // multithreaded)
        {
            std::lock_guard l{g_mesh_cache_mutex};
            auto [it, inserted] = g_mesh_cache.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(path),
                std::make_tuple());

            if (inserted) {
                load_mesh_data(m.getMesh(), it->second);
            }
        }

        // load the path into the model-to-path mapping lookup
        osmv::Mesh_id meshid = [this, &path]() {
            auto [it, inserted] = impl.str_to_meshid.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(path),
                std::make_tuple());

            if (not inserted) {
                return it->second;
            } else {
                // populate lookups

                size_t meshid_s = impl.meshid_to_str.size();
                assert(meshid_s < std::numeric_limits<osmv::Mesh_id>::max());
                auto meshid = static_cast<osmv::Mesh_id>(meshid_s);
                impl.meshid_to_str.push_back(path);
                it->second = meshid;
                return meshid;
            }
        }();

        glm::mat4 normal_mtx = glm::transpose(glm::inverse(xform));

        out.mesh_instances.push_back({ xform, normal_mtx, rgba(m), meshid });
    }
    void implementArrowGeometry(const DecorativeArrow&) override {
    }
    void implementTorusGeometry(const DecorativeTorus&) override {
    }
    void implementConeGeometry(const DecorativeCone&) override {
    }
};


osmv::Model::Model(std::unique_ptr<OpenSim::Model> _m) noexcept : handle{std::move(_m)} {
}
osmv::Model::Model(OpenSim::Model const& m) : handle{new OpenSim::Model{m}} {
}
osmv::Model::Model(Model&&) noexcept = default;
osmv::Model& osmv::Model::operator=(Model&&) noexcept = default;
osmv::Model::~Model() noexcept = default;

osmv::State::State(SimTK::State const& st) :
    handle{new SimTK::State(st)} {
}
osmv::State& osmv::State::operator=(SimTK::State const& st) {
    if (handle != nullptr) {
        *handle = st;
    } else {
        handle = std::make_unique<SimTK::State>(st);
    }
    return *this;
}
osmv::State::State(State&&) noexcept = default;
osmv::State& osmv::State::operator=(State&&) noexcept = default;
osmv::State::~State() noexcept = default;

osmv::Model osmv::load_osim(std::filesystem::path const& path) {
    // HACK: osmv has a `geometry/` dir packaged with it, which OpenSim
    //       should search in
    //
    //       this sets a global in OpenSim, so only needs to be called once
    static bool _ = []() {
        std::filesystem::path geometry_dir = cfg::resource_path("geometry");
        ModelVisualizer::addDirToGeometrySearchPaths(geometry_dir.string());
        return true;
    }();

    return Model{std::make_unique<OpenSim::Model>(path.string())};
}

void osmv::finalize_from_properties(OpenSim::Model& m) {
    m.finalizeFromProperties();
}

double osmv::simulation_time(SimTK::State const& s) {
    return s.getTime();
}

int osmv::num_prescribeq_calls(OpenSim::Model const& m) {
    return m.getSystem().getNumPrescribeQCalls();
}

SimTK::State& osmv::init_system(OpenSim::Model& m) {
    return m.initSystem();
}

SimTK::State& osmv::upd_working_state(OpenSim::Model& m) {
    return m.updWorkingState();
}

void osmv::finalize_properties_from_state(OpenSim::Model& m, SimTK::State const& s) {
    m.setPropertiesFromState(s);
}

void osmv::realize_velocity(OpenSim::Model const& m, SimTK::State& s) {
    m.realizeVelocity(s);
}

void osmv::realize_report(OpenSim::Model const& m, SimTK::State& s) {
    m.realizeReport(s);
}

osmv::State osmv::fd_simulation(OpenSim::Model& model, State initial_state, Fd_sim_config const& config) {
    struct CustomAnalysis final : public Analysis {
        OpenSim::Manager& manager;
        std::function<Callback_response(SimTK::State const&)> on_integration_step;

        CustomAnalysis(OpenSim::Manager& _m, std::function<Callback_response(SimTK::State const&)> _f) :
            manager{_m},
            on_integration_step{std::move(_f)} {
        }

        int begin(SimTK::State const& s) override {
            if (on_integration_step(s) == Callback_response::Please_halt) {
                manager.halt();
            }
            return 0;
        }

        int step(SimTK::State const& s, int) override {
            if (on_integration_step(s) == Callback_response::Please_halt) {
                manager.halt();
            }
            return 0;
        }

        int end(SimTK::State const& s) override {
            if (on_integration_step(s) == Callback_response::Please_halt) {
                manager.halt();
            }
            return 0;
        }

        CustomAnalysis* clone() const override {
            return new CustomAnalysis{manager, on_integration_step};
        }

        std::string const& getConcreteClassName() const override {
            static std::string const name = "CustomAnalysis";
            return name;
        }
    };

    OpenSim::Manager manager{model};

    if (config.on_integration_step) {
        model.addAnalysis(new CustomAnalysis{manager, *config.on_integration_step});
    }

    manager.setWriteToStorage(false);
    manager.setIntegratorInternalStepLimit(config.max_steps);
    manager.setIntegratorMaximumStepSize(config.max_step_size);
    manager.setIntegratorMinimumStepSize(config.min_step_size);
    manager.setIntegratorAccuracy(config.integrator_accuracy);

    model.getMultibodySystem().realize(initial_state, Stage::Position);
    model.equilibrateMuscles(initial_state);

    manager.initialize(initial_state);
    return State{manager.integrate(config.final_time)};
}

osmv::State osmv::fd_simulation(OpenSim::Model& model) {
    return fd_simulation(model, State{osmv::init_system(model)}, Fd_sim_config{});
}

static osmv::Motion_type convert_to_osim_motiontype(OpenSim::Coordinate::MotionType m) {
    using OpenSim::Coordinate;
    using osmv::Motion_type;

    switch (m) {
    case Coordinate::MotionType::Undefined:
        return Motion_type::Undefined;
    case Coordinate::MotionType::Rotational:
        return Motion_type::Rotational;
    case Coordinate::MotionType::Translational:
        return Motion_type::Translational;
    case Coordinate::MotionType::Coupled:
        return Motion_type::Coupled;
    default:
        throw std::runtime_error{"convert_to_osim_motiontype: unknown coordinate type encountered"};
    }
}

void osmv::get_coordinates(OpenSim::Model const& m,
                           SimTK::State const& st,
                           std::vector<Coordinate>& out) {

    CoordinateSet const& s = m.getCoordinateSet();
    int len = s.getSize();
    out.reserve(out.size() + static_cast<size_t>(len));
    for (int i = 0; i < len; ++i) {
        OpenSim::Coordinate const& c = s[i];
        out.push_back(osmv::Coordinate{
            &c,
            &c.getName(),
            static_cast<float>(c.getRangeMin()),
            static_cast<float>(c.getRangeMax()),
            static_cast<float>(c.getValue(st)),
            convert_to_osim_motiontype(c.getMotionType()),
            c.getLocked(st),
        });
    }
}

void osmv::get_muscle_stats(OpenSim::Model const& m, SimTK::State const& s, std::vector<Muscle_stat>& out) {
    for (OpenSim::Muscle const& musc : m.getComponentList<OpenSim::Muscle>()) {
        out.push_back(osmv::Muscle_stat{
            &musc,
            &musc.getName(),
            static_cast<float>(musc.getLength(s)),
        });
    }
}

void osmv::set_coord_value(
        OpenSim::Coordinate const& c,
        SimTK::State& s,
        double v) {
    c.setValue(s, v);
}

void osmv::lock_coord(OpenSim::Coordinate const& c, SimTK::State& s) {
    c.setLocked(s, true);
}

void osmv::unlock_coord(OpenSim::Coordinate const& c, SimTK::State& s) {
    c.setLocked(s, false);
}

void osmv::disable_wrapping_surfaces(OpenSim::Model& m) {
    OpenSim::ComponentList<OpenSim::WrapObjectSet> l =
            m.updComponentList<OpenSim::WrapObjectSet>();
    for (OpenSim::WrapObjectSet& wos : l) {
        for (int i = 0; i < wos.getSize(); ++i) {
            OpenSim::WrapObject& wo = wos[i];
            wo.set_active(false);
            wo.upd_Appearance().set_visible(false);
        }
    }
}

void osmv::enable_wrapping_surfaces(OpenSim::Model& m) {
    OpenSim::ComponentList<OpenSim::WrapObjectSet> l =
            m.updComponentList<OpenSim::WrapObjectSet>();
    for (OpenSim::WrapObjectSet& wos : l) {
        for (int i = 0; i < wos.getSize(); ++i) {
            OpenSim::WrapObject& wo = wos[i];
            wo.set_active(true);
            wo.upd_Appearance().set_visible(true);
        }
    }
}

void osmv::compute_moment_arms(
        OpenSim::Muscle const& muscle,
        SimTK::State const& st,
        OpenSim::Coordinate const& c,
        float* out,
        size_t steps) {

    SimTK::State state = st;
    realize_report(muscle.getModel(), state);

    bool prev_locked = c.getLocked(state);
    double prev_val = c.getValue(state);

    c.setLocked(state, false);

    double start = c.getRangeMin();
    double end = c.getRangeMax();
    double step = (end - start) / steps;

    for (size_t i = 0; i < steps; ++i) {
        double v = start + (i * step);
        c.setValue(state, v);
        out[i] = static_cast<float>(muscle.getGeometryPath().computeMomentArm(state, c));
    }

    c.setLocked(state, prev_locked);
    c.setValue(state, prev_val);
}

void osmv::get_available_outputs(OpenSim::Model const& m, std::vector<Available_output>& out) {

    auto is_single_double_val = [](OpenSim::AbstractOutput const* ao) {
        return (not ao->isListOutput()) and dynamic_cast<OpenSim::Output<double> const*>(ao) != nullptr;
    };

    for (auto const& p : m.getOutputs()) {
        out.push_back(Available_output{
            &m.getName(),
            &p.second->getName(),
            p.second.get(),
            is_single_double_val(p.second.get()),
        });
    }

    for (auto const& musc : m.getComponentList<OpenSim::Muscle>()) {
        for (auto const& p : musc.getOutputs()) {
            out.push_back(Available_output{
                &musc.getName(),
                &p.second->getName(),
                p.second.get(),
                is_single_double_val(p.second.get()),
            });
        }
    }
}

std::string osmv::get_output_val_any(OpenSim::AbstractOutput const& ao, SimTK::State const& s) {
    return ao.getValueAsString(s);
}

double osmv::get_output_val_double(OpenSim::AbstractOutput const& ao, SimTK::State const& s) {
    auto* o = dynamic_cast<OpenSim::Output<double> const*>(&ao);
    assert(o);
    return o->getValue(s);
}

osmv::Geometry_loader::Geometry_loader() :
    impl{new Geometry_loader_impl{}} {
}
osmv::Geometry_loader::Geometry_loader(Geometry_loader&&) = default;
osmv::Geometry_loader& osmv::Geometry_loader::operator=(Geometry_loader&&) = default;

void osmv::Geometry_loader::all_geometry_in(
    OpenSim::Model& m,
    SimTK::State& s,
    State_geometry& out) {

    impl->pm_swap.clear();
    impl->dg_swp.clear();

    DynamicDecorationGenerator dg{&m};
    dg.generateDecorations(s, impl->dg_swp);

    auto visitor = Geometry_visitor{m, s, *impl, out};
    for (DecorativeGeometry& dg : impl->dg_swp) {
        dg.implementGeometry(visitor);
    }
}

void osmv::Geometry_loader::load_mesh(Mesh_id id, std::vector<Untextured_vert>& out) {
    // handle reserved mesh IDs
    switch (id) {
    case sphere_meshid:
        osmv::unit_sphere_triangles(out);
        return;
    case cylinder_meshid:
        osmv::simbody_cylinder_triangles(12, out);
        return;
    }

    std::string const& path = impl->meshid_to_str.at(id);

    std::lock_guard l{g_mesh_cache_mutex};

    auto [it, inserted] = g_mesh_cache.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(path),
        std::make_tuple());

    if (inserted) {
        // wasn't cached: load the mesh

        PolygonalMesh& mesh = impl->pm_swap;
        mesh.clear();
        mesh.loadFile(path);

        load_mesh_data(mesh, it->second);
    }

    out = it->second;
}

osmv::Geometry_loader::~Geometry_loader() noexcept = default;
