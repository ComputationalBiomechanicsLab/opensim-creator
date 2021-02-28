#include "simbody_geometry_visitor.hpp"

#include "src/3d/gpu_cache.hpp"
#include "src/3d/gpu_data_reference.hpp"
#include "src/3d/mesh.hpp"
#include "src/3d/mesh_instance.hpp"
#include "src/3d/untextured_vert.hpp"
#include "src/constants.hpp"

#include <SimTKcommon/Orientation.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <simbody/internal/MobilizedBody.h>
#include <simbody/internal/SimbodyMatterSubsystem.h>

#include <algorithm>
#include <limits>

using namespace SimTK;
using namespace osmv;

// create an xform that transforms the unit cylinder into a line between
// two points
static glm::mat4 cylinder_to_line_xform(float line_width, glm::vec3 const& p1, glm::vec3 const& p2) {
    glm::vec3 p1_to_p2 = p2 - p1;
    glm::vec3 c1_to_c2 = glm::vec3{0.0f, 2.0f, 0.0f};
    auto rotation = glm::rotate(
        glm::identity<glm::mat4>(),
        glm::acos(glm::dot(glm::normalize(c1_to_c2), glm::normalize(p1_to_p2))),
        glm::cross(glm::normalize(c1_to_c2), glm::normalize(p1_to_p2)));
    float scale = glm::length(p1_to_p2) / glm::length(c1_to_c2);
    auto scale_xform = glm::scale(glm::identity<glm::mat4>(), glm::vec3{line_width, scale, line_width});
    auto translation = glm::translate(glm::identity<glm::mat4>(), p1 + p1_to_p2 / 2.0f);
    return translation * rotation * scale_xform;
}

// load a SimTK::PolygonalMesh into an osmv::Untextured_vert mesh ready for GPU upload
static void load_mesh_data(PolygonalMesh const& mesh, Plain_mesh& out) {

    // helper function: gets a vertex for a face
    auto get_face_vert_pos = [&](int face, int vert) {
        SimTK::Vec3 pos = mesh.getVertexPosition(mesh.getFaceVertex(face, vert));
        return glm::vec3{pos[0], pos[1], pos[2]};
    };

    // helper function: compute the normal of the triangle p1, p2, p3
    auto make_normal = [](glm::vec3 const& p1, glm::vec3 const& p2, glm::vec3 const& p3) {
        return glm::normalize(glm::cross(p2 - p1, p3 - p1));
    };

    out.clear();
    std::vector<Untextured_vert>& triangles = out.vert_data;

    // iterate over each face in the PolygonalMesh and transform each into a sequence of
    // GPU-friendly triangle verts
    for (auto face = 0; face < mesh.getNumFaces(); ++face) {
        auto num_vertices = mesh.getNumVerticesForFace(face);

        if (num_vertices < 3) {
            // line?: ignore for now
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

            for (int vert = 0; vert < num_vertices - 1; ++vert) {
                glm::vec3 p1 = get_face_vert_pos(face, vert);
                glm::vec3 p2 = get_face_vert_pos(face, vert + 1);
                glm::vec3 normal = make_normal(p1, p2, center);

                triangles.push_back({p1, normal});
                triangles.push_back({p2, normal});
                triangles.push_back({center, normal});
            }

            // complete the polygon loop
            glm::vec3 p1 = get_face_vert_pos(face, num_vertices - 1);
            glm::vec3 p2 = get_face_vert_pos(face, 0);
            glm::vec3 normal = make_normal(p1, p2, center);

            triangles.push_back({p1, normal});
            triangles.push_back({p2, normal});
            triangles.push_back({center, normal});
        }
    }

    out = Plain_mesh::by_deduping(std::move(triangles));
}

static Transform ground_to_decoration_xform(
    SimbodyMatterSubsystem const& ms, SimTK::State const& state, DecorativeGeometry const& geom) {
    MobilizedBody const& mobod = ms.getMobilizedBody(MobilizedBodyIndex(geom.getBodyId()));
    Transform const& ground_to_body_xform = mobod.getBodyTransform(state);
    Transform const& body_to_decoration_xform = geom.getTransform();

    return ground_to_body_xform * body_to_decoration_xform;
}

static glm::mat4 to_mat4(Transform t) {
    // glm::mat4 is column major:
    //     see: https://glm.g-truc.net/0.9.2/api/a00001.html
    //     (and just Google "glm column major?")
    //
    // SimTK is whoknowswtf-major (actually, row), carefully read the
    // sourcecode for `SimTK::Transform`.

    glm::mat4 m;

    // x0 y0 z0 w0
    Rotation const& r = t.R();
    Vec3 const& p = t.p();

    {
        auto const& row0 = r[0];
        m[0][0] = static_cast<float>(row0[0]);
        m[1][0] = static_cast<float>(row0[1]);
        m[2][0] = static_cast<float>(row0[2]);
        m[3][0] = static_cast<float>(p[0]);
    }

    {
        auto const& row1 = r[1];
        m[0][1] = static_cast<float>(row1[0]);
        m[1][1] = static_cast<float>(row1[1]);
        m[2][1] = static_cast<float>(row1[2]);
        m[3][1] = static_cast<float>(p[1]);
    }

    {
        auto const& row2 = r[2];
        m[0][2] = static_cast<float>(row2[0]);
        m[1][2] = static_cast<float>(row2[1]);
        m[2][2] = static_cast<float>(row2[2]);
        m[3][2] = static_cast<float>(p[2]);
    }

    m[0][3] = 0.0f;
    m[1][3] = 0.0f;
    m[2][3] = 0.0f;
    m[3][3] = 1.0f;

    return m;
}

static glm::mat4 to_mat4(SimbodyMatterSubsystem const& ms, SimTK::State const& state, DecorativeGeometry const& geom) {
    return to_mat4(ground_to_decoration_xform(ms, state, geom));
}

static glm::vec3 scale_factors(DecorativeGeometry const& geom) {
    Vec3 sf = geom.getScaleFactors();
    for (int i = 0; i < 3; ++i) {
        sf[i] = sf[i] <= 0 ? 1.0 : sf[i];
    }
    return glm::vec3{sf[0], sf[1], sf[2]};
}

static Rgba32 extract_rgba(DecorativeGeometry const& geom) {
    Vec3 const& rgb = geom.getColor();
    Real ar = geom.getOpacity();
    ar = ar < 0.0 ? 1.0 : ar;

    Rgba32 rv;
    int r = std::min(255, static_cast<int>(255.0 * rgb[0]));
    int g = std::min(255, static_cast<int>(255.0 * rgb[1]));
    int b = std::min(255, static_cast<int>(255.0 * rgb[2]));
    int a = std::min(255, static_cast<int>(255.0 * ar));
    rv.r = static_cast<unsigned char>(r);
    rv.g = static_cast<unsigned char>(g);
    rv.b = static_cast<unsigned char>(b);
    rv.a = static_cast<unsigned char>(a);

    return rv;
}

static glm::vec4 to_vec4(Vec3 const& v, float w = 1.0f) {
    return glm::vec4{v[0], v[1], v[2], w};
}

void Simbody_geometry_visitor::implementPointGeometry(SimTK::DecorativePoint const&) {
    // nyi
}

void Simbody_geometry_visitor::implementLineGeometry(SimTK::DecorativeLine const& geom) {
    glm::mat4 xform = to_mat4(matter_subsys, state, geom);
    glm::vec3 p1 = xform * to_vec4(geom.getPoint1());
    glm::vec3 p2 = xform * to_vec4(geom.getPoint2());
    glm::mat4 cylinder_xform = cylinder_to_line_xform(0.005f, p1, p2);

    emplace_instance(cylinder_xform, extract_rgba(geom), gpu_cache.simbody_cylinder);
}

void Simbody_geometry_visitor::implementBrickGeometry(SimTK::DecorativeBrick const& geom) {
    SimTK::Vec3 dims = geom.getHalfLengths();
    glm::mat4 base_xform = to_mat4(matter_subsys, state, geom);
    glm::mat4 xform = glm::scale(base_xform, glm::vec3{dims[0], dims[1], dims[2]});

    emplace_instance(xform, extract_rgba(geom), gpu_cache.simbody_cube);
}

void Simbody_geometry_visitor::implementCylinderGeometry(SimTK::DecorativeCylinder const& geom) {
    glm::mat4 m = to_mat4(matter_subsys, state, geom);
    glm::vec3 s = scale_factors(geom);
    s.x *= static_cast<float>(geom.getRadius());
    s.y *= static_cast<float>(geom.getHalfHeight());
    s.z *= static_cast<float>(geom.getRadius());

    emplace_instance(glm::scale(m, s), extract_rgba(geom), gpu_cache.simbody_cylinder);
}

void Simbody_geometry_visitor::implementCircleGeometry(SimTK::DecorativeCircle const&) {
    // nyi
}

void Simbody_geometry_visitor::implementSphereGeometry(SimTK::DecorativeSphere const& geom) {
    float r = static_cast<float>(geom.getRadius());
    glm::mat4 xform = glm::scale(to_mat4(matter_subsys, state, geom), glm::vec3{r, r, r});

    emplace_instance(xform, extract_rgba(geom), gpu_cache.simbody_sphere);
}

void Simbody_geometry_visitor::implementEllipsoidGeometry(SimTK::DecorativeEllipsoid const&) {
    // nyi
}

void Simbody_geometry_visitor::implementFrameGeometry(SimTK::DecorativeFrame const& geom) {
    glm::mat4 xform = to_mat4(matter_subsys, state, geom);

    glm::mat4 scaler = [&geom]() {
        glm::vec3 s = scale_factors(geom);
        s *= static_cast<float>(geom.getAxisLength());
        return glm::scale(glm::identity<glm::mat4>(), glm::vec3{0.015f * s.x, 0.1f * s.y, 0.015f * s.z});
    }();

    glm::mat4 mover = glm::translate(glm::identity<glm::mat4>(), glm::vec3{0.0f, 1.0f, 0.0f});

    // origin
    emplace_instance(
        glm::scale(xform, glm::vec3{0.0075f}), glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, gpu_cache.simbody_sphere);

    // axis Y
    emplace_instance(xform * scaler * mover, glm::vec4{0.0f, 0.75f, 0.0f, 1.0f}, gpu_cache.simbody_cylinder);

    // X
    glm::mat4 rotate_plusy_to_plusx =
        glm::rotate(glm::identity<glm::mat4>(), pi_f / 2.0f, glm::vec3{0.0f, 0.0f, -1.0f});
    emplace_instance(
        xform * rotate_plusy_to_plusx * scaler * mover, glm::vec4{0.75f, 0.0f, 0.0f, 1.0f}, gpu_cache.simbody_cylinder);

    // Z
    glm::mat4 rotate_plusy_to_plusz = glm::rotate(glm::identity<glm::mat4>(), pi_f / 2.0f, glm::vec3{1.0f, 0.0f, 0.0f});
    emplace_instance(
        xform * rotate_plusy_to_plusz * scaler * mover, glm::vec4{0.0f, 0.0f, 0.75f, 1.0f}, gpu_cache.simbody_cylinder);
}

void Simbody_geometry_visitor::implementTextGeometry(SimTK::DecorativeText const&) {
    // nyi
}

void Simbody_geometry_visitor::implementMeshGeometry(SimTK::DecorativeMesh const&) {
    // nyi
}

void Simbody_geometry_visitor::implementMeshFileGeometry(SimTK::DecorativeMeshFile const& geom) {
    Mesh_reference ref =
        gpu_cache.lookup_or_construct_mesh(geom.getMeshFile(), [&geom, &mesh_swap = this->mesh_swap]() {
            load_mesh_data(geom.getMesh(), mesh_swap);
            return mesh_swap;
        });

    emplace_instance(glm::scale(to_mat4(matter_subsys, state, geom), scale_factors(geom)), extract_rgba(geom), ref);
}

void Simbody_geometry_visitor::implementArrowGeometry(SimTK::DecorativeArrow const&) {
    // nyi
}

void Simbody_geometry_visitor::implementTorusGeometry(SimTK::DecorativeTorus const&) {
    // nyi
}

void Simbody_geometry_visitor::implementConeGeometry(SimTK::DecorativeCone const&) {
    // nyi
}
