#include "geometry_generator.hpp"

#include "src/log.hpp"
#include "src/3d/3d.hpp"

#include <simbody/internal/SimbodyMatterSubsystem.h>

#include <iosfwd>


static glm::mat4 stk_xform_to_mat4(SimTK::Transform const& t) {
    // glm::mat4 is column major:
    //     see: https://glm.g-truc.net/0.9.2/api/a00001.html
    //     (and just Google "glm column major?")
    //
    // SimTK is row-major, carefully read the sourcecode for
    // `SimTK::Transform`.

    glm::mat4 m;

    // x0 y0 z0 w0
    SimTK::Rotation const& r = t.R();
    SimTK::Vec3 const& p = t.p();

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

static glm::vec3 scale_factors(SimTK::DecorativeGeometry const& geom) {
    SimTK::Vec3 sf = geom.getScaleFactors();
    for (int i = 0; i < 3; ++i) {
        sf[i] = sf[i] <= 0 ? 1.0 : sf[i];
    }
    return glm::vec3{sf[0], sf[1], sf[2]};
}

static glm::vec4 extract_rgba(SimTK::DecorativeGeometry const& geom) {
    SimTK::Vec3 const& rgb = geom.getColor();
    SimTK::Real ar = geom.getOpacity();
    ar = ar < 0.0 ? 1.0 : ar;
    return glm::vec4(rgb[0], rgb[1], rgb[2], ar);
}

static glm::vec3 to_vec3(SimTK::Vec3 const& v) {
    return glm::vec3(v[0], v[1], v[2]);
}

static glm::vec4 to_vec4(SimTK::Vec3 const& v, float w = 1.0f) {
    return glm::vec4{v[0], v[1], v[2], w};
}

static glm::mat4 geom_xform(SimTK::SimbodyMatterSubsystem const& matter, SimTK::State const& state, SimTK::DecorativeGeometry const& g) {
    SimTK::MobilizedBody const& mobod = matter.getMobilizedBody(SimTK::MobilizedBodyIndex(g.getBodyId()));
    glm::mat4 ground2body = stk_xform_to_mat4(mobod.getBodyTransform(state));
    glm::mat4 body2decoration = stk_xform_to_mat4(g.getTransform());
    return ground2body * body2decoration;
}


// public API

std::ostream& osc::operator<<(std::ostream& o, Simbody_geometry::Sphere const& s) {
    return o << "Sphere(rgba = " << s.rgba << ", pos = " << s.pos << ", radius = " << s.radius;
}

std::ostream& osc::operator<<(std::ostream& o, Simbody_geometry::Line const& l) {
    return o << "Line(rgba = " << l.rgba << ", p1 = " << l.p1 << ", p2 = " << l.p2 << ')';
}

std::ostream& osc::operator<<(std::ostream& o, Simbody_geometry::Cylinder const& c) {
    return o << "Cylinder(rgba = " << c.rgba << ", model_matrix = " << c.model_mtx << ')';
}

std::ostream& osc::operator<<(std::ostream& o, Simbody_geometry::Brick const& b) {
    return o << "Brick(rgba = " << b.rgba << ", model_matrix = " << b.model_mtx << ')';
}

std::ostream& osc::operator<<(std::ostream& o, Simbody_geometry::MeshFile const& m) {
    return o << "MeshFile(path = " << *m.path << ", rgba = " << m.rgba << ", model_matrix = " << m.model_mtx << ')';
}

std::ostream& osc::operator<<(std::ostream& o, Simbody_geometry::Frame const& f) {
    return o << "Frame(pos = " << f.pos << ", axis_scales = " << f.axis_lengths << ", rotation = " << f.rotation << ')';
}

std::ostream& osc::operator<<(std::ostream& o, Simbody_geometry::Ellipsoid const& c) {
    return o << "Ellipsoid(rgba = " << c.rgba << ", model_mtx = " << c.model_mtx << ')';
}

std::ostream& osc::operator<<(std::ostream& o, Simbody_geometry::Cone const& c) {
    return o << "Cone(rgba = " << c.rgba << ", pos = " << c.pos << ", height = " << c.height << ", direction = " << c.direction << ", base_radius = " << c.base_radius;
}

std::ostream& osc::operator<<(std::ostream& o, Simbody_geometry::Arrow const& a) {
    return o << "Arrow(rgba = " << a.rgba << ", p1 = " << a.p1 << ", p2 = " << a.p2 << ')';
}

std::ostream& osc::operator<<(std::ostream& o, Simbody_geometry const& g) {
    switch (g.geom_type) {
    case Simbody_geometry::Type::Sphere:
        return o << g.sphere;
    case Simbody_geometry::Type::Line:
        return o << g.line;
    case Simbody_geometry::Type::Cylinder:
        return o << g.cylinder;
    case Simbody_geometry::Type::Brick:
        return o << g.brick;
    case Simbody_geometry::Type::MeshFile:
        return o << g.meshfile;
    case Simbody_geometry::Type::Frame:
        return o << g.frame;
    case Simbody_geometry::Type::Ellipsoid:
        return o << g.ellipsoid;
    case Simbody_geometry::Type::Cone:
        return o << g.cone;
    case Simbody_geometry::Type::Arrow:
        return o << g.arrow;
    default:
        return o;
    }
}

osc::Geometry_generator::Geometry_generator(SimTK::SimbodyMatterSubsystem const& matter_, SimTK::State const& st_) :
    m_Matter{matter_},
    m_St{st_} {
}

void osc::Geometry_generator::implementPointGeometry(SimTK::DecorativePoint const&) {
    static bool shown_nyi_warning = []() {
        log::warn("this model uses implementPointGeometry, which is not yet implemented in OSC");
        return true;
    }();
    (void)shown_nyi_warning;
}

void osc::Geometry_generator::implementLineGeometry(SimTK::DecorativeLine const& dl) {
    glm::mat4 m = geom_xform(m_Matter, m_St, dl);

    Simbody_geometry g;
    g.geom_type = Simbody_geometry::Type::Line;
    g.line.rgba = extract_rgba(dl);
    g.line.p1 = glm::vec3{m * to_vec4(dl.getPoint1())};
    g.line.p2 = glm::vec3{m * to_vec4(dl.getPoint2())};

    on_emit(g);
}

void osc::Geometry_generator::implementBrickGeometry(SimTK::DecorativeBrick const& db) {
    glm::vec3 halfdims = to_vec3(db.getHalfLengths());

    Simbody_geometry g;
    g.geom_type = Simbody_geometry::Type::Brick;
    g.brick.rgba = extract_rgba(db);
    g.brick.model_mtx = glm::scale(geom_xform(m_Matter, m_St, db), halfdims);

    on_emit(g);
}

void osc::Geometry_generator::implementCylinderGeometry(SimTK::DecorativeCylinder const& dc) {
    glm::vec3 s = scale_factors(dc);
    s.x *= static_cast<float>(dc.getRadius());
    s.y *= static_cast<float>(dc.getHalfHeight());
    s.z *= static_cast<float>(dc.getRadius());

    Simbody_geometry g;
    g.geom_type = Simbody_geometry::Type::Cylinder;
    g.cylinder.rgba = extract_rgba(dc);
    g.cylinder.model_mtx = glm::scale(geom_xform(m_Matter, m_St, dc), s);

    on_emit(g);
}

void osc::Geometry_generator::implementCircleGeometry(SimTK::DecorativeCircle const&) {
    static bool shown_nyi_warning = []() {
        log::warn("this model uses implementCircleGeometry, which is not yet implemented in OSC");
        return true;
    }();
    (void)shown_nyi_warning;
}

void osc::Geometry_generator::implementSphereGeometry(SimTK::DecorativeSphere const& ds) {
    glm::mat4 xform = geom_xform(m_Matter, m_St, ds);
    // scale factors are ignored, sorry not sorry, etc.

    Simbody_geometry g;
    g.geom_type = Simbody_geometry::Type::Sphere;
    g.sphere.rgba = extract_rgba(ds);
    g.sphere.radius = static_cast<float>(ds.getRadius());
    g.sphere.pos = {xform[3][0], xform[3][1], xform[3][2]};

    on_emit(g);
}

void osc::Geometry_generator::implementEllipsoidGeometry(SimTK::DecorativeEllipsoid const& de) {
    glm::mat4 xform = geom_xform(m_Matter, m_St, de);
    glm::vec3 sfs = scale_factors(de);
    glm::vec3 radii = to_vec3(de.getRadii());

    Simbody_geometry g;
    g.geom_type = Simbody_geometry::Type::Ellipsoid;
    g.ellipsoid.rgba = extract_rgba(de);
    g.ellipsoid.model_mtx = glm::scale(xform, sfs * radii);

    on_emit(g);
}

void osc::Geometry_generator::implementFrameGeometry(SimTK::DecorativeFrame const& df) {
    glm::mat4 raw_xform = geom_xform(m_Matter, m_St, df);

    glm::vec3 pos{raw_xform[3]};
    glm::mat3 rotation_mtx{raw_xform};

    glm::vec3 scales = scale_factors(df) * static_cast<float>(df.getAxisLength());

    Simbody_geometry g;
    g.geom_type = Simbody_geometry::Type::Frame;
    g.frame.pos = pos;
    g.frame.axis_lengths = scales;
    g.frame.rotation = rotation_mtx;

    on_emit(g);
}

void osc::Geometry_generator::implementTextGeometry(SimTK::DecorativeText const&) {
    static bool shown_nyi_warning = []() {
        log::warn("this model uses implementTextGeometry, which is not yet implemented in OSC");
        return true;
    }();
    (void)shown_nyi_warning;
}

void osc::Geometry_generator::implementMeshGeometry(SimTK::DecorativeMesh const&) {
    static bool shown_nyi_warning = []() {
        log::warn("this model uses implementMeshGeometry, which is not yet implemented in OSC");
        return true;
    }();
    (void)shown_nyi_warning;
}

void osc::Geometry_generator::implementMeshFileGeometry(SimTK::DecorativeMeshFile const& dmf) {
    Simbody_geometry g;
    g.geom_type = Simbody_geometry::Type::MeshFile;
    g.meshfile.path = &dmf.getMeshFile();
    g.meshfile.model_mtx = glm::scale(geom_xform(m_Matter, m_St, dmf), scale_factors(dmf));
    g.meshfile.rgba = extract_rgba(dmf);

    on_emit(g);
}

void osc::Geometry_generator::implementArrowGeometry(SimTK::DecorativeArrow const& da) {
    glm::mat4 xform = glm::scale(geom_xform(m_Matter, m_St, da), scale_factors(da));

    glm::vec3 base_startpoint = to_vec3(da.getStartPoint());
    glm::vec3 base_endpoint = to_vec3(da.getEndPoint());
    glm::vec3 startpoint = glm::vec3{xform * glm::vec4{base_startpoint, 1.0f}};
    glm::vec3 endpoint = glm::vec3{xform * glm::vec4{base_endpoint, 1.0f}};

    Simbody_geometry g;
    g.geom_type = Simbody_geometry::Type::Arrow;
    g.arrow.rgba = extract_rgba(da);
    g.arrow.p1 = startpoint;
    g.arrow.p2 = endpoint;

    on_emit(g);
}

void osc::Geometry_generator::implementTorusGeometry(SimTK::DecorativeTorus const&) {
    static bool shown_nyi_warning = []() {
        log::warn("this model uses implementTorusGeometry, which is not yet implemented in OSC");
        return true;
    }();
    (void)shown_nyi_warning;
}

void osc::Geometry_generator::implementConeGeometry(SimTK::DecorativeCone const& dc) {
    glm::mat4 xform = glm::scale(geom_xform(m_Matter, m_St, dc), scale_factors(dc));

    glm::vec3 base_pos = to_vec3(dc.getOrigin());
    glm::vec3 base_dir = to_vec3(dc.getDirection());

    glm::vec3 worldpos = glm::vec3{xform * glm::vec4{base_pos, 1.0f}};
    glm::vec3 worlddir = glm::normalize(glm::vec3{xform * glm::vec4{base_dir, 0.0f}});

    Simbody_geometry g;
    g.geom_type = Simbody_geometry::Type::Cone;
    g.cone.rgba = extract_rgba(dc);
    g.cone.pos = worldpos;
    g.cone.direction = worlddir;
    g.cone.base_radius = static_cast<float>(dc.getBaseRadius());
    g.cone.height = static_cast<float>(dc.getHeight());

    on_emit(g);
}
