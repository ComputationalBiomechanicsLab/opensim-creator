#include "opensim_wrapper.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <OpenSim/OpenSim.h>
#include <iostream>
#include "meshes.hpp"

using namespace SimTK;
using namespace OpenSim;

static constexpr osim::Mesh_id sphere_meshid = 0;
static constexpr osim::Mesh_id cylinder_meshid = 1;
static constexpr size_t num_reserved_meshids = 2;  // count of above
static std::mutex g_mesh_cache_mutex;
static std::unordered_map<std::string, osim::Untextured_mesh> g_mesh_cache;

namespace osim {
    struct Geometry_loader_impl final {
        // this swap space prevents the geometry loader from having to allocate
        // space every time geometry is requested
        //Array_<DecorativeGeometry> decorative_geom_swap;
        PolygonalMesh pm_swap;

        Array_<DecorativeGeometry> dg_swp;

        // two-way lookup to establish a meshid-to-path mappings. This is so
        // that the renderer can just opaquely handle ID ints
        std::vector<std::string> meshid_to_str = std::vector<std::string>(num_reserved_meshids);
        std::unordered_map<std::string, osim::Mesh_id> str_to_meshid;
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

static glm::vec3 to_vec3(Vec3 const& v) {
    return glm::vec3{v[0], v[1], v[2]};
}

// load a SimTK PolygonalMesh into a more generic Untextured_mesh struct
static void load_mesh_data(PolygonalMesh const& mesh, osim::Untextured_mesh& out) {

    // helper function: gets a vertex for a face
    auto get_face_vert_pos = [&](int face, int vert) {
        return to_vec3(mesh.getVertexPosition(mesh.getFaceVertex(face, vert)));
    };

    auto make_triangle = [](glm::vec3 const& p1, glm::vec3 const& p2, glm::vec3 const& p3) {
        glm::vec3 normal = glm::cross(p2 - p1, p3 - p1);

        return osim::Untextured_triangle{
            .p1 = {p1, normal},
            .p2 = {p2, normal},
            .p3 = {p3, normal},
        };
    };

    std::vector<osim::Untextured_triangle>& triangles = out.triangles;
    triangles.clear();

    for (auto face = 0; face < mesh.getNumFaces(); ++face) {
        auto num_vertices = mesh.getNumVerticesForFace(face);

        if (num_vertices < 3) {
            // do nothing
        } else if (num_vertices == 3) {
            // standard triangle face


            triangles.push_back(make_triangle(
                get_face_vert_pos(face, 0),
                get_face_vert_pos(face, 1),
                get_face_vert_pos(face, 2)
            ));
        } else if (num_vertices == 4) {
            // rectangle: split into two triangles
            glm::vec3 p1 = get_face_vert_pos(face, 0);
            glm::vec3 p2 = get_face_vert_pos(face, 1);
            glm::vec3 p3 = get_face_vert_pos(face, 2);
            glm::vec3 p4 = get_face_vert_pos(face, 3);

            triangles.push_back(make_triangle(p1, p2, p3));
            triangles.push_back(make_triangle(p3, p4, p1));
        } else {
            // polygon with >= 4 edges:
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
                triangles.push_back(make_triangle(p1, p2, center));
            }
            // loop back
            {
                glm::vec3 p1 = get_face_vert_pos(face, num_vertices-1);
                glm::vec3 p2 = get_face_vert_pos(face, 0);
                triangles.push_back(make_triangle(p1, p2, center));
            }
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
    osim::Geometry_loader_impl& impl;
    osim::State_geometry& out;


    Geometry_visitor(Model& _model,
                     State const& _state,
                     osim::Geometry_loader_impl& _impl,
                     osim::State_geometry& _out) :
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
        return to_vec3(sf);
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

        out.mesh_instances.push_back(osim::Mesh_instance{
            .transform = cylinder_xform,
            .normal_xform = glm::transpose(glm::inverse(cylinder_xform)),
            .rgba = rgba(geom),

            .mesh = cylinder_meshid,
        });
    }
    void implementBrickGeometry(const DecorativeBrick&) override {
    }
    void implementCylinderGeometry(const DecorativeCylinder& geom) override {
        glm::mat4 m = transform(geom);
        glm::vec3 s = scale_factors(geom);
        s.x *= geom.getRadius();
        s.y *= geom.getHalfHeight();
        s.z *= geom.getRadius();

        auto xform = glm::scale(m, s);

        out.mesh_instances.push_back(osim::Mesh_instance{
            .transform = xform,
            .normal_xform = glm::transpose(glm::inverse(xform)),
            .rgba = rgba(geom),

            .mesh = cylinder_meshid,
        });
    }
    void implementCircleGeometry(const DecorativeCircle&) override {
    }
    void implementSphereGeometry(const DecorativeSphere& geom) override {
        float r = geom.getRadius();
        auto xform = glm::scale(transform(geom), glm::vec3{r, r, r});

        out.mesh_instances.push_back(osim::Mesh_instance{
            .transform = xform,
            .normal_xform = glm::transpose(glm::inverse(xform)),
            .rgba = rgba(geom),

            .mesh = sphere_meshid
        });
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
        osim::Mesh_id meshid = [this, &path]() {
            auto [it, inserted] = impl.str_to_meshid.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(path),
                std::make_tuple());

            if (not inserted) {
                return it->second;
            } else {
                // populate lookups

                size_t meshid_s = impl.meshid_to_str.size();
                assert(meshid_s < std::numeric_limits<osim::Mesh_id>::max());
                auto meshid = static_cast<osim::Mesh_id>(meshid_s);
                impl.meshid_to_str.push_back(path);
                it->second = meshid;
                return meshid;
            }
        }();

        out.mesh_instances.push_back(osim::Mesh_instance{
            .transform = xform,
            .normal_xform = glm::transpose(glm::inverse(xform)),
            .rgba = rgba(m),

            .mesh = meshid,
        });
    }
    void implementArrowGeometry(const DecorativeArrow&) override {
    }
    void implementTorusGeometry(const DecorativeTorus&) override {
    }
    void implementConeGeometry(const DecorativeCone&) override {
    }
};


osim::OSMV_Model::OSMV_Model(std::unique_ptr<OpenSim::Model> _m) :
    handle{std::move(_m)} {
}
osim::OSMV_Model::OSMV_Model(OSMV_Model&&) noexcept = default;
osim::OSMV_Model& osim::OSMV_Model::operator=(OSMV_Model&&) noexcept = default;
osim::OSMV_Model::~OSMV_Model() noexcept = default;

osim::OSMV_State::OSMV_State(SimTK::State const& st) :
    handle{new SimTK::State(st)} {
}
osim::OSMV_State& osim::OSMV_State::operator=(SimTK::State const& st) {
    if (handle != nullptr) {
        *handle = st;
    } else {
        handle = std::make_unique<SimTK::State>(st);
    }
    return *this;
}
osim::OSMV_State::OSMV_State(OSMV_State&&) noexcept = default;
osim::OSMV_State& osim::OSMV_State::operator=(OSMV_State&&) noexcept = default;
osim::OSMV_State::~OSMV_State() noexcept = default;

osim::OSMV_Model osim::load_osim(char const* path) {
    return OSMV_Model{std::make_unique<OpenSim::Model>(path)};
}

void osim::finalize_from_properties(OpenSim::Model& m) {
    m.finalizeFromProperties();
}

double osim::simulation_time(SimTK::State const& s) {
    return s.getTime();
}

osim::OSMV_Model osim::copy_model(OpenSim::Model const& m) {
    return OSMV_Model{std::make_unique<OpenSim::Model>(m)};
}

SimTK::State& osim::init_system(OpenSim::Model& m) {
    return m.initSystem();
}

SimTK::State& osim::upd_working_state(OpenSim::Model& m) {
    return m.updWorkingState();
}

void osim::finalize_properties_from_state(OpenSim::Model& m, SimTK::State const& s) {
    m.setPropertiesFromState(s);
}

void osim::realize_velocity(OpenSim::Model& m, SimTK::State& s) {
    m.realizeAcceleration(s);
}

void osim::realize_report(OpenSim::Model const& m, SimTK::State& s) {
    m.realizeReport(s);
}

osim::OSMV_State osim::fd_simulation(
        OpenSim::Model& model,
        SimTK::State const& initial_state,
        double final_time,
        std::function<int(Simulation_update_event const&)> reporter) {

    struct CustomAnalysis final : public Analysis {
        OpenSim::Model& m;
        std::function<int(Simulation_update_event const&)> f;

        CustomAnalysis(OpenSim::Model& _m,
                       std::function<int(Simulation_update_event const&)> _f) :
            m{_m},
            f{std::move(_f)} {
        }

        Simulation_update_event make_event(SimTK::State const& s) {
            return Simulation_update_event{
                s,
                s.getTime(),
                m.getSystem().getNumPrescribeQCalls()
            };
        }

        int begin(SimTK::State const& s) override {
            return f(make_event(s));
        }

        int step(SimTK::State const& s, int) override {
            return f(make_event(s));
        }

        int end(SimTK::State const& s) override {
            return f(make_event(s));
        }

        CustomAnalysis* clone() const override {
            return new CustomAnalysis{m, f};
        }

        const std::string& getConcreteClassName() const override {
            static std::string name = "CustomAnalysis";
            return name;
        }
    };

    model.addAnalysis(new CustomAnalysis{model, std::move(reporter)});

    OpenSim::Manager manager(model);
    manager.setWriteToStorage(false);
    SimTK::State copy = initial_state;
    manager.initialize(copy);

    return OSMV_State{manager.integrate(final_time)};
}

static osim::Motion_type convert_to_osim_motiontype(OpenSim::Coordinate::MotionType m) {
    using OpenSim::Coordinate;
    using osim::Motion_type;

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

void osim::get_coordinates(OpenSim::Model const& m,
                           SimTK::State const& st,
                           std::vector<Coordinate>& out) {

    CoordinateSet const& s = m.getCoordinateSet();
    int len = s.getSize();
    out.reserve(out.size() + static_cast<size_t>(len));
    for (int i = 0; i < len; ++i) {
        OpenSim::Coordinate const& c = s[i];
        out.push_back(osim::Coordinate{
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

void osim::get_muscle_stats(OpenSim::Model const& m, SimTK::State const& s, std::vector<Muscle_stat>& out) {
    for (OpenSim::Muscle const& musc : m.getComponentList<OpenSim::Muscle>()) {
        out.push_back(osim::Muscle_stat{
            &musc,
            &musc.getName(),
            static_cast<float>(musc.getLength(s)),
        });
    }
}

void osim::set_coord_value(
        OpenSim::Coordinate const& c,
        SimTK::State& s,
        double v) {
    c.setValue(s, v);
}

void osim::lock_coord(OpenSim::Coordinate const& c, SimTK::State& s) {
    c.setLocked(s, true);
}

void osim::unlock_coord(OpenSim::Coordinate const& c, SimTK::State& s) {
    c.setLocked(s, false);
}

void osim::disable_wrapping_surfaces(OpenSim::Model& m) {
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

void osim::enable_wrapping_surfaces(OpenSim::Model& m) {
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

void osim::compute_moment_arms(
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

void osim::get_available_outputs(
        OpenSim::Model const& m,
        std::vector<Available_output>& out) {

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

std::string osim::get_output_val(
        OpenSim::AbstractOutput const& ao,
        SimTK::State const& s) {
    return ao.getValueAsString(s);
}

double osim::get_single_double_output_val(OpenSim::AbstractOutput const& ao, SimTK::State const& s) {
    auto* o = dynamic_cast<OpenSim::Output<double> const*>(&ao);
    return o->getValue(s);
}

osim::Geometry_loader::Geometry_loader() :
    impl{new Geometry_loader_impl{}} {
}
osim::Geometry_loader::Geometry_loader(Geometry_loader&&) = default;
osim::Geometry_loader& osim::Geometry_loader::operator=(Geometry_loader&&) = default;

void osim::Geometry_loader::all_geometry_in(
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

void osim::Geometry_loader::load_mesh(Mesh_id id, Untextured_mesh &out) {
    // handle reserved meshes
    switch (id) {
    case sphere_meshid:
        osmv::unit_sphere_triangles(out.triangles);
        return;
    case cylinder_meshid:
        osmv::simbody_cylinder_triangles(12, out.triangles);
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

osim::Geometry_loader::~Geometry_loader() noexcept = default;
