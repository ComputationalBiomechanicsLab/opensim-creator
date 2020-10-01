#include "opensim_wrapper.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <OpenSim/OpenSim.h>

using namespace SimTK;
using namespace OpenSim;

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
        State& state;
        std::vector<osim::Geometry>& out;

        Geometry_visitor(Model& _model,
                         State& _state,
                         std::vector<osim::Geometry>& _out) :
            model{_model},
            state{_state},
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

        glm::vec3 to_vec3(Vec3 const& v) {
            return glm::vec3{v[0], v[1], v[2]};
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
            out.push_back(osim::Line{
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

            out.push_back(osim::Cylinder{
                .transform = m,
                .scale = s,
                .rgba = rgba(geom),
            });
        }
        void implementCircleGeometry(const DecorativeCircle&) override {
        }
        void implementSphereGeometry(const DecorativeSphere& geom) override {
            out.push_back(osim::Sphere{
                .transform = transform(geom),
                .rgba = rgba(geom),
                .radius = static_cast<float>(geom.getRadius()),
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
            PolygonalMesh const& mesh = m.getMesh();

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

            out.push_back(osim::Mesh{
                .transform = transform(m),
                .scale = scale_factors(m),
                .rgba = rgba(m),
                .triangles = std::move(triangles),
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

std::vector<osim::Geometry> osim::geometry_in(std::string_view path) {
    Model model{std::string{path}};
    model.finalizeFromProperties();
    model.finalizeConnections();

    // Configure the model.

    model.buildSystem();
    State& state = model.initSystem();
    model.initializeState();
    model.updMatterSubsystem().setShowDefaultGeometry(false);

    DynamicDecorationGenerator dg{&model};
    Array_<DecorativeGeometry> tmp;
    dg.generateDecorations(state, tmp);

    auto rv = std::vector<osim::Geometry>{};
    auto visitor = Geometry_visitor{model, state, rv};
    for (DecorativeGeometry& dg : tmp) {
        dg.implementGeometry(visitor);
    }

    return rv;
}
