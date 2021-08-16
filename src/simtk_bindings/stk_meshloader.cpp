#include "stk_meshloader.hpp"

#include "src/3d/model.hpp"
#include "src/simtk_bindings/stk_converters.hpp"

#include <SimTKcommon.h>

using namespace osc;

static glm::vec3 get_face_vert(SimTK::PolygonalMesh const& mesh, int face, int vert) noexcept {
    int vertidx = mesh.getFaceVertex(face, vert);
    SimTK::Vec3 const& data = mesh.getVertexPosition(vertidx);
    return stk_vec3_from_Vec3(data);
}

// public API

NewMesh osc::stk_load_mesh(std::filesystem::path const& p) {
    SimTK::DecorativeMeshFile dmf{p.string()};
    SimTK::PolygonalMesh const& mesh = dmf.getMesh();

    NewMesh rv;
    rv.reserve(static_cast<size_t>(mesh.getNumVertices()));

    unsigned short index = 0;
    auto push = [&rv, &index](glm::vec3 const& pos, glm::vec3 const& normal) {
        rv.verts.push_back(pos);
        rv.normals.push_back(normal);
        rv.indices.push_back(index++);
    };

    for (int face = 0, nfaces = mesh.getNumFaces(); face < nfaces; ++face) {
        int verts = mesh.getNumVerticesForFace(face);

        if (verts < 3) {
            // line/point

            // ignore

        } else if (verts == 3) {
            // triangle

            glm::vec3 vs[] = {
                get_face_vert(mesh, face, 0),
                get_face_vert(mesh, face, 1),
                get_face_vert(mesh, face, 2),
            };
            glm::vec3 normal = triangle_normal(vs);

            push(vs[0], normal);
            push(vs[1], normal);
            push(vs[2], normal);

        } else if (verts == 4) {
            // quad: render as two triangles

            glm::vec3 vs[] = {
                get_face_vert(mesh, face, 0),
                get_face_vert(mesh, face, 1),
                get_face_vert(mesh, face, 2),
                get_face_vert(mesh, face, 3),
            };

            glm::vec3 norms[] = {
                triangle_normal(vs[0], vs[1], vs[2]),
                triangle_normal(vs[2], vs[3], vs[0]),
            };

            push(vs[0], norms[0]);
            push(vs[1], norms[0]);
            push(vs[2], norms[0]);
            push(vs[2], norms[1]);
            push(vs[3], norms[1]);
            push(vs[0], norms[1]);

        } else {
            // polygon (>3 edges):
            //
            // create a vertex at the average center point and attach
            // every two verices to the center as triangles.

            glm::vec3 center = {0.0f, 0.0f, 0.0f};
            for (int vert = 0; vert < verts; ++vert) {
                center += get_face_vert(mesh, face, vert);
            }
            center /= verts;

            for (int vert = 0; vert < verts - 1; ++vert) {

                glm::vec3 vs[] = {
                    get_face_vert(mesh, face, vert),
                    get_face_vert(mesh, face, vert + 1),
                    center,
                };
                glm::vec3 normal = triangle_normal(vs);

                push(vs[0], normal);
                push(vs[1], normal);
                push(vs[2], normal);
            }

            // complete the polygon loop
            glm::vec3 vs[] = {
                get_face_vert(mesh, face, verts - 1),
                get_face_vert(mesh, face, 0),
                center,
            };
            glm::vec3 normal = triangle_normal(vs);

            push(vs[0], normal);
            push(vs[1], normal);
            push(vs[2], normal);
        }
    }


    return rv;
}
