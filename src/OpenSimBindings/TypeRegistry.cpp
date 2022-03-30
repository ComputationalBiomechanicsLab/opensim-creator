#include "TypeRegistry.hpp"

#include "src/Utils/Algorithms.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/CStringView.hpp"

#include <OpenSim/Actuators/BodyActuator.h>
#include <OpenSim/Actuators/DeGrooteFregly2016Muscle.h>
#include <OpenSim/Actuators/Millard2012EquilibriumMuscle.h>
#include <OpenSim/Actuators/MuscleFixedWidthPennationModel.h>
#include <OpenSim/Actuators/PointActuator.h>
#include <OpenSim/Actuators/RigidTendonMuscle.h>
#include <OpenSim/Actuators/SpringGeneralizedForce.h>
#include <OpenSim/Actuators/Thelen2003Muscle.h>
#include <OpenSim/Actuators/TorqueActuator.h>
#include <OpenSim/Simulation/Model/ActivationFiberLengthMuscle.h>
#include <OpenSim/Simulation/Model/BushingForce.h>
#include <OpenSim/Simulation/Model/ContactHalfSpace.h>
#include <OpenSim/Simulation/Model/ContactMesh.h>
#include <OpenSim/Simulation/Model/ContactSphere.h>
#include <OpenSim/Simulation/Model/CoordinateLimitForce.h>
#include <OpenSim/Simulation/Model/ElasticFoundationForce.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/PathSpring.h>
#include <OpenSim/Simulation/Model/PointToPointSpring.h>
#include <OpenSim/Simulation/Model/SmoothSphereHalfSpaceForce.h>
#include <OpenSim/Simulation/SimbodyEngine/BallJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/ConstantDistanceConstraint.h>
#include <OpenSim/Simulation/SimbodyEngine/CoordinateCouplerConstraint.h>
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
#include <memory>
#include <unordered_set>
#include <vector>

static bool SortedByClassName(std::shared_ptr<OpenSim::Component const> a, std::shared_ptr<OpenSim::Component const> b)
{
    return a->getConcreteClassName() < b->getConcreteClassName();
}

// create a lookup for user-facing description strings
//
// these are used for in-UI documentation. If a component doesn't have one of these
// then the UI should show something appropriate (e.g. "no description")
static std::unordered_map<std::type_info const*, osc::CStringView> CreateDescriptionLut()
{
    return
    {
        {
            &typeid(OpenSim::BallJoint),
            "A Ball joint. The underlying implementation in Simbody is SimTK::MobilizedBody::Ball. The Ball joint implements a fixed 1-2-3 (X-Y-Z) body-fixed Euler sequence, without translations, for generalized coordinate calculation. Ball joint uses quaternions in calculation and are therefore singularity-free (unlike GimbalJoint)."
        },
        {
            &typeid(OpenSim::EllipsoidJoint),
            "An Ellipsoid joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Ellipsoid. An Ellipsoid joint provides three mobilities – coordinated rotation and translation along the surface of an ellipsoid fixed to the parent body. The ellipsoid surface is determined by an input Vec3 which describes the ellipsoid radius.",
        },
        {
            &typeid(OpenSim::FreeJoint),
            "A Free joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Free. Free joint allows unrestricted motion with three rotations and three translations. Rotations are modeled similarly to BallJoint -using quaternions with no singularities- while the translational generalized coordinates are XYZ Translations along the parent axis.",
        },
        {
            &typeid(OpenSim::GimbalJoint),
            "A Gimbal joint. The underlying implementation Simbody is a SimTK::MobilizedBody::Gimbal. The opensim Gimbal joint implementation uses a  X-Y-Z body fixed Euler sequence for generalized coordinates calculation. Gimbal joints have a singularity when Y is near \f$\frac{\\pi}{2}\f$.",
        },
        {
            &typeid(OpenSim::PinJoint),
            "A Pin joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Pin. Pin provides one DOF about the common Z-axis of the joint (not body) frames in the parent and child body. If you want rotation about a different direction, rotate the joint and body frames such that the z axes are in the desired direction.",
        },
        {
            &typeid(OpenSim::PlanarJoint),
            "A Planar joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Planar. A Planar joint provides three ordered mobilities; rotation about Z and translation in X then Y.",
        },
        {
            &typeid(OpenSim::ScapulothoracicJoint),
            "A 4-DOF ScapulothoracicJoint. Motion of the scapula is described by an ellipsoid surface fixed to the thorax upon which the joint frame of scapul rides.",
        },
        {
            &typeid(OpenSim::SliderJoint),
            "A Slider joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Slider. The Slider provides a single coordinate along the common X-axis of the parent and child joint frames.",
        },
        {
            &typeid(OpenSim::UniversalJoint),
            "A Universal joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Universal. Universal provides two DoF: rotation about the x axis of the joint frames, followed by a rotation about the new y axis. The joint is badly behaved when the second rotation is near 90 degrees.",
        },
        {
            &typeid(OpenSim::WeldJoint),
            "A Weld joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Weld. There is no relative motion of bodies joined by a weld. Weld joints are often used to create composite bodies from smaller simpler bodies. You can also get the reaction force at the weld in the usual manner.",
        },
        {
            &typeid(OpenSim::ConstantDistanceConstraint),
            "Maintains a constant distance between between two points on separate PhysicalFrames. The underlying SimTK::Constraint in Simbody is a SimTK::Constraint::Rod.",
        },
        {
            &typeid(OpenSim::CoordinateCouplerConstraint),
            "Implements a CoordinateCoupler Constraint. The underlying SimTK Constraint is a Constraint::CoordinateCoupler in Simbody, which relates coordinates to one another at the position level (i.e. holonomic). Relationship between coordinates is specified by a function that equates to zero only when the coordinates satisfy the constraint function.",
        },
        {
            &typeid(OpenSim::PointOnLineConstraint),
            "Implements a Point On Line Constraint. The underlying Constraint in Simbody is a SimTK::Constraint::PointOnLine.maintains a constant distance between between two points on separate PhysicalFrames. The underlying SimTK::Constraint in Simbody is a SimTK::Constraint::Rod.",
        },
        {
            &typeid(OpenSim::RollingOnSurfaceConstraint),
            "Implements a collection of rolling-without-slipping and non-penetration constraints on a surface.",
        },
        {
            &typeid(OpenSim::WeldConstraint),
            "Implements a Weld Constraint. A WeldConstraint eliminates up to 6 dofs of a model by fixing two PhysicalFrames together at their origins aligning their axes.  PhysicalFrames are generally Ground, Body, or PhysicalOffsetFrame attached to a PhysicalFrame. The underlying Constraint in Simbody is a SimTK::Constraint::Weld.",
        },
        {
            &typeid(OpenSim::ContactHalfSpace),
            "Represents a half space (that is, everything to one side of an infinite plane) for use in contact modeling.  In its local coordinate system, all points for which x>0 are considered to be inside the geometry. Its location and orientation properties can be used to move and rotate it to represent other half spaces.Represents a spherical object for use in contact modeling.",
        },
        {
            &typeid(OpenSim::ContactMesh),
            "Represents a polygonal mesh for use in contact modeling",
        },
        {
            &typeid(OpenSim::ContactSphere),
            "Represents a spherical object for use in contact modeling.",
        },
        {
            &typeid(OpenSim::BodyActuator),
            "Apply a spatial force (that is, [torque, force]) on a given point of the given body. That is, the force is applied at the given point; torques don't have associated points. This actuator has no states; the control signal should provide a 6D vector including [torque(3D), force(3D)] that is supposed to be applied to the body. The associated controller can generate the spatial force [torque, force] both in the body and global (ground) frame. The default is assumed to be global frame. The point of application can be specified both in the body and global (ground) frame. The default is assumed to be the body frame.",
        },
        {
            &typeid(OpenSim::BushingForce),
            "A Bushing Force is the force proportional to the deviation of two frames. One can think of the Bushing as being composed of 3 linear and 3 torsional spring-dampers, which act along or about the bushing frames. Orientations are measured as x-y-z body-fixed Euler rotations, which are treated as though they were uncoupled. Damping is proportional to the deflection rate of change (e.g. Euler angle derivatives) which is NOT the angular velocity between the two frames. That makes this bushing model suitable only for relatively small relative orientation deviations between the frames.",
        },
        {
            &typeid(OpenSim::CoordinateLimitForce),
            "Generate a force that acts to limit the range of motion of a coordinate. Force is experienced at upper and lower limits of the coordinate value according to a constant stiffnesses K_upper and K_lower, with a C2 continuous transition from 0 to K. The transition parameter defines how far beyond the limit the stiffness becomes constant. The integrator will like smoother (i.e. larger transition regions).",
        },
        {
            &typeid(OpenSim::DeGrooteFregly2016Muscle),
            "This muscle model was published in De Groote et al. 2016.",
        },
        {
            &typeid(OpenSim::ElasticFoundationForce),
            "This Force subclass implements an elastic foundation contact model. It places a spring at the center of each face of each ContactMesh it acts on. Those springs interact with all objects (both meshes and other objects) the mesh comes in contact with.",
        },
        {
            &typeid(OpenSim::HuntCrossleyForce),
            "This force subclass implements a Hunt-Crossley contact model. It uses Hertz contact theory to model the interactions between a set of ContactSpheres and ContactHalfSpaces.",
        },
        {
            &typeid(OpenSim::Millard2012EquilibriumMuscle),
            "This class implements a configurable equilibrium muscle model, as described in Millard et al. (2013).",
        },
        {
            &typeid(OpenSim::PointToPointSpring),
            "A simple point to point spring with a resting length and stiffness. Points are connected to bodies and are defined in the body frame.",
        },
        {
            &typeid(OpenSim::PathSpring),
            "A spring that follows a one-dimensional path. A PathSpring is a massless force element which applies tension along a path connected to bodies. A path spring can also wrap over wrap surfaces.\n\nThe tension is proportional to its stretch beyond its resting length and the amount of dissipation scales with the amount of stretch.",
        },
        {
            &typeid(OpenSim::RigidTendonMuscle),
            "A class implementing a RigidTendonMuscle actuator with no states. The path information for a RigidTendonMuscle is contained in the base class, and the force-generating behavior should is defined in this class. The force (muscle tension) assumes rigid tendon so that fiber-length and velocity are kinematics dependent and the force-length force-velocity relationships are evaluated directly. The control of this model is its activation. Force production is instantaneous with no excitation-to-activation dynamics and excitation=activation.",
        },
        {
            &typeid(OpenSim::SmoothSphereHalfSpaceForce),
            "This compliant contact force model is similar to HuntCrossleyForce, except that this model applies force even when not in contact. Unlike HuntCrossleyForce, the normal force is differentiable as a function of penetration depth. This component is designed for use in gradient-based optimizations, in which the model is required to be differentiable. This component models contact between a single sphere and a single half space. This force does NOT use ContactGeometry objects; the description of the contact geometries is done through properties of this component.",
        },
        {
            &typeid(OpenSim::Thelen2003Muscle),
            "Implementation of a two state (activation and fiber-length) Muscle model by Thelen 2003. This a complete rewrite of a previous implementation (present in OpenSim 2.4 and earlier) contained numerous errors.",
        },
        {
            &typeid(OpenSim::TorqueActuator),
            "A TorqueActuator applies equal and opposite torques on the two bodies (bodyA and B) that it connects. The torque is applied about an axis specified in ground (global) by default, otherwise it is in bodyA's frame. The magnitude of the torque is equal to the product of the optimal_force of the actuator and its control signal.",
        },
    };
}

// fetch cached version of the above lookup
static std::unordered_map<std::type_info const*, osc::CStringView> const& GetDescriptionLut()
{
    static std::unordered_map<std::type_info const*, osc::CStringView> const g_Lut = CreateDescriptionLut();
    return g_Lut;
}

// helper: construct a prototype joint and assign its coordinate names
template<typename TJoint>
static std::shared_ptr<OpenSim::Joint const> JointWithCoords(std::initializer_list<char const*> names)
{
    std::shared_ptr<OpenSim::Joint> j = std::make_shared<TJoint>();
    int i = 0;
    for (char const* name : names)
    {
        j->upd_coordinates(i++).setName(name);
    }
    return j;
}

// create a lookup of pre-initialized prototype components
static std::unordered_map<std::type_info const*, std::shared_ptr<OpenSim::Component const>> CreatePrototypeLut()
{
    return
    {
        {
            &typeid(OpenSim::BallJoint),
            JointWithCoords<OpenSim::BallJoint>({"rx", "ry", "rz"}),
        },
        {
            &typeid(OpenSim::EllipsoidJoint),
            JointWithCoords<OpenSim::EllipsoidJoint>({"rx", "ry", "rz"}),
        },
        {
            &typeid(OpenSim::FreeJoint),
            JointWithCoords<OpenSim::FreeJoint>({"rx", "ry", "rz", "tx", "ty", "tz"}),
        },
        {
            &typeid(OpenSim::GimbalJoint),
            JointWithCoords<OpenSim::GimbalJoint>({"rx", "ry", "rz"}),
        },
        {
            &typeid(OpenSim::PinJoint),
            JointWithCoords<OpenSim::PinJoint>({"rz"}),
        },
        {
            &typeid(OpenSim::PlanarJoint),
            JointWithCoords<OpenSim::PlanarJoint>({"rz", "tx", "ty"}),
        },
        {
            &typeid(OpenSim::ScapulothoracicJoint),
            JointWithCoords<OpenSim::ScapulothoracicJoint>({"rx_abduction", "ry_elevation", "rz_upwardrotation", "ryp_winging"}),
        },
        {
            &typeid(OpenSim::SliderJoint),
            JointWithCoords<OpenSim::SliderJoint>({"tx"}),
        },
        {
            &typeid(OpenSim::UniversalJoint),
            JointWithCoords<OpenSim::UniversalJoint>({"rx", "ry"}),
        },
        {
            &typeid(OpenSim::WeldJoint),
            JointWithCoords<OpenSim::WeldJoint>({}),
        },
        {
            &typeid(OpenSim::HuntCrossleyForce),
            []() {
                auto hcf = std::make_shared<OpenSim::HuntCrossleyForce>();
                hcf->setStiffness(100000000.0);
                hcf->setDissipation(0.5);
                hcf->setStaticFriction(0.9);
                hcf->setDynamicFriction(0.9);
                hcf->setViscousFriction(0.6);
                return hcf;
            }(),
        },
        {
            &typeid(OpenSim::PathSpring),
            []() {
                auto ps = std::make_shared<OpenSim::PathSpring>();
                ps->setRestingLength(1.0);
                ps->setStiffness(1000.0);
                ps->setDissipation(0.5);
                return ps;
            }(),
        },
    };
}

static std::unordered_map<std::type_info const*, std::shared_ptr<OpenSim::Component const>> const& GetPrototypeLut()
{
    static std::unordered_map<std::type_info const*, std::shared_ptr<OpenSim::Component const>> const g_Lut = CreatePrototypeLut();
    return g_Lut;
}

template<typename T>
static std::vector<std::shared_ptr<T const>> CreatePrototypeLutT()
{
    OpenSim::ArrayPtrs<T> ptrs;
    OpenSim::Object::getRegisteredObjectsOfGivenType<T>(ptrs);

    std::vector<std::shared_ptr<T const>> rv;
    rv.reserve(ptrs.size());

    auto const& protoLut = GetPrototypeLut();

    for (int i = 0; i < ptrs.size(); ++i)
    {
        T const& v = *ptrs[i];
        auto it = protoLut.find(&typeid(v));
        if (it != protoLut.end())
        {
            std::shared_ptr<T const> p = std::dynamic_pointer_cast<T const>(it->second);
            if (p)
            {
                rv.push_back(p);
            }
            else
            {
                rv.emplace_back(v.clone());
            }
        }
        else
        {
            rv.emplace_back(v.clone());
        }
    }

    osc::Sort(rv, SortedByClassName);

    return rv;
}

template<typename T>
static std::vector<osc::CStringView> CreateNameViews(nonstd::span<std::shared_ptr<T const> const> protos)
{
    std::vector<osc::CStringView> rv;
    rv.reserve(protos.size());
    for (auto const& proto : protos)
    {
        rv.push_back(proto->getConcreteClassName());
    }
    return rv;
}

template<typename T>
static std::vector<osc::CStringView> CreateDescriptionViews(nonstd::span<std::shared_ptr<T const> const> protos)
{
    auto const& lut = GetDescriptionLut();

    std::vector<osc::CStringView> rv;
    rv.reserve(protos.size());
    for (auto const& proto : protos)
    {
        auto it = lut.find(&typeid(*proto));
        if (it != lut.end())
        {
            rv.push_back(it->second);
        }
        else
        {
            rv.emplace_back();
        }
    }
    return rv;
}

static std::vector<char const*> CreateCStrings(nonstd::span<osc::CStringView const> views)
{
    std::vector<char const*> rv;
    rv.reserve(views.size());
    for (auto const& view : views)
    {
        rv.push_back(view.c_str());
    }
    return rv;
}

template<typename T>
static std::optional<size_t> IndexOf(nonstd::span<std::shared_ptr<T const> const> container, std::type_info const& ti)
{
    auto hasTypeID = [&ti](std::shared_ptr<T const> const& p)
    {
        return typeid(*p) == ti;
    };
    auto it = std::find_if(container.begin(), container.end(), hasTypeID);
    return it == container.end() ? std::nullopt : std::optional<size_t>{std::distance(container.begin(), it)};
}

template<typename T>
static void AddTypeIDsFromRegistry(nonstd::span<std::shared_ptr<T const> const> protos,
                                   std::unordered_set<std::type_info const*>& out)
{
    for (auto const& ptr : protos)
    {
        out.insert(&typeid(*ptr));
    }
}


// TypeRegistry<OpenSim::Joint>

template<>
osc::CStringView osc::TypeRegistry<OpenSim::Joint>::name() noexcept
{
    return "Joint";
}

template<>
osc::CStringView osc::TypeRegistry<OpenSim::Joint>::description() noexcept
{
    return "An OpenSim::Joint is a OpenSim::ModelComponent which connects two PhysicalFrames together and specifies their relative permissible motion as described in internal coordinates.";
}

template<>
nonstd::span<std::shared_ptr<OpenSim::Joint const> const> osc::TypeRegistry<OpenSim::Joint>::prototypes() noexcept
{
    static std::vector<std::shared_ptr<OpenSim::Joint const>> g_Protos = CreatePrototypeLutT<OpenSim::Joint>();
    return g_Protos;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Joint>::nameStrings() noexcept
{
    static std::vector<osc::CStringView> const g_Names = CreateNameViews(prototypes());
    return g_Names;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Joint>::nameCStrings() noexcept
{
    static std::vector<char const*> const g_CStrs = CreateCStrings(nameStrings());
    return g_CStrs;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Joint>::descriptionStrings() noexcept
{
    static std::vector<osc::CStringView> const g_Descriptions = CreateDescriptionViews(prototypes());
    return g_Descriptions;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Joint>::descriptionCStrings() noexcept
{
    static std::vector<char const*> const g_DescriptionCStrs = CreateCStrings(descriptionStrings());
    return g_DescriptionCStrs;
}

template<>
std::optional<size_t> osc::TypeRegistry<OpenSim::Joint>::indexOf(OpenSim::Joint const& joint) noexcept
{
    return ::IndexOf(prototypes(), typeid(joint));
}


// TypeRegistry<OpenSim::ContactGeometry>

template<>
osc::CStringView osc::TypeRegistry<OpenSim::ContactGeometry>::name() noexcept
{
    return "Contact Geometry";
}

template<>
osc::CStringView osc::TypeRegistry<OpenSim::ContactGeometry>::description() noexcept
{
    return "Add a geometry with a physical shape that participates in contact modeling. The geometry is attached to an OpenSim::PhysicalFrame in the model (e.g. a body) and and moves with that frame.";
}

template<>
nonstd::span<std::shared_ptr<OpenSim::ContactGeometry const> const> osc::TypeRegistry<OpenSim::ContactGeometry>::prototypes() noexcept
{
    static std::vector<std::shared_ptr<OpenSim::ContactGeometry const>> g_Protos =CreatePrototypeLutT<OpenSim::ContactGeometry>();
    return g_Protos;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::ContactGeometry>::nameStrings() noexcept
{
    static std::vector<osc::CStringView> const g_Names = CreateNameViews(prototypes());
    return g_Names;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::ContactGeometry>::nameCStrings() noexcept
{
    static std::vector<char const*> const g_CStrs = CreateCStrings(nameStrings());
    return g_CStrs;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::ContactGeometry>::descriptionStrings() noexcept
{
    static std::vector<osc::CStringView> const g_Descriptions = CreateDescriptionViews(prototypes());
    return g_Descriptions;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::ContactGeometry>::descriptionCStrings() noexcept
{
    static std::vector<char const*> const g_DescriptionCStrs = CreateCStrings(descriptionStrings());
    return g_DescriptionCStrs;
}

template<>
std::optional<size_t> osc::TypeRegistry<OpenSim::ContactGeometry>::indexOf(OpenSim::ContactGeometry const& cg) noexcept
{
    return ::IndexOf(prototypes(), typeid(cg));
}


// TypeRegistry<OpenSim::Constraint>

template<>
osc::CStringView osc::TypeRegistry<OpenSim::Constraint>::name() noexcept
{
    return "Constraint";
}

template<>
osc::CStringView osc::TypeRegistry<OpenSim::Constraint>::description() noexcept
{
    return "A constraint typically constrains the motion of physical frame(s) in the model some way. For example, an OpenSim::ConstantDistanceConstraint constrains the system to *have* to keep two frames at some constant distance from eachover.";
}

template<>
nonstd::span<std::shared_ptr<OpenSim::Constraint const> const> osc::TypeRegistry<OpenSim::Constraint>::prototypes() noexcept
{
    static std::vector<std::shared_ptr<OpenSim::Constraint const>> g_Protos = CreatePrototypeLutT<OpenSim::Constraint>();
    return g_Protos;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Constraint>::nameStrings() noexcept
{
    static std::vector<osc::CStringView> const g_Names = CreateNameViews(prototypes());
    return g_Names;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Constraint>::nameCStrings() noexcept
{
    static std::vector<char const*> const g_CStrs = CreateCStrings(nameStrings());
    return g_CStrs;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Constraint>::descriptionStrings() noexcept
{
    static std::vector<osc::CStringView> const g_Descriptions = CreateDescriptionViews(prototypes());
    return g_Descriptions;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Constraint>::descriptionCStrings() noexcept
{
    static std::vector<char const*> const g_DescriptionCStrs = CreateCStrings(descriptionStrings());
    return g_DescriptionCStrs;
}

template<>
std::optional<size_t> osc::TypeRegistry<OpenSim::Constraint>::indexOf(OpenSim::Constraint const& cg) noexcept
{
    return ::IndexOf(prototypes(), typeid(cg));
}


// TypeRegistry<OpenSim::Force>

template<>
osc::CStringView osc::TypeRegistry<OpenSim::Force>::name() noexcept
{
    return "Force";
}

template<>
osc::CStringView osc::TypeRegistry<OpenSim::Force>::description() noexcept
{
    return "During a simulation, the force is applied to bodies or generalized coordinates in the model. Muscles are specialized `OpenSim::Force`s with biomech-focused features.";
}

template<>
nonstd::span<std::shared_ptr<OpenSim::Force const> const> osc::TypeRegistry<OpenSim::Force>::prototypes() noexcept
{
    static std::vector<std::shared_ptr<OpenSim::Force const>> g_Protos = CreatePrototypeLutT<OpenSim::Force>();
    return g_Protos;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Force>::nameStrings() noexcept
{
    static std::vector<osc::CStringView> const g_Names = CreateNameViews(prototypes());
    return g_Names;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Force>::nameCStrings() noexcept
{
    static std::vector<char const*> const g_CStrs = CreateCStrings(nameStrings());
    return g_CStrs;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Force>::descriptionStrings() noexcept
{
    static std::vector<osc::CStringView> const g_Descriptions = CreateDescriptionViews(prototypes());
    return g_Descriptions;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Force>::descriptionCStrings() noexcept
{
    static std::vector<char const*> const g_DescriptionCStrs = CreateCStrings(descriptionStrings());
    return g_DescriptionCStrs;
}

template<>
std::optional<size_t> osc::TypeRegistry<OpenSim::Force>::indexOf(OpenSim::Force const& cg) noexcept
{
    return ::IndexOf(prototypes(), typeid(cg));
}


// TypeRegistry<OpenSim::Controller>

template<>
osc::CStringView osc::TypeRegistry<OpenSim::Controller>::name() noexcept
{
    return "Controller";
}

template<>
osc::CStringView osc::TypeRegistry<OpenSim::Controller>::description() noexcept
{
    return "A controller computes and sets the values of the controls for the actuators under its control.";
}

template<>
nonstd::span<std::shared_ptr<OpenSim::Controller const> const> osc::TypeRegistry<OpenSim::Controller>::prototypes() noexcept
{
    static std::vector<std::shared_ptr<OpenSim::Controller const>> g_Protos = CreatePrototypeLutT<OpenSim::Controller>();
    return g_Protos;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Controller>::nameStrings() noexcept
{
    static std::vector<osc::CStringView> const g_Names = CreateNameViews(prototypes());
    return g_Names;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Controller>::nameCStrings() noexcept
{
    static std::vector<char const*> const g_CStrs = CreateCStrings(nameStrings());
    return g_CStrs;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Controller>::descriptionStrings() noexcept
{
    static std::vector<osc::CStringView> const g_Descriptions = CreateDescriptionViews(prototypes());
    return g_Descriptions;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Controller>::descriptionCStrings() noexcept
{
    static std::vector<char const*> const g_DescriptionCStrs = CreateCStrings(descriptionStrings());
    return g_DescriptionCStrs;
}

template<>
std::optional<size_t> osc::TypeRegistry<OpenSim::Controller>::indexOf(OpenSim::Controller const& cg) noexcept
{
    return ::IndexOf(prototypes(), typeid(cg));
}


// TypeRegistry<OpenSim::Probe>

template<>
osc::CStringView osc::TypeRegistry<OpenSim::Probe>::name() noexcept
{
    return "Probe";
}

template<>
osc::CStringView osc::TypeRegistry<OpenSim::Probe>::description() noexcept
{
    return "This class represents a Probe which is designed to query a Vector of model values given system state. This model quantity is specified as a SimTK::Vector by the pure virtual method computeProbeInputs(), which must be specified for each child Probe.  In addition, the Probe model component interface allows <I> operations </I> to be performed on this value (specified by the property: probe_operation), and then have this result scaled (by the scalar property: 'scale_factor'). A controller computes and sets the values of the controls for the actuators under its control.";
}

template<>
nonstd::span<std::shared_ptr<OpenSim::Probe const> const> osc::TypeRegistry<OpenSim::Probe>::prototypes() noexcept
{
    static std::vector<std::shared_ptr<OpenSim::Probe const>> g_Protos = CreatePrototypeLutT<OpenSim::Probe>();
    return g_Protos;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Probe>::nameStrings() noexcept
{
    static std::vector<osc::CStringView> const g_Names = CreateNameViews(prototypes());
    return g_Names;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Probe>::nameCStrings() noexcept
{
    static std::vector<char const*> const g_CStrs = CreateCStrings(nameStrings());
    return g_CStrs;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Probe>::descriptionStrings() noexcept
{
    static std::vector<osc::CStringView> const g_Descriptions = CreateDescriptionViews(prototypes());
    return g_Descriptions;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Probe>::descriptionCStrings() noexcept
{
    static std::vector<char const*> const g_DescriptionCStrs = CreateCStrings(descriptionStrings());
    return g_DescriptionCStrs;
}

template<>
std::optional<size_t> osc::TypeRegistry<OpenSim::Probe>::indexOf(OpenSim::Probe const& cg) noexcept
{
    return ::IndexOf(prototypes(), typeid(cg));
}


// TypeRegistry<OpenSim::Component>

static std::vector<std::shared_ptr<OpenSim::Component const>> CreateOtherComponentLut()
{
    std::unordered_set<std::type_info const*> groupedEls;
    AddTypeIDsFromRegistry(osc::TypeRegistry<OpenSim::Joint>::prototypes(), groupedEls);
    AddTypeIDsFromRegistry(osc::TypeRegistry<OpenSim::ContactGeometry>::prototypes(), groupedEls);
    AddTypeIDsFromRegistry(osc::TypeRegistry<OpenSim::Constraint>::prototypes(), groupedEls);
    AddTypeIDsFromRegistry(osc::TypeRegistry<OpenSim::Force>::prototypes(), groupedEls);
    AddTypeIDsFromRegistry(osc::TypeRegistry<OpenSim::Controller>::prototypes(), groupedEls);
    AddTypeIDsFromRegistry(osc::TypeRegistry<OpenSim::Probe>::prototypes(), groupedEls);

    OpenSim::ArrayPtrs<OpenSim::ModelComponent> ptrs;
    OpenSim::Object::getRegisteredObjectsOfGivenType<OpenSim::ModelComponent>(ptrs);

    std::vector<std::shared_ptr<OpenSim::Component const>> rv;
    OSC_ASSERT(ptrs.size() > static_cast<int>(groupedEls.size()));
    rv.reserve(ptrs.size() - groupedEls.size());

    for (int i = 0; i < ptrs.size(); ++i)
    {
        OpenSim::Component const& c = *ptrs[i];
        if (!osc::Contains(groupedEls, &typeid(c)))
        {
            rv.emplace_back(c.clone());
        }
    }

    osc::Sort(rv, SortedByClassName);

    return rv;
}

template<>
osc::CStringView osc::TypeRegistry<OpenSim::Component>::name() noexcept
{
    return "Component";
}

template<>
osc::CStringView osc::TypeRegistry<OpenSim::Component>::description() noexcept
{
    return "These are all the components that OpenSim Creator knows about, but can't put into an existing category (e.g. Force)";
}

template<>
nonstd::span<std::shared_ptr<OpenSim::Component const> const> osc::TypeRegistry<OpenSim::Component>::prototypes() noexcept
{
    static std::vector<std::shared_ptr<OpenSim::Component const>> g_Protos = CreateOtherComponentLut();
    return g_Protos;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Component>::nameStrings() noexcept
{
    static std::vector<osc::CStringView> const g_Names = CreateNameViews(prototypes());
    return g_Names;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Component>::nameCStrings() noexcept
{
    static std::vector<char const*> const g_CStrs = CreateCStrings(nameStrings());
    return g_CStrs;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Component>::descriptionStrings() noexcept
{
    static std::vector<osc::CStringView> const g_Descriptions = CreateDescriptionViews(prototypes());
    return g_Descriptions;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Component>::descriptionCStrings() noexcept
{
    static std::vector<char const*> const g_DescriptionCStrs = CreateCStrings(descriptionStrings());
    return g_DescriptionCStrs;
}

template<>
std::optional<size_t> osc::TypeRegistry<OpenSim::Component>::indexOf(OpenSim::Component const& cg) noexcept
{
    return ::IndexOf(prototypes(), typeid(cg));
}
