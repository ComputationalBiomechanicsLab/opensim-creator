#include "TypeRegistry.hpp"

#include "src/Assertions.hpp"

#include <OpenSim/Actuators/DeGrooteFregly2016Muscle.h>
#include <OpenSim/Actuators/Millard2012EquilibriumMuscle.h>
#include <OpenSim/Actuators/MuscleFixedWidthPennationModel.h>
#include <OpenSim/Actuators/PointActuator.h>
#include <OpenSim/Actuators/RigidTendonMuscle.h>
#include <OpenSim/Actuators/SpringGeneralizedForce.h>
#include <OpenSim/Actuators/Thelen2003Muscle.h>
#include <OpenSim/Simulation/Model/ActivationFiberLengthMuscle.h>
#include <OpenSim/Simulation/Model/BushingForce.h>
#include <OpenSim/Simulation/Model/ContactHalfSpace.h>
#include <OpenSim/Simulation/Model/ContactMesh.h>
#include <OpenSim/Simulation/Model/ContactSphere.h>
#include <OpenSim/Simulation/Model/CoordinateLimitForce.h>
#include <OpenSim/Simulation/Model/ElasticFoundationForce.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/PointToPointSpring.h>
#include <OpenSim/Simulation/Model/SmoothSphereHalfSpaceForce.h>
#include <OpenSim/Simulation/SimbodyEngine/BallJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/ConstantDistanceConstraint.h>
#include <OpenSim/Simulation/SimbodyEngine/CoordinateCouplerConstraint.h>
// #include <OpenSim/Simulation/SimbodyEngine/CustomJoint.h>  seems to be broken on buildSystem after switching from another joint
#include <OpenSim/Simulation/SimbodyEngine/EllipsoidJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/GimbalJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PlanarJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PointOnLineConstraint.h>
#include <OpenSim/Simulation/SimbodyEngine/RollingOnSurfaceConstraint.h>
#include <OpenSim/Simulation/SimbodyEngine/SpatialTransform.h>
#include <OpenSim/Simulation/SimbodyEngine/ScapulothoracicJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/SliderJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/UniversalJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/WeldConstraint.h>
#include <OpenSim/Simulation/SimbodyEngine/WeldJoint.h>

#include <algorithm>
#include <array>

using namespace osc;

// helper: make a statically-sized array of type prototypes
template<typename TBase, typename... TDeriveds>
static auto MakePrototypeCollection() -> std::array<std::unique_ptr<TBase const>, sizeof...(TDeriveds)> {
    return std::array<std::unique_ptr<TBase const>, sizeof...(TDeriveds)>{std::make_unique<TDeriveds const>()...};
}

// helper: extract the concrete class names of components in a prototype array
template<typename Component, size_t N>
static auto ExtractNames(std::array<std::unique_ptr<Component>, N> const& components) -> std::array<char const*, N> {
    std::array<char const*, N> rv;
    for (size_t i = 0; i < N; ++i) {
        rv[i] = components[i]->getConcreteClassName().c_str();
    }
    return rv;
}

template<typename Component, size_t N>
static auto ExtractTypeHashes(std::array<std::unique_ptr<Component>, N> const& components)
    -> std::array<size_t const, N> {
    std::array<size_t const, N> rv{};
    for (size_t i = 0; i < N; ++i) {
        const_cast<size_t&>(rv[i]) = typeid(*components[i]).hash_code();
    }
    return rv;
}

// testing helper: statically assert that all elements in a collection are not null
template<typename Collection>
[[nodiscard]] static constexpr bool AllElementsNotNull(Collection const& c) noexcept {
    for (void const* el : c) {
        if (el == nullptr) {
            return false;
        }
    }
    return true;
}

// helper: construct a prototype joint and assign its coordinate names
template<typename TJoint>
static std::unique_ptr<OpenSim::Joint const> JointWithCoords(std::initializer_list<char const*> names) {
    std::unique_ptr<OpenSim::Joint> j = std::make_unique<TJoint>();
    int i = 0;
    for (char const* name : names) {
        j->upd_coordinates(i++).setName(name);
    }
    return j;
}

template<typename Container, typename T>
static std::optional<size_t> IndexOf(Container const& c, T const& v) {
    using std::begin;
    using std::end;

    auto first = begin(c);
    auto last = end(c);

    auto it = std::find(first, last, v);

    return it != last ? std::optional<size_t>{static_cast<size_t>(std::distance(first, it))} : std::nullopt;
}

// Joint LUTs

static std::array<std::unique_ptr<OpenSim::Joint const>, 10> g_JointPrototypes = {
    JointWithCoords<OpenSim::FreeJoint>({"rx", "ry", "rz", "tx", "ty", "tz"}),
    JointWithCoords<OpenSim::PinJoint>({"rz"}),
    JointWithCoords<OpenSim::UniversalJoint>({"rx", "ry"}),
    JointWithCoords<OpenSim::BallJoint>({"rx", "ry", "rz"}),
    JointWithCoords<OpenSim::EllipsoidJoint>({"rx", "ry", "rz"}),
    JointWithCoords<OpenSim::GimbalJoint>({"rx", "ry", "rz"}),
    JointWithCoords<OpenSim::PlanarJoint>({"rz", "tx", "ty"}),
    JointWithCoords<OpenSim::SliderJoint>({"tx"}),
    JointWithCoords<OpenSim::WeldJoint>({}),
    JointWithCoords<OpenSim::ScapulothoracicJoint>({"rx_abduction", "ry_elevation", "rz_upwardrotation", "ryp_winging"}),
    // joint_with_coords<OpenSim::CustomJoint>({}), // broken: see above
};
static auto const g_JointNames = ExtractNames(g_JointPrototypes);
static constexpr std::array<char const*, g_JointPrototypes.size()> g_JointDescriptions = {
    "A Free joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Free. Free joint allows unrestricted motion with three rotations and three translations. Rotations are modeled similarly to BallJoint -using quaternions with no singularities- while the translational generalized coordinates are XYZ Translations along the parent axis.",
    "A Pin joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Pin. Pin provides one DOF about the common Z-axis of the joint (not body) frames in the parent and child body. If you want rotation about a different direction, rotate the joint and body frames such that the z axes are in the desired direction.",
    "A Universal joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Universal. Universal provides two DoF: rotation about the x axis of the joint frames, followed by a rotation about the new y axis. The joint is badly behaved when the second rotation is near 90 degrees.",
    "A Ball joint. The underlying implementation in Simbody is SimTK::MobilizedBody::Ball. The Ball joint implements a fixed 1-2-3 (X-Y-Z) body-fixed Euler sequence, without translations, for generalized coordinate calculation. Ball joint uses quaternions in calculation and are therefore singularity-free (unlike GimbalJoint).",
    "An Ellipsoid joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Ellipsoid. An Ellipsoid joint provides three mobilities – coordinated rotation and translation along the surface of an ellipsoid fixed to the parent body. The ellipsoid surface is determined by an input Vec3 which describes the ellipsoid radius.",
    "A Gimbal joint. The underlying implementation Simbody is a SimTK::MobilizedBody::Gimbal. The opensim Gimbal joint implementation uses a  X-Y-Z body fixed Euler sequence for generalized coordinates calculation. Gimbal joints have a singularity when Y is near \f$\frac{\\pi}{2}\f$.",
    "A Planar joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Planar. A Planar joint provides three ordered mobilities; rotation about Z and translation in X then Y.",
    "A Slider joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Slider. The Slider provides a single coordinate along the common X-axis of the parent and child joint frames.",
    "A Weld joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Weld. There is no relative motion of bodies joined by a weld. Weld joints are often used to create composite bodies from smaller simpler bodies. You can also get the reaction force at the weld in the usual manner.",
    "A 4-DOF ScapulothoracicJoint. Motion of the scapula is described by an ellipsoid surface fixed to the thorax upon which the joint frame of scapul rides.",
    // "A class implementing a custom joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::FunctionBased. Custom joints offer a generic joint representation, which can be used to model both conventional (pins, slider, universal, etc.) as well as more complex biomechanical joints. The behavior of a custom joint is specified by its SpatialTransform. A SpatialTransform is comprised of 6 TransformAxes (3 rotations and 3 translations) that define the spatial position of Child in Parent as a function of coordinates. Each transform axis has a function of joint coordinates that describes the motion about or along the transform axis. The order of the spatial transform is fixed with rotations first followed by translations. Subsequently, coupled motion (i.e., describing motion of two degrees of freedom as a function of one coordinate) is handled by transform axis functions that depend on the same coordinate(s).",  // broken: see above
};
static auto const g_JointHashes = ExtractTypeHashes(g_JointPrototypes);

static_assert(g_JointNames.size() == g_JointPrototypes.size());
static_assert(g_JointDescriptions.size() == g_JointPrototypes.size());
static_assert(AllElementsNotNull(g_JointDescriptions), "description missing?");
static_assert(g_JointHashes.size() == g_JointPrototypes.size());

// Constraint LUTs

static auto const g_ConstraintPrototypes = MakePrototypeCollection<
    OpenSim::Constraint,
    OpenSim::ConstantDistanceConstraint,
    OpenSim::PointOnLineConstraint,
    OpenSim::RollingOnSurfaceConstraint,
    OpenSim::CoordinateCouplerConstraint,
    OpenSim::WeldConstraint>();
static auto const g_ConstraintNames = ExtractNames(g_ConstraintPrototypes);
static constexpr std::array<char const*, g_ConstraintPrototypes.size()> g_ConstraintDescriptions = {
    "Maintains a constant distance between between two points on separate PhysicalFrames. The underlying SimTK::Constraint in Simbody is a SimTK::Constraint::Rod.",
    "Implements a Point On Line Constraint. The underlying Constraint in Simbody is a SimTK::Constraint::PointOnLine.maintains a constant distance between between two points on separate PhysicalFrames. The underlying SimTK::Constraint in Simbody is a SimTK::Constraint::Rod.",
    "Implements a collection of rolling-without-slipping and non-penetration constraints on a surface.",
    "Implements a CoordinateCoupler Constraint. The underlying SimTK Constraint is a Constraint::CoordinateCoupler in Simbody, which relates coordinates to one another at the position level (i.e. holonomic). Relationship between coordinates is specified by a function that equates to zero only when the coordinates satisfy the constraint function.",
    "Implements a Weld Constraint. A WeldConstraint eliminates up to 6 dofs of a model by fixing two PhysicalFrames together at their origins aligning their axes.  PhysicalFrames are generally Ground, Body, or PhysicalOffsetFrame attached to a PhysicalFrame. The underlying Constraint in Simbody is a SimTK::Constraint::Weld.",
};
static auto const g_ConstraintHashes = ExtractTypeHashes(g_ConstraintPrototypes);

static_assert(g_ConstraintNames.size() == g_ConstraintPrototypes.size());
static_assert(g_ConstraintDescriptions.size() == g_ConstraintPrototypes.size());
static_assert(AllElementsNotNull(g_ConstraintDescriptions), "description missing?");
static_assert(g_ConstraintHashes.size() == g_ConstraintPrototypes.size());

// ContactGeometry LUTs

static auto const g_ContactGeomPrototypes = MakePrototypeCollection<
    OpenSim::ContactGeometry,

    OpenSim::ContactSphere,
    OpenSim::ContactHalfSpace,
    OpenSim::ContactMesh>();
static auto const g_ContactGeomNames = ExtractNames(g_ContactGeomPrototypes);
static constexpr std::array<char const*, g_ContactGeomPrototypes.size()> g_ContactGeomDescriptions = {
    "Represents a spherical object for use in contact modeling.",
    "Represents a half space (that is, everything to one side of an infinite plane) for use in contact modeling.  In its local coordinate system, all points for which x>0 are considered to be inside the geometry. Its location and orientation properties can be used to move and rotate it to represent other half spaces.Represents a spherical object for use in contact modeling.",
    "Represents a polygonal mesh for use in contact modeling",
};
static auto const g_ContactGeomHashes = ExtractTypeHashes(g_ContactGeomPrototypes);

static_assert(g_ContactGeomNames.size() == g_ContactGeomPrototypes.size());
static_assert(g_ContactGeomDescriptions.size() == g_ContactGeomPrototypes.size());
static_assert(AllElementsNotNull(g_ContactGeomDescriptions), "description missing?");
static_assert(g_ContactGeomHashes.size() == g_ContactGeomPrototypes.size());

// Force LUTs

std::array<std::unique_ptr<OpenSim::Force const>, 10> const g_ForcePrototypes = {
    std::make_unique<OpenSim::BushingForce>(),
    std::make_unique<OpenSim::CoordinateLimitForce>(),
    std::make_unique<OpenSim::ElasticFoundationForce>(),
    []() {
        auto hcf = std::make_unique<OpenSim::HuntCrossleyForce>();
        hcf->setStiffness(100000000.0);
        hcf->setDissipation(0.5);
        hcf->setStaticFriction(0.9);
        hcf->setDynamicFriction(0.9);
        hcf->setViscousFriction(0.6);
        return hcf;
    }(),
    std::make_unique<OpenSim::PointToPointSpring>(),
    std::make_unique<OpenSim::SmoothSphereHalfSpaceForce>(),
    std::make_unique<OpenSim::Thelen2003Muscle>(),
    std::make_unique<OpenSim::DeGrooteFregly2016Muscle>(),
    std::make_unique<OpenSim::Millard2012EquilibriumMuscle>(),
    std::make_unique<OpenSim::RigidTendonMuscle>(),
};

static auto const g_ForceNames = ExtractNames(g_ForcePrototypes);
static constexpr std::array<char const*, g_ForcePrototypes.size()> g_ForceDescriptions = {
    "A Bushing Force is the force proportional to the deviation of two frames. One can think of the Bushing as being composed of 3 linear and 3 torsional spring-dampers, which act along or about the bushing frames. Orientations are measured as x-y-z body-fixed Euler rotations, which are treated as though they were uncoupled. Damping is proportional to the deflection rate of change (e.g. Euler angle derivatives) which is NOT the angular velocity between the two frames. That makes this bushing model suitable only for relatively small relative orientation deviations between the frames.",
    "Generate a force that acts to limit the range of motion of a coordinate. Force is experienced at upper and lower limits of the coordinate value according to a constant stiffnesses K_upper and K_lower, with a C2 continuous transition from 0 to K. The transition parameter defines how far beyond the limit the stiffness becomes constant. The integrator will like smoother (i.e. larger transition regions).",
    "This Force subclass implements an elastic foundation contact model. It places a spring at the center of each face of each ContactMesh it acts on. Those springs interact with all objects (both meshes and other objects) the mesh comes in contact with.",
    "This force subclass implements a Hunt-Crossley contact model. It uses Hertz contact theory to model the interactions between a set of ContactSpheres and ContactHalfSpaces.",
    "A simple point to point spring with a resting length and stiffness. Points are connected to bodies and are defined in the body frame.",
    "This compliant contact force model is similar to HuntCrossleyForce, except that this model applies force even when not in contact. Unlike HuntCrossleyForce, the normal force is differentiable as a function of penetration depth. This component is designed for use in gradient-based optimizations, in which the model is required to be differentiable. This component models contact between a single sphere and a single half space. This force does NOT use ContactGeometry objects; the description of the contact geometries is done through properties of this component.",
    "Implementation of a two state (activation and fiber-length) Muscle model by Thelen 2003. This a complete rewrite of a previous implementation (present in OpenSim 2.4 and earlier) contained numerous errors.",
    "This muscle model was published in De Groote et al. 2016.",
    "This class implements a configurable equilibrium muscle model, as described in Millard et al. (2013).",
    "A class implementing a RigidTendonMuscle actuator with no states. The path information for a RigidTendonMuscle is contained in the base class, and the force-generating behavior should is defined in this class. The force (muscle tension) assumes rigid tendon so that fiber-length and velocity are kinematics dependent and the force-length force-velocity relationships are evaluated directly. The control of this model is its activation. Force production is instantaneous with no excitation-to-activation dynamics and excitation=activation."
};
static auto const g_ForceHashes = ExtractTypeHashes(g_ForcePrototypes);

static_assert(g_ForceNames.size() == g_ForcePrototypes.size());
static_assert(g_ForceDescriptions.size() == g_ForcePrototypes.size());
static_assert(AllElementsNotNull(g_ForceDescriptions), "description missing?");
static_assert(g_ForceHashes.size() == g_ForcePrototypes.size());

// Type_registry<OpenSim::Joint>

template<>
nonstd::span<std::unique_ptr<OpenSim::Joint const> const> osc::TypeRegistry<OpenSim::Joint>::prototypes() noexcept {
    return g_JointPrototypes;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Joint>::names() noexcept {
    return g_JointNames;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Joint>::descriptions() noexcept {
    return g_JointDescriptions;
}

template<>
std::optional<size_t> osc::TypeRegistry<OpenSim::Joint>::indexOf(OpenSim::Joint const& joint) {
    return ::IndexOf(g_JointHashes, typeid(joint).hash_code());
}

// Type_registry<OpenSim::ContactGeometry>

template<>
nonstd::span<std::unique_ptr<OpenSim::ContactGeometry const> const>
    osc::TypeRegistry<OpenSim::ContactGeometry>::prototypes() noexcept {
    return g_ContactGeomPrototypes;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::ContactGeometry>::names() noexcept {
    return g_ContactGeomNames;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::ContactGeometry>::descriptions() noexcept {
    return g_ContactGeomDescriptions;
}

template<>
std::optional<size_t> osc::TypeRegistry<OpenSim::ContactGeometry>::indexOf(OpenSim::ContactGeometry const& cg) {
    return ::IndexOf(g_ContactGeomHashes, typeid(cg).hash_code());
}

// Type_registry<OpenSim::Constraint>

template<>
nonstd::span<std::unique_ptr<OpenSim::Constraint const> const>
    osc::TypeRegistry<OpenSim::Constraint>::prototypes() noexcept {
    return g_ConstraintPrototypes;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Constraint>::names() noexcept {
    return g_ConstraintNames;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Constraint>::descriptions() noexcept {
    return g_ConstraintDescriptions;
}

template<>
std::optional<size_t> osc::TypeRegistry<OpenSim::Constraint>::indexOf(OpenSim::Constraint const& constraint) {
    return ::IndexOf(g_ConstraintHashes, typeid(constraint).hash_code());
}

// Type_registry<OpenSim::Force>

template<>
nonstd::span<std::unique_ptr<OpenSim::Force const> const> osc::TypeRegistry<OpenSim::Force>::prototypes() noexcept {
    return g_ForcePrototypes;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Force>::names() noexcept {
    return g_ForceNames;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Force>::descriptions() noexcept {
    return g_ForceDescriptions;
}

template<>
std::optional<size_t> osc::TypeRegistry<OpenSim::Force>::indexOf(OpenSim::Force const& force) {
    return ::IndexOf(g_ForceHashes, typeid(force).hash_code());
}
