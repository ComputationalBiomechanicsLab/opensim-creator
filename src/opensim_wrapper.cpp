#include "opensim_wrapper.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <OpenSim/OpenSim.h>
#include <iostream>
#include "glm_extensions.hpp"

using namespace SimTK;
using namespace OpenSim;

namespace osim {
    struct Geometry_loader_impl final {
        Array_<DecorativeGeometry> tmp;
        std::unordered_map<std::string, int> path_to_id;
        std::vector<std::string> id_to_path;
    };
}

namespace {
    void generateGeometry(Model& model, State const& state, Array_<DecorativeGeometry>& geometry) {
        model.generateDecorations(true, model.getDisplayHints(), state, geometry);
        ComponentList<const Component> allComps = model.getComponentList();
        ComponentList<Component>::const_iterator iter = allComps.begin();
        while (iter != allComps.end()){
            //std::string cn = iter->getConcreteClassName();
            //std::cout << cn << ":" << iter->getName() << std::endl;
            iter->generateDecorations(true, model.getDisplayHints(), state, geometry);
            iter++;
        }

        // necessary to render muscles
        DefaultGeometry dg{model};
        dg.generateDecorations(state, geometry);
    }

    glm::vec3 to_vec3(Vec3 const& v) {
        return glm::vec3{v[0], v[1], v[2]};
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
            generateGeometry(*_model, state, geometry);
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
            glm::mat4 xform = transform(geom);
            glm::vec4 p1 = xform * to_vec4(geom.getPoint1());
            glm::vec4 p2 = xform * to_vec4(geom.getPoint2());

            out.lines.push_back(osim::Line{
                .p1 = {p1.x, p1.y, p1.z},
                .p2 = {p2.x, p2.y, p2.z},
                .rgba = rgba(geom)
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

            out.cylinders.push_back(osim::Cylinder{
                .transform = xform,
                .normal_xform = glm::transpose(glm::inverse(xform)),
                .rgba = rgba(geom),
            });
        }
        void implementCircleGeometry(const DecorativeCircle&) override {
        }
        void implementSphereGeometry(const DecorativeSphere& geom) override {
            float r = geom.getRadius();
            auto xform = glm::scale(transform(geom), glm::vec3{r, r, r});

            out.spheres.push_back(osim::Sphere{
                .transform = xform,
                .normal_xform = glm::transpose(glm::inverse(xform)),
                .rgba = rgba(geom),
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

            auto& p2id = impl.path_to_id;
            std::string const& path = m.getMeshFile();

            int id = -1;
            auto it = p2id.find(path);
            if (it == p2id.end()) {
                // path wasn't previously encountered, so allocate a new mesh
                // ID, which requires populating the lookups

                auto& id2p = impl.id_to_path;

                auto maybe_new_id = id2p.size();
                if (maybe_new_id >= std::numeric_limits<int>::max()) {
                    throw std::runtime_error{"ran out of IDs when trying to allocate a new mesh ID"};
                }
                id = static_cast<int>(maybe_new_id);

                // populate lookups
                id2p.push_back(path);
                p2id[path] = id;
            } else {
                id = it->second;
            }

            out.mesh_instances.push_back(osim::Mesh_instance{
                .transform = xform,
                .normal_xform = glm::transpose(glm::inverse(xform)),
                .rgba = rgba(m),

                .mesh = id,
            });
        }
        void implementArrowGeometry(const DecorativeArrow&) override {
        }
        void implementTorusGeometry(const DecorativeTorus&) override {
        }
        void implementConeGeometry(const DecorativeCone&) override {
        }
    };
}

static osim::Mesh load(PolygonalMesh const& mesh) {

    // helper function: gets a vertex for a face
    auto get_face_vert = [&](int face, int vert) {
        return to_vec3(mesh.getVertexPosition(mesh.getFaceVertex(face, vert)));
    };

    std::vector<osim::Triangle> triangles;

    for (auto face = 0; face < mesh.getNumFaces(); ++face) {
        auto num_vertices = mesh.getNumVerticesForFace(face);

        if (num_vertices < 3) {
            // do nothing
        } else if (num_vertices == 3) {
            // standard triangle face

            triangles.push_back(osim::Triangle{
                get_face_vert(face, 0),
                get_face_vert(face, 1),
                get_face_vert(face, 2)
            });
        } else if (num_vertices == 4) {
            // rectangle: split into two triangles

            triangles.push_back(osim::Triangle{
                get_face_vert(face, 0),
                get_face_vert(face, 1),
                get_face_vert(face, 2)
            });
            triangles.push_back(osim::Triangle{
                get_face_vert(face, 2),
                get_face_vert(face, 3),
                get_face_vert(face, 0)
            });
        } else {
            // polygon with >= 4 edges:
            //
            // create a vertex at the average center point and attach
            // every two verices to the center as triangles.

            auto center = glm::vec3{0.0f, 0.0f, 0.0f};
            for (int vert = 0; vert < num_vertices; ++vert) {
                center += get_face_vert(face, vert);
            }
            center /= num_vertices;

            for (int vert = 0; vert < num_vertices-1; ++vert) {
                triangles.push_back(osim::Triangle{
                    get_face_vert(face, vert),
                    get_face_vert(face, vert+1),
                    center
                });
            }
            // loop back
            triangles.push_back(osim::Triangle{
                get_face_vert(face, num_vertices-1),
                get_face_vert(face, 0),
                center
            });
        }
    }

    return osim::Mesh{std::move(triangles)};
}

std::ostream& osim::operator<<(std::ostream& o, osim::Line const& l) {
    o << "line:" << std::endl
      << "     p1 = " << l.p1 << std::endl
      << "     p2 = " << l.p2 << std::endl
      << "     rgba = " << l.rgba << std::endl;
    return o;
}

std::ostream& osim::operator<<(std::ostream& o, osim::Sphere const& s) {
    o << "sphere:" << std::endl
      << "    transform = " << s.transform << std::endl
      << "    color = " << s.rgba << std::endl;
    return o;
}

std::ostream& osim::operator<<(std::ostream& o, osim::Mesh_instance const& m) {
    o << "mesh:" << std::endl
      << "    transform = " << m.transform << std::endl
      << "    rgba = " << m.rgba << std::endl;
    return o;
}

namespace osim {
    struct Model_handle {
        Model model;

        Model_handle(char const* path) : model{path} {
            model.finalizeFromProperties();
            model.finalizeConnections();
            model.buildSystem();
        }
    };
}

osim::Model_wrapper::~Model_wrapper() noexcept = default;

osim::Model_wrapper osim::load_osim(char const* path) {
    return Model_wrapper{std::make_shared<Model_handle>(path)};
}

namespace osim {
    struct State_handle {
        std::shared_ptr<Model_handle> model;
        State state;

        State_handle(std::shared_ptr<Model_handle> _model) :
            model{std::move(_model)},
            state{model->model.initializeState()} {

            model->model.equilibrateMuscles(state);
        }
    };
}

osim::State_wrapper::~State_wrapper() noexcept = default;

osim::State_wrapper osim::initial_state(Model_wrapper& mw) {
    return State_wrapper{std::make_unique<State_handle>(mw.handle)};
}

osim::Geometry_loader::Geometry_loader() :
    impl{new Geometry_loader_impl{}}
{
}

osim::Geometry_loader::Geometry_loader(Geometry_loader&&) = default;

osim::Geometry_loader& osim::Geometry_loader::operator=(Geometry_loader&&) = default;

void osim::Geometry_loader::geometry_in(const State_wrapper &st, State_geometry &out) {
    out.lines.clear();
    out.spheres.clear();
    out.cylinders.clear();
    out.mesh_instances.clear();
    impl->tmp.clear();

    OpenSim::Model& m = st.handle->model->model;
    SimTK::State& s = st.handle->state;

    DynamicDecorationGenerator dg{&m};
    dg.generateDecorations(st.handle->state, impl->tmp);

    auto visitor = Geometry_visitor{m, s, *impl, out};
    for (DecorativeGeometry& dg : impl->tmp) {
        dg.implementGeometry(visitor);
    }
}

std::string const& osim::Geometry_loader::path_to(Mesh_id mesh) const {
    assert(impl->id_to_path.size() >= static_cast<size_t>(mesh));
    return impl->id_to_path[static_cast<size_t>(mesh)];
}

osim::Geometry_loader::~Geometry_loader() noexcept = default;

namespace osim {
    // this exists in case the mesh loader impl needs to hold onto some state
    struct Mesh_loader_impl final {
        PolygonalMesh mesh;
    };
}

osim::Mesh_loader::Mesh_loader() :
    impl{new Mesh_loader_impl{}}
{
}
osim::Mesh_loader::Mesh_loader(Mesh_loader&&) = default;
osim::Mesh_loader& osim::Mesh_loader::Mesh_loader::operator=(Mesh_loader&&) = default;

void osim::Mesh_loader::load(std::string const& path, Mesh& out) {
    PolygonalMesh& mesh = impl->mesh;
    mesh.clear();
    mesh.loadFile(path);

    // helper function: gets a vertex for a face
    auto get_face_vert = [&](int face, int vert) {
        return to_vec3(mesh.getVertexPosition(mesh.getFaceVertex(face, vert)));
    };

    std::vector<osim::Triangle>& triangles = out.triangles;
    triangles.clear();

    for (auto face = 0; face < mesh.getNumFaces(); ++face) {
        auto num_vertices = mesh.getNumVerticesForFace(face);

        if (num_vertices < 3) {
            // do nothing
        } else if (num_vertices == 3) {
            // standard triangle face

            triangles.push_back(osim::Triangle{
                get_face_vert(face, 0),
                get_face_vert(face, 1),
                get_face_vert(face, 2)
            });
        } else if (num_vertices == 4) {
            // rectangle: split into two triangles

            triangles.push_back(osim::Triangle{
                get_face_vert(face, 0),
                get_face_vert(face, 1),
                get_face_vert(face, 2)
            });
            triangles.push_back(osim::Triangle{
                get_face_vert(face, 2),
                get_face_vert(face, 3),
                get_face_vert(face, 0)
            });
        } else {
            // polygon with >= 4 edges:
            //
            // create a vertex at the average center point and attach
            // every two verices to the center as triangles.

            auto center = glm::vec3{0.0f, 0.0f, 0.0f};
            for (int vert = 0; vert < num_vertices; ++vert) {
                center += get_face_vert(face, vert);
            }
            center /= num_vertices;

            for (int vert = 0; vert < num_vertices-1; ++vert) {
                triangles.push_back(osim::Triangle{
                    get_face_vert(face, vert),
                    get_face_vert(face, vert+1),
                    center
                });
            }
            // loop back
            triangles.push_back(osim::Triangle{
                get_face_vert(face, num_vertices-1),
                get_face_vert(face, 0),
                center
            });
        }
    }
}

osim::Mesh_loader::~Mesh_loader() noexcept = default;
