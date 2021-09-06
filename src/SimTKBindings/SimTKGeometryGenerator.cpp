#include "SimTKGeometryGenerator.hpp"

#include "src/Log.hpp"
#include "src/3D/Model.hpp"
#include "src/SimTKBindings/SimTKConverters.hpp"

#include <simbody/internal/SimbodyMatterSubsystem.h>
#include <glm/gtc/matrix_transform.hpp>

#include <iosfwd>

using namespace osc;


// extract scale factors from geometry
static glm::vec3 scaleFactors(SimTK::DecorativeGeometry const& geom) {
    SimTK::Vec3 sf = geom.getScaleFactors();
    for (int i = 0; i < 3; ++i) {
        sf[i] = sf[i] <= 0 ? 1.0 : sf[i];
    }
    return glm::vec3{sf[0], sf[1], sf[2]};
}
// extract RGBA from geometry
static glm::vec4 extractRGBA(SimTK::DecorativeGeometry const& geom) {
    SimTK::Vec3 const& rgb = geom.getColor();
    SimTK::Real ar = geom.getOpacity();
    ar = ar < 0.0 ? 1.0 : ar;
    return glm::vec4(rgb[0], rgb[1], rgb[2], ar);
}

// get modelspace to worldspace xform for a given decorative element
static glm::mat4 geomXform(SimTK::SimbodyMatterSubsystem const& matter, SimTK::State const& state, SimTK::DecorativeGeometry const& g) {
    SimTK::MobilizedBody const& mobod = matter.getMobilizedBody(SimTK::MobilizedBodyIndex(g.getBodyId()));
    glm::mat4 ground2body = SimTKMat4x4FromTransform(mobod.getBodyTransform(state));
    glm::mat4 body2decoration = SimTKMat4x4FromTransform(g.getTransform());
    return ground2body * body2decoration;
}


// public API

std::ostream& osc::operator<<(std::ostream& o, SimbodyGeometry::Sphere const& s) {
    return o << "Sphere(rgba = " << s.rgba << ", pos = " << s.pos << ", radius = " << s.radius;
}

std::ostream& osc::operator<<(std::ostream& o, SimbodyGeometry::Line const& l) {
    return o << "Line(rgba = " << l.rgba << ", p1 = " << l.p1 << ", p2 = " << l.p2 << ')';
}

std::ostream& osc::operator<<(std::ostream& o, SimbodyGeometry::Cylinder const& c) {
    return o << "Cylinder(rgba = " << c.rgba << ", modelMtx = " << c.modelMtx << ')';
}

std::ostream& osc::operator<<(std::ostream& o, SimbodyGeometry::Brick const& b) {
    return o << "Brick(rgba = " << b.rgba << ", modelMtx = " << b.modelMtx << ')';
}

std::ostream& osc::operator<<(std::ostream& o, SimbodyGeometry::MeshFile const& m) {
    return o << "MeshFile(path = " << *m.path << ", rgba = " << m.rgba << ", modelMtx = " << m.modelMtx << ')';
}

std::ostream& osc::operator<<(std::ostream& o, SimbodyGeometry::Frame const& f) {
    return o << "Frame(pos = " << f.pos << ", axisLengths = " << f.axisLengths << ", rotation = " << f.rotation << ')';
}

std::ostream& osc::operator<<(std::ostream& o, SimbodyGeometry::Ellipsoid const& c) {
    return o << "Ellipsoid(rgba = " << c.rgba << ", modelMtx = " << c.modelMtx << ')';
}

std::ostream& osc::operator<<(std::ostream& o, SimbodyGeometry::Cone const& c) {
    return o << "Cone(rgba = " << c.rgba << ", pos = " << c.pos << ", height = " << c.height << ", direction = " << c.direction << ", baseRadius = " << c.baseRadius;
}

std::ostream& osc::operator<<(std::ostream& o, SimbodyGeometry::Arrow const& a) {
    return o << "Arrow(rgba = " << a.rgba << ", p1 = " << a.p1 << ", p2 = " << a.p2 << ')';
}

std::ostream& osc::operator<<(std::ostream& o, SimbodyGeometry const& g) {
    switch (g.geometryType) {
    case SimbodyGeometry::Type::Sphere:
        return o << g.sphere;
    case SimbodyGeometry::Type::Line:
        return o << g.line;
    case SimbodyGeometry::Type::Cylinder:
        return o << g.cylinder;
    case SimbodyGeometry::Type::Brick:
        return o << g.brick;
    case SimbodyGeometry::Type::MeshFile:
        return o << g.meshfile;
    case SimbodyGeometry::Type::Frame:
        return o << g.frame;
    case SimbodyGeometry::Type::Ellipsoid:
        return o << g.ellipsoid;
    case SimbodyGeometry::Type::Cone:
        return o << g.cone;
    case SimbodyGeometry::Type::Arrow:
        return o << g.arrow;
    default:
        return o;
    }
}

osc::GeometryGenerator::GeometryGenerator(SimTK::SimbodyMatterSubsystem const& matter, SimTK::State const& st) :
    m_Matter{matter},
    m_St{st} {
}

void osc::GeometryGenerator::implementPointGeometry(SimTK::DecorativePoint const&) {
    static bool shown_once = []() {
        log::warn("this model uses implementPointGeometry, which is not yet implemented in OSC");
        return true;
    }();
    (void)shown_once;
}

void osc::GeometryGenerator::implementLineGeometry(SimTK::DecorativeLine const& dl) {
    glm::mat4 m = geomXform(m_Matter, m_St, dl);

    SimbodyGeometry g;
    g.geometryType = SimbodyGeometry::Type::Line;
    g.line.rgba = extractRGBA(dl);
    g.line.p1 = glm::vec3{m * SimTKVec4FromVec3(dl.getPoint1())};
    g.line.p2 = glm::vec3{m * SimTKVec4FromVec3(dl.getPoint2())};

    onGeometryEmission(g);
}

void osc::GeometryGenerator::implementBrickGeometry(SimTK::DecorativeBrick const& db) {
    glm::vec3 halfdims = SimTKVec3FromVec3(db.getHalfLengths());

    SimbodyGeometry g;
    g.geometryType = SimbodyGeometry::Type::Brick;
    g.brick.rgba = extractRGBA(db);
    g.brick.modelMtx = glm::scale(geomXform(m_Matter, m_St, db), halfdims);

    onGeometryEmission(g);
}

void osc::GeometryGenerator::implementCylinderGeometry(SimTK::DecorativeCylinder const& dc) {
    glm::vec3 s = scaleFactors(dc);
    s.x *= static_cast<float>(dc.getRadius());
    s.y *= static_cast<float>(dc.getHalfHeight());
    s.z *= static_cast<float>(dc.getRadius());

    SimbodyGeometry g;
    g.geometryType = SimbodyGeometry::Type::Cylinder;
    g.cylinder.rgba = extractRGBA(dc);
    g.cylinder.modelMtx = glm::scale(geomXform(m_Matter, m_St, dc), s);

    onGeometryEmission(g);
}

void osc::GeometryGenerator::implementCircleGeometry(SimTK::DecorativeCircle const&) {
    static bool shownWarning = []() {
        log::warn("this model uses implementCircleGeometry, which is not yet implemented in OSC");
        return true;
    }();
    (void)shownWarning;
}

void osc::GeometryGenerator::implementSphereGeometry(SimTK::DecorativeSphere const& ds) {
    glm::mat4 xform = geomXform(m_Matter, m_St, ds);
    // scale factors are ignored, sorry not sorry, etc.

    SimbodyGeometry g;
    g.geometryType = SimbodyGeometry::Type::Sphere;
    g.sphere.rgba = extractRGBA(ds);
    g.sphere.radius = static_cast<float>(ds.getRadius());
    g.sphere.pos = {xform[3][0], xform[3][1], xform[3][2]};

    onGeometryEmission(g);
}

void osc::GeometryGenerator::implementEllipsoidGeometry(SimTK::DecorativeEllipsoid const& de) {
    glm::mat4 xform = geomXform(m_Matter, m_St, de);
    glm::vec3 sfs = scaleFactors(de);
    glm::vec3 radii = SimTKVec3FromVec3(de.getRadii());

    SimbodyGeometry g;
    g.geometryType = SimbodyGeometry::Type::Ellipsoid;
    g.ellipsoid.rgba = extractRGBA(de);
    g.ellipsoid.modelMtx = glm::scale(xform, sfs * radii);

    onGeometryEmission(g);
}

void osc::GeometryGenerator::implementFrameGeometry(SimTK::DecorativeFrame const& df) {
    glm::mat4 rawXform = geomXform(m_Matter, m_St, df);

    glm::vec3 pos{rawXform[3]};
    glm::mat3 rotation_mtx{rawXform};

    glm::vec3 scales = scaleFactors(df) * static_cast<float>(df.getAxisLength());

    SimbodyGeometry g;
    g.geometryType = SimbodyGeometry::Type::Frame;
    g.frame.pos = pos;
    g.frame.axisLengths = scales;
    g.frame.rotation = rotation_mtx;

    onGeometryEmission(g);
}

void osc::GeometryGenerator::implementTextGeometry(SimTK::DecorativeText const&) {
    static bool shownWarning = []() {
        log::warn("this model uses implementTextGeometry, which is not yet implemented in OSC");
        return true;
    }();
    (void)shownWarning;
}

void osc::GeometryGenerator::implementMeshGeometry(SimTK::DecorativeMesh const&) {
    static bool shownWarning = []() {
        log::warn("this model uses implementMeshGeometry, which is not yet implemented in OSC");
        return true;
    }();
    (void)shownWarning;
}

void osc::GeometryGenerator::implementMeshFileGeometry(SimTK::DecorativeMeshFile const& dmf) {
    SimbodyGeometry g;
    g.geometryType = SimbodyGeometry::Type::MeshFile;
    g.meshfile.path = &dmf.getMeshFile();
    g.meshfile.modelMtx = glm::scale(geomXform(m_Matter, m_St, dmf), scaleFactors(dmf));
    g.meshfile.rgba = extractRGBA(dmf);

    onGeometryEmission(g);
}

void osc::GeometryGenerator::implementArrowGeometry(SimTK::DecorativeArrow const& da) {
    glm::mat4 xform = glm::scale(geomXform(m_Matter, m_St, da), scaleFactors(da));

    glm::vec3 baseStartpoint = SimTKVec3FromVec3(da.getStartPoint());
    glm::vec3 baseEndpoint = SimTKVec3FromVec3(da.getEndPoint());
    glm::vec3 startPoint = glm::vec3{xform * glm::vec4{baseStartpoint, 1.0f}};
    glm::vec3 endPoint = glm::vec3{xform * glm::vec4{baseEndpoint, 1.0f}};

    SimbodyGeometry g;
    g.geometryType = SimbodyGeometry::Type::Arrow;
    g.arrow.rgba = extractRGBA(da);
    g.arrow.p1 = startPoint;
    g.arrow.p2 = endPoint;

    onGeometryEmission(g);
}

void osc::GeometryGenerator::implementTorusGeometry(SimTK::DecorativeTorus const&) {
    static bool shownWarning = []() {
        log::warn("this model uses implementTorusGeometry, which is not yet implemented in OSC");
        return true;
    }();
    (void)shownWarning;
}

void osc::GeometryGenerator::implementConeGeometry(SimTK::DecorativeCone const& dc) {
    glm::mat4 xform = glm::scale(geomXform(m_Matter, m_St, dc), scaleFactors(dc));

    glm::vec3 basePos = SimTKVec3FromVec3(dc.getOrigin());
    glm::vec3 baseDir = SimTKVec3FromVec3(dc.getDirection());

    glm::vec3 worldPos = glm::vec3{xform * glm::vec4{basePos, 1.0f}};
    glm::vec3 worldDir = glm::normalize(glm::vec3{xform * glm::vec4{baseDir, 0.0f}});

    SimbodyGeometry g;
    g.geometryType = SimbodyGeometry::Type::Cone;
    g.cone.rgba = extractRGBA(dc);
    g.cone.pos = worldPos;
    g.cone.direction = worldDir;
    g.cone.baseRadius = static_cast<float>(dc.getBaseRadius());
    g.cone.height = static_cast<float>(dc.getHeight());

    onGeometryEmission(g);
}
