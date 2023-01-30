#include "TypeRegistry.hpp"

#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/Macros.hpp"

#include <nonstd/span.hpp>
#include <OpenSim/Common/ArrayPtrs.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/LinearFunction.h>
#include <OpenSim/Common/Object.h>
#include <OpenSim/Simulation/Control/Controller.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Force.h>
#include <OpenSim/Simulation/Model/Probe.h>
#include <OpenSim/Simulation/SimbodyEngine/Constraint.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>

// these have manual prototypes
#include <OpenSim/Actuators/ActivationCoordinateActuator.h>
#include <OpenSim/Actuators/PointToPointActuator.h>
#include <OpenSim/Actuators/SpringGeneralizedForce.h>
#include <OpenSim/Simulation/Model/ContactSphere.h>
#include <OpenSim/Simulation/Model/ExpressionBasedPointToPointForce.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/PathActuator.h>
#include <OpenSim/Simulation/Model/PathSpring.h>
#include <OpenSim/Simulation/SimbodyEngine/BallJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/ConstantDistanceConstraint.h>
#include <OpenSim/Simulation/SimbodyEngine/CoordinateCouplerConstraint.h>
#include <OpenSim/Simulation/SimbodyEngine/EllipsoidJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/GimbalJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PlanarJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/ScapulothoracicJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/SliderJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/UniversalJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/WeldJoint.h>


#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// create a lookup for user-facing description strings
//
// these are used for in-UI documentation. If a component doesn't have one of these
// then the UI should show something appropriate (e.g. "no description")
static std::unordered_map<osc::CStringView, osc::CStringView> CreateDescriptionLut()
{
    return
    {
        {
            "BallJoint",
            "A Ball joint. The underlying implementation in Simbody is SimTK::MobilizedBody::Ball. The Ball joint implements a fixed 1-2-3 (X-Y-Z) body-fixed Euler sequence, without translations, for generalized coordinate calculation. Ball joint uses quaternions in calculation and are therefore singularity-free (unlike GimbalJoint)."
        },
        {
            "CustomJoint",
            "Custom joints offer a generic joint representation, which can be used to model both conventional (pins, slider, universal, etc.) as well as more complex biomechanical joints. The behavior of a custom joint is specified by its SpatialTransform. A SpatialTransform is comprised of 6 TransformAxes (3 rotations and 3 translations) that define the spatial position of Child in Parent as a function of coordinates. Each transform axis has a function of joint coordinates that describes the motion about or along the transform axis. The order of the spatial transform is fixed with rotations first followed by translations. Subsequently, coupled motion (i.e., describing motion of two degrees of freedom as a function of one coordinate) is handled by transform axis functions that depend on the same coordinate(s).",
        },
        {
            "EllipsoidJoint",
            "An Ellipsoid joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Ellipsoid. An Ellipsoid joint provides three mobilities – coordinated rotation and translation along the surface of an ellipsoid fixed to the parent body. The ellipsoid surface is determined by an input Vec3 which describes the ellipsoid radius.",
        },
        {
            "FreeJoint",
            "A Free joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Free. Free joint allows unrestricted motion with three rotations and three translations. Rotations are modeled similarly to BallJoint -using quaternions with no singularities- while the translational generalized coordinates are XYZ Translations along the parent axis.",
        },
        {
            "GimbalJoint",
            "A Gimbal joint. The underlying implementation Simbody is a SimTK::MobilizedBody::Gimbal. The opensim Gimbal joint implementation uses a  X-Y-Z body fixed Euler sequence for generalized coordinates calculation. Gimbal joints have a singularity when Y is near \f$\frac{\\pi}{2}\f$.",
        },
        {
            "PinJoint",
            "A Pin joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Pin. Pin provides one DOF about the common Z-axis of the joint (not body) frames in the parent and child body. If you want rotation about a different direction, rotate the joint and body frames such that the z axes are in the desired direction.",
        },
        {
            "PlanarJoint",
            "A Planar joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Planar. A Planar joint provides three ordered mobilities; rotation about Z and translation in X then Y.",
        },
        {
            "ScapulothoracicJoint",
            "A 4-DOF ScapulothoracicJoint. Motion of the scapula is described by an ellipsoid surface fixed to the thorax upon which the joint frame of scapul rides.",
        },
        {
            "SliderJoint",
            "A Slider joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Slider. The Slider provides a single coordinate along the common X-axis of the parent and child joint frames.",
        },
        {
            "UniversalJoint",
            "A Universal joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Universal. Universal provides two DoF: rotation about the x axis of the joint frames, followed by a rotation about the new y axis. The joint is badly behaved when the second rotation is near 90 degrees.",
        },
        {
            "WeldJoint",
            "A Weld joint. The underlying implementation in Simbody is a SimTK::MobilizedBody::Weld. There is no relative motion of bodies joined by a weld. Weld joints are often used to create composite bodies from smaller simpler bodies. You can also get the reaction force at the weld in the usual manner.",
        },
        {
            "CustomDistanceJoint",
            "Maintains a constant distance between between two points on separate PhysicalFrames. The underlying SimTK::Constraint in Simbody is a SimTK::Constraint::Rod.",
        },
        {
            "CoordinateCouplerConstraint",
            "Implements a CoordinateCoupler Constraint. The underlying SimTK Constraint is a Constraint::CoordinateCoupler in Simbody, which relates coordinates to one another at the position level (i.e. holonomic). Relationship between coordinates is specified by a function that equates to zero only when the coordinates satisfy the constraint function.",
        },
        {
            "PointOnLineConstraint",
            "Implements a Point On Line Constraint. The underlying Constraint in Simbody is a SimTK::Constraint::PointOnLine.maintains a constant distance between between two points on separate PhysicalFrames. The underlying SimTK::Constraint in Simbody is a SimTK::Constraint::Rod.",
        },
        {
            "RollingOnSurfaceConstraint",
            "Implements a collection of rolling-without-slipping and non-penetration constraints on a surface.",
        },
        {
            "WeldConstraint",
            "Implements a Weld Constraint. A WeldConstraint eliminates up to 6 dofs of a model by fixing two PhysicalFrames together at their origins aligning their axes.  PhysicalFrames are generally Ground, Body, or PhysicalOffsetFrame attached to a PhysicalFrame. The underlying Constraint in Simbody is a SimTK::Constraint::Weld.",
        },
        {
            "ContactHalfSpace",
            "Represents a half space (that is, everything to one side of an infinite plane) for use in contact modeling.  In its local coordinate system, all points for which x>0 are considered to be inside the geometry. Its location and orientation properties can be used to move and rotate it to represent other half spaces.Represents a spherical object for use in contact modeling.",
        },
        {
            "ContactMesh",
            "Represents a polygonal mesh for use in contact modeling",
        },
        {
            "ContactSphere",
            "Represents a spherical object for use in contact modeling.",
        },
        {
            "BodyActuator",
            "Apply a spatial force (that is, [torque, force]) on a given point of the given body. That is, the force is applied at the given point; torques don't have associated points. This actuator has no states; the control signal should provide a 6D vector including [torque(3D), force(3D)] that is supposed to be applied to the body. The associated controller can generate the spatial force [torque, force] both in the body and global (ground) frame. The default is assumed to be global frame. The point of application can be specified both in the body and global (ground) frame. The default is assumed to be the body frame.",
        },
        {
            "BushingForce",
            "A Bushing Force is the force proportional to the deviation of two frames. One can think of the Bushing as being composed of 3 linear and 3 torsional spring-dampers, which act along or about the bushing frames. Orientations are measured as x-y-z body-fixed Euler rotations, which are treated as though they were uncoupled. Damping is proportional to the deflection rate of change (e.g. Euler angle derivatives) which is NOT the angular velocity between the two frames. That makes this bushing model suitable only for relatively small relative orientation deviations between the frames.",
        },
        {
            "CoordinateLimitForce",
            "Generate a force that acts to limit the range of motion of a coordinate. Force is experienced at upper and lower limits of the coordinate value according to a constant stiffnesses K_upper and K_lower, with a C2 continuous transition from 0 to K. The transition parameter defines how far beyond the limit the stiffness becomes constant. The integrator will like smoother (i.e. larger transition regions).",
        },
        {
            "DeGrooteFregly2016Muscle",
            "This muscle model was published in De Groote et al. 2016.",
        },
        {
            "ElasticFoundationForce",
            "This Force subclass implements an elastic foundation contact model. It places a spring at the center of each face of each ContactMesh it acts on. Those springs interact with all objects (both meshes and other objects) the mesh comes in contact with.",
        },
        {
            "HuntCrossleyForce",
            "This force subclass implements a Hunt-Crossley contact model. It uses Hertz contact theory to model the interactions between a set of ContactSpheres and ContactHalfSpaces.",
        },
        {
            "Millard2012EquilibriumMuscle",
            "This class implements a configurable equilibrium muscle model, as described in Millard et al. (2013).",
        },
        {
            "PointToPointSpring",
            "A simple point to point spring with a resting length and stiffness. Points are connected to bodies and are defined in the body frame.",
        },
        {
            "PathSpring",
            "A spring that follows a one-dimensional path. A PathSpring is a massless force element which applies tension along a path connected to bodies. A path spring can also wrap over wrap surfaces.\n\nThe tension is proportional to its stretch beyond its resting length and the amount of dissipation scales with the amount of stretch.",
        },
        {
            "RigidTendonMuscle",
            "A class implementing a RigidTendonMuscle actuator with no states. The path information for a RigidTendonMuscle is contained in the base class, and the force-generating behavior should is defined in this class. The force (muscle tension) assumes rigid tendon so that fiber-length and velocity are kinematics dependent and the force-length force-velocity relationships are evaluated directly. The control of this model is its activation. Force production is instantaneous with no excitation-to-activation dynamics and excitation=activation.",
        },
        {
            "SmoothSphereHalfForce",
            "This compliant contact force model is similar to HuntCrossleyForce, except that this model applies force even when not in contact. Unlike HuntCrossleyForce, the normal force is differentiable as a function of penetration depth. This component is designed for use in gradient-based optimizations, in which the model is required to be differentiable. This component models contact between a single sphere and a single half space. This force does NOT use ContactGeometry objects; the description of the contact geometries is done through properties of this component.",
        },
        {
            "Thelen2003Muscle",
            "Implementation of a two state (activation and fiber-length) Muscle model by Thelen 2003. This a complete rewrite of a previous implementation (present in OpenSim 2.4 and earlier) contained numerous errors.",
        },
        {
            "TorqueActuator",
            "A TorqueActuator applies equal and opposite torques on the two bodies (bodyA and B) that it connects. The torque is applied about an axis specified in ground (global) by default, otherwise it is in bodyA's frame. The magnitude of the torque is equal to the product of the optimal_force of the actuator and its control signal.",
        },
        {
            "PointConstraint",
            "A class implementing a Point Constraint.The constraint keeps two points, one on each of two separate PhysicalFrame%s, coincident and free to rotate about that point.",
        },
        {
            "ActivationCoordinateActuator",
            "Similar to CoordinateActuator (simply produces a generalized force) but with first-order linear activation dynamics. This actuator has one state variable, `activation`, with \\f$ \\dot{a} = (x - a) / \\tau \\f$, where \\f$ a \a\f$ is activation, \\f$ x \\f$ is excitation, and \\f$ \\tau \\f$ is the activation time constant (there is no separate deactivation time constant). The statebounds_activation output is used in Moco to set default values for the activation state variable.",
        },
        {
            "Blankevoort1991Ligament",
            "This class implements a nonlinear spring ligament model introduced by Blankevoort et al.(1991) [1] and further described in Smith et al.(2016) [2]. This model is partially based on the formulation orginally proposed by Wismans et al. (1980) [3]. The ligament is represented as a passive spring with the force-strain relationship described by a quadratic \"toe\" region at low strains and a linear region at high strains. The toe region represents the uncrimping and alignment of collagen fibers and the linear region represents the subsequent stretching of the aligned fibers. The ligament model also includes a damping force that is only applied if the ligament is stretched beyond the slack length and if the ligament is lengthening.",
        },
        {
            "ClutchedPathSpring",
            "The ClutchedPathSpring is an actuator that has passive path spring behavior only when the clutch is engaged. The clutch is engaged by a control signal of 1 and is off for a control signal of 0. Off means the spring is not engaged and the path is free to change length with the motion of the bodies it is connected to. The tension produced by the spring is proportional to the stretch (z) from the instant that the clutch is engaged.\n The spring tension = x*(K*z)*(1+D*Ldot), where:\n    - x is the control signal to the actuator\n    - z is the stretch in the spring\n    - Ldot is the lengthening speed of the actuator\n    - K is the spring's linear stiffness (N/m)\n    - D is the spring's dissipation factor",
        },
        {
            "CoordinateActuator",
            "An actuator that applies a generalized force in the direction of a generalized coordinate. The applied generalized force is proportional to the input control of the CoordinateActuator. Replaces the GeneralizedForce class.",
        },
        {
            "ExpressionBasedPointToPointForce",
            "A point - to - point Force who's force magnitude is determined by a user-defined expression, with the distance (d) and its time derivative (ddot) as variables. The direction of the force is directed along the line connecting the two points.\n \"d\" and \"ddot\" are the variables names expected by the expression parser. Common C math library functions such as: exp(), pow(), sqrt(), sin(), are permitted. See Lepton/Operation.h for a complete list.\n\nFor example: string expression = \"-1.5*exp(10*(d-0.25)^2)*(1 + 2.0*ddot)\" provides a model of a nonlinear point-to point spring, while expression = \"1.25/(rd^2)\" is an electric field force between charged particles at points separated by the distance, d. i.e. K*q1*q2 = 1.25",
        },
        {
            "ExternalForce",
            "An ExternalForce is a Force class specialized at applying an external force and /or torque to a body as described by arrays(columns) of a Storage object.The source of the Storage may be experimental sensor recording or user generated data.The Storage must be able to supply(1) an array of time, (2) arrays for the x,y,z, components of forceand /or torque in time.Optionally, (3) arrays for the point of force application in time.An ExternalForce must specify the identifier(e.g.Force1.x Force1.y Force1.z) for the force components(columns) listed in the Storage either by individual labels or collectively(e.g.as \"Force1\"). Similarly, identifiers for the applied torque and optionally the point of force application must be specified.\n\nIf an identifier is supplied and it cannot uniquely identify the force data (e.g. the force, torque, or point) in the Storage, then an Exception is thrown.",
        },
        {
            "FunctionBasedBushingForce",
            "A class implementing a bushing force specified by functions of the frame deflections. These functions are user specified and can be used to capture the nonlinearities of biologic structures.  This FunctionBasedBushing does not capture coupling between the deflections (e.g. force in x due to rotation about z).\n\nA bushing force is the resistive force due to deviation between two frames. One can think of the Bushing as being composed of 3 translational and 3 torsional spring-dampers, which act along or about the bushing frame axes. Orientations are measured as x-y-z body-fixed Euler rotations.",
        },
        {
            "McKibbenActuator",
            "McKibben Pneumatic Actuator Model based on the simple cylindrical formulation described in J. Dyn. Sys., Meas., Control 122, 386-388  (1998) (3 pages); doi:10.1115/1.482478.\n\nPressure is used as a control signal. There is an optional 'cord' attached to the actuator which allows for the path length of the actuator to be shorter than the total distance spanned by the points to which the actuator is connected. By default its length is zero. Please refer to the above paper for details regarding the rest of the properties.",
        },
        {
            "PathActuator",
            "This is the base class for actuators that apply controllable tension along a geometry path. %PathActuator has no states; the control is simply the tension to be applied along a geometry path (i.e. tensionable rope).",
        },
        {
            "PointActuator",
            "A class that implements a point actuator acting on the model. This actuator has no states; the control is simply the force to be applied to the model.",
        },
        {
            "PointToPointActuator",
            "A class that implements a force actuator acting between two points on two bodies. The direction of the force is along the line between the points, with a positive value acting to expand the distance between them. This actuator has no states; the control is simply the force to be applied to the model.",
        },
        {
            "PrescribedForce",
            "This applies to a PhysicalFrame a force and /or torque that is specified as a function of time. It is defined by three sets of functions, all of which are optional:\n\n    - Three functions that specify the (x,y,z) components of a force vector to apply (at a given point) as a function of time. If these functions are not provided, no force is applied.\n\n    - Three functions that specify the (x,y,z) components of a point location at which the force should be applied. If these functions are not provided, the force is applied at the frame's origin.\n\n    - Three functions that specify the (x,y,z) components of a pure torque vector to apply. This is in addition to any torque resulting from the applied force. If these functions are not provided, no additional torque is applied.",
        },
        {
            "SpringGeneralizedForce",
            "A Force that exerts a generalized force based on spring - like characteristics (stiffness and viscosity).",
        },
        {
            "ControlSetController",
            "ControlSetController that simply assigns controls from a ControlSet",
        },
        {
            "ToyReflexController",
            "ToyReflexController is a concrete controller that excites muscles in response to muscle lengthening to simulate a simple stretch reflex. This controller is meant to serve as an example how to implement a controller in OpenSim. It is intended for demonstration purposes only.",
        },
        {
            "ConstantDistanceConstraint",
            "A constraint that maintains a constant distance between two points on separate physical frames (underlying constraint: SimTK::Constraint::Rod)",
        },
    };
}

// fetch cached version of the above lookup
static std::unordered_map<osc::CStringView, osc::CStringView> const& GetDescriptionLut()
{
    static std::unordered_map<osc::CStringView, osc::CStringView> const s_Lut = CreateDescriptionLut();
    return s_Lut;
}

// creates a list of classes that shouldn't be presented to the user as addition options
//
// this is because they are known to be troublemakers that crash the UI
static std::unordered_set<std::string> CreateBlacklist()
{
    return
    {
        // it implicitly depends on having an owning joint and will explode when it tries to
        // get its associated joint (it doesn't declare this dependency via sockets)
        "Coordinate",

        // it requires creating generalized coordinate children, which gets hairy to automate
        // in the UI
        "CustomJoint",

        // it requires at least two path points, so it can't be default-constructed and added
        "GeometryPath",

        // it would cause all kinds of mayhem if a user could nest a model within a model
        "Model",

        // can't set it as the child of a geometry path automatically (no sockets)
        "PathWrap",

        // doesn't seem to add into the model at all - just hangs?
        "PositionMotion",

        // it has a constructor that depends on a `TaskSet` that OpenSim creator can't automatically
        // deduce (#526)
        "CMC",

        // wrap geometry crash the UI if the user adds it because they implicitly depend on `setFrame`
        // being called during `generateDecorations` they do not have an API-visible socket
        "WrapCylinder",
        "WrapEllipsoid",
        "WrapCylinderObst",
        "WrapDoubleCylinderObst",
        "WrapEllipsoid",
        "WrapObjectSet",
        "WrapSphere",
        "WrapSphereObst",
        "WrapTorus",

        // it's deprecated (#521)
        "Delp1990Muscle_Deprecated",

        // it's a base class: users should use concrete derived classes (#521)
        "PathActuator",

        // it's deprecated (#521)
        "Schutte1993Muscle_Deprecated",

        // it's deprecated (#521)
        "Thelen2003Muscle_Deprecated",

        // probably shouldn't allow two grounds in a model (#521)
        "Ground",

        // OpenSim Creator doesn't provide a way of specifying the required GeometryPath (#518)
        //
        // (can be un-blacklisted once #522 is implemented)
        "Blankevoort1991Ligament",
    };
}

// cached version of the above
static std::unordered_set<std::string> const& GetBlacklist()
{
    static std::unordered_set<std::string> const s_Blacklist = CreateBlacklist();
    return s_Blacklist;
}

// helper: add elements that derive from type T
template<typename T>
static void AddRegisteredElementsOfType(std::unordered_set<std::string>& out)
{
    OpenSim::ArrayPtrs<T> ptrs;
    OpenSim::Object::getRegisteredObjectsOfGivenType<T>(ptrs);

    for (int i = 0; i < ptrs.getSize(); ++i)
    {
        out.insert(ptrs[i]->getConcreteClassName());
    }
}

// create a set that contains all the components that are already assigned to
// a "group" in OSC
static std::unordered_set<std::string> CreateSetOfAllGroupedElements()
{
    std::unordered_set<std::string> rv;
    AddRegisteredElementsOfType<OpenSim::Joint>(rv);
    AddRegisteredElementsOfType<OpenSim::ContactGeometry>(rv);
    AddRegisteredElementsOfType<OpenSim::Constraint>(rv);
    AddRegisteredElementsOfType<OpenSim::Force>(rv);
    AddRegisteredElementsOfType<OpenSim::Controller>(rv);
    AddRegisteredElementsOfType<OpenSim::Probe>(rv);
    return rv;
}

// cached version of the above
static std::unordered_set<std::string> const& GetSetOfAllGroupedElements()
{
    static std::unordered_set<std::string> const s_GroupedEls = CreateSetOfAllGroupedElements();
    return s_GroupedEls;
}

// helper: construct a prototype joint and assign its coordinate names
template<typename TJoint>
static std::shared_ptr<TJoint> JointWithCoords(std::initializer_list<char const*> names)
{
    std::shared_ptr<TJoint> j = std::make_shared<TJoint>();
    int i = 0;
    for (char const* name : names)
    {
        j->upd_coordinates(i++).setName(name);
    }
    return j;
}

// create a lookup of pre-initialized prototype components
static std::unordered_map<osc::CStringView, std::shared_ptr<OpenSim::Component const>> CreatePrototypeLut()
{
    return
    {
        {
            "BallJoint",
            JointWithCoords<OpenSim::BallJoint>({"rx", "ry", "rz"}),
        },
        {
            "EllipsoidJoint",
            []()
            {
                auto joint = JointWithCoords<OpenSim::EllipsoidJoint>({"rx", "ry", "rz"});
                joint->updProperty_radii_x_y_z() = {1.0, 1.0, 1.0};
                return joint;
            }()
        },
        {
            "FreeJoint",
            JointWithCoords<OpenSim::FreeJoint>({"rx", "ry", "rz", "tx", "ty", "tz"}),
        },
        {
            "GimbalJoint",
            JointWithCoords<OpenSim::GimbalJoint>({"rx", "ry", "rz"}),
        },
        {
            "PinJoint",
            JointWithCoords<OpenSim::PinJoint>({"rz"}),
        },
        {
            "PlanarJoint",
            JointWithCoords<OpenSim::PlanarJoint>({"rz", "tx", "ty"}),
        },
        {
            "ScapulothoracicJoint",
            []()
            {
                auto joint = JointWithCoords<OpenSim::ScapulothoracicJoint>({"rx_abduction", "ry_elevation", "rz_upwardrotation", "ryp_winging"});
                joint->updProperty_thoracic_ellipsoid_radii_x_y_z() = {1.0, 1.0, 1.0};
                return joint;
            }()
        },
        {
            "SliderJoint",
            JointWithCoords<OpenSim::SliderJoint>({"tx"}),
        },
        {
            "UniversalJoint",
            JointWithCoords<OpenSim::UniversalJoint>({"rx", "ry"}),
        },
        {
            "WeldJoint",
            JointWithCoords<OpenSim::WeldJoint>({}),
        },
        {
            "HuntCrossleyForce",
            []()
            {
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
            "PathSpring",
            []()
            {
                auto ps = std::make_shared<OpenSim::PathSpring>();
                ps->setRestingLength(1.0);
                ps->setStiffness(1000.0);
                ps->setDissipation(0.5);
                return ps;
            }(),
        },
        {
            "ContactSphere",
            []()
            {
                auto cs = std::make_shared<OpenSim::ContactSphere>();
                cs->setRadius(1.0);
                return cs;
            }()
        },
        {
            "ConstantDistanceConstraint",
            []()
            {
                auto cdc = std::make_shared<OpenSim::ConstantDistanceConstraint>();
                cdc->setConstantDistance(1.0);
                return cdc;
            }()
        },

        // HOTFIX: set SpringGeneralizedForce's `coordinate` property to prevent an OpenSim 4.4 segfault (#524)
        {
            "SpringGeneralizedForce",
            []()
            {
                auto c = std::make_shared<OpenSim::SpringGeneralizedForce>();
                c->set_coordinate("");
                return c;
            }()
        },

        // HOTFIX: set `CoordinateCouplerConstraint`s `coupled_coordinates_function` property to prevent an OpenSim 4.4 segfault (#515)
        {
            "CoordinateCouplerConstraint",
            []()
            {
                auto c = std::make_shared<OpenSim::CoordinateCouplerConstraint>();
                c->setFunction(OpenSim::LinearFunction{1.0, 0.0});  // identity function
                return c;
            }()
        },

        // HOTFIX: set `ActivationCoordinateActuator`s `coordinate` property to prevent an OpenSim 4.4 segfault (#517)
        {
            "ActivationCoordinateActuator",
            []()
            {
                auto c = std::make_shared<OpenSim::ActivationCoordinateActuator>();
                c->set_coordinate("");
                return c;
            }()
        },

        // HOTFIX: set `ExpressionBasedPointToPointForce` body properties to prevent an OpenSim 4.4 segfault (#520)
        {
            "ExpressionBasedPointToPointForce",
            []()
            {
                auto c = std::make_shared<OpenSim::ExpressionBasedPointToPointForce>();
                c->set_body1("");
                c->set_body2("");
                return c;
            }()
        },

        // HOTFIX: set `PointToPointActuator`s body properties to prevent an OpenSim 4.4 segfault (#523)
        {
            "PointToPointActuator",
            []()
            {
                auto c = std::make_shared<OpenSim::PointToPointActuator>();
                c->set_bodyA("");
                c->set_bodyB("");
                return c;
            }()
        },
    };
}

static std::unordered_map<osc::CStringView, std::shared_ptr<OpenSim::Component const>> const& GetPrototypeLut()
{
    static std::unordered_map<osc::CStringView, std::shared_ptr<OpenSim::Component const>> const s_Lut = CreatePrototypeLut();
    return s_Lut;
}

template<typename T>
static std::vector<std::shared_ptr<T const>> CreatePrototypeLutT()
{
    OpenSim::ArrayPtrs<T> ptrs;
    OpenSim::Object::getRegisteredObjectsOfGivenType<T>(ptrs);

    std::vector<std::shared_ptr<T const>> rv;
    rv.reserve(ptrs.size());

    auto const& protoLut = GetPrototypeLut();
    auto const& blacklistLut = GetBlacklist();

    for (int i = 0; i < ptrs.size(); ++i)
    {
        T const& v = *ptrs[i];
        std::string const& name = v.getConcreteClassName();
        if (blacklistLut.find(name) != blacklistLut.end())
        {
            continue;  // it's a blacklisted component, hide it in the UI
        }

        auto it = protoLut.find(name);

        if (it != protoLut.end())
        {
            // it has already been manually created in the prototype LUT - use that
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
            // not in the manual prototype LUT - just take whatever OpenSim has
            rv.emplace_back(v.clone());
        }
    }

    osc::Sort(rv, osc::IsConcreteClassNameLexographicallyLowerThan<std::shared_ptr<OpenSim::Component const>>);

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
        auto it = lut.find(proto->getConcreteClassName());
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
    static std::vector<std::shared_ptr<OpenSim::Joint const>> const s_Protos = CreatePrototypeLutT<OpenSim::Joint>();
    return s_Protos;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Joint>::nameStrings() noexcept
{
    static std::vector<osc::CStringView> const s_Names = CreateNameViews(prototypes());
    return s_Names;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Joint>::nameCStrings() noexcept
{
    static std::vector<char const*> const s_CStrs = CreateCStrings(nameStrings());
    return s_CStrs;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Joint>::descriptionStrings() noexcept
{
    static std::vector<osc::CStringView> const s_Descriptions = CreateDescriptionViews(prototypes());
    return s_Descriptions;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Joint>::descriptionCStrings() noexcept
{
    static std::vector<char const*> const s_DescriptionCStrs = CreateCStrings(descriptionStrings());
    return s_DescriptionCStrs;
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
    static std::vector<std::shared_ptr<OpenSim::ContactGeometry const>> const s_Protos = CreatePrototypeLutT<OpenSim::ContactGeometry>();
    return s_Protos;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::ContactGeometry>::nameStrings() noexcept
{
    static std::vector<osc::CStringView> const s_Names = CreateNameViews(prototypes());
    return s_Names;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::ContactGeometry>::nameCStrings() noexcept
{
    static std::vector<char const*> const s_CStrs = CreateCStrings(nameStrings());
    return s_CStrs;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::ContactGeometry>::descriptionStrings() noexcept
{
    static std::vector<osc::CStringView> const s_Descriptions = CreateDescriptionViews(prototypes());
    return s_Descriptions;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::ContactGeometry>::descriptionCStrings() noexcept
{
    static std::vector<char const*> const s_DescriptionCStrs = CreateCStrings(descriptionStrings());
    return s_DescriptionCStrs;
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
    static std::vector<std::shared_ptr<OpenSim::Constraint const>> const s_Protos = CreatePrototypeLutT<OpenSim::Constraint>();
    return s_Protos;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Constraint>::nameStrings() noexcept
{
    static std::vector<osc::CStringView> const s_Names = CreateNameViews(prototypes());
    return s_Names;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Constraint>::nameCStrings() noexcept
{
    static std::vector<char const*> const s_CStrs = CreateCStrings(nameStrings());
    return s_CStrs;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Constraint>::descriptionStrings() noexcept
{
    static std::vector<osc::CStringView> const s_Descriptions = CreateDescriptionViews(prototypes());
    return s_Descriptions;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Constraint>::descriptionCStrings() noexcept
{
    static std::vector<char const*> const s_DescriptionCStrs = CreateCStrings(descriptionStrings());
    return s_DescriptionCStrs;
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
    static std::vector<std::shared_ptr<OpenSim::Force const>> const s_Protos = CreatePrototypeLutT<OpenSim::Force>();
    return s_Protos;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Force>::nameStrings() noexcept
{
    static std::vector<osc::CStringView> const s_Names = CreateNameViews(prototypes());
    return s_Names;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Force>::nameCStrings() noexcept
{
    static std::vector<char const*> const s_CStrs = CreateCStrings(nameStrings());
    return s_CStrs;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Force>::descriptionStrings() noexcept
{
    static std::vector<osc::CStringView> const s_Descriptions = CreateDescriptionViews(prototypes());
    return s_Descriptions;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Force>::descriptionCStrings() noexcept
{
    static std::vector<char const*> const s_DescriptionCStrs = CreateCStrings(descriptionStrings());
    return s_DescriptionCStrs;
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
    static std::vector<std::shared_ptr<OpenSim::Controller const>> const s_Protos = CreatePrototypeLutT<OpenSim::Controller>();
    return s_Protos;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Controller>::nameStrings() noexcept
{
    static std::vector<osc::CStringView> const s_Names = CreateNameViews(prototypes());
    return s_Names;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Controller>::nameCStrings() noexcept
{
    static std::vector<char const*> const s_CStrs = CreateCStrings(nameStrings());
    return s_CStrs;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Controller>::descriptionStrings() noexcept
{
    static std::vector<osc::CStringView> const s_Descriptions = CreateDescriptionViews(prototypes());
    return s_Descriptions;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Controller>::descriptionCStrings() noexcept
{
    static std::vector<char const*> const s_DescriptionCStrs = CreateCStrings(descriptionStrings());
    return s_DescriptionCStrs;
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
    static std::vector<std::shared_ptr<OpenSim::Probe const>> const s_Protos = CreatePrototypeLutT<OpenSim::Probe>();
    return s_Protos;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Probe>::nameStrings() noexcept
{
    static std::vector<osc::CStringView> const s_Names = CreateNameViews(prototypes());
    return s_Names;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Probe>::nameCStrings() noexcept
{
    static std::vector<char const*> const s_CStrs = CreateCStrings(nameStrings());
    return s_CStrs;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Probe>::descriptionStrings() noexcept
{
    static std::vector<osc::CStringView> const s_Descriptions = CreateDescriptionViews(prototypes());
    return s_Descriptions;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Probe>::descriptionCStrings() noexcept
{
    static std::vector<char const*> const s_DescriptionCStrs = CreateCStrings(descriptionStrings());
    return s_DescriptionCStrs;
}

template<>
std::optional<size_t> osc::TypeRegistry<OpenSim::Probe>::indexOf(OpenSim::Probe const& cg) noexcept
{
    return ::IndexOf(prototypes(), typeid(cg));
}


// TypeRegistry<OpenSim::Component>

static std::vector<std::shared_ptr<OpenSim::Component const>> CreateOtherComponentLut()
{
    std::unordered_set<std::string> const& grouped = GetSetOfAllGroupedElements();
    std::unordered_set<std::string> const& blacklisted = GetBlacklist();

    OpenSim::ArrayPtrs<OpenSim::ModelComponent> ptrs;
    OpenSim::Object::getRegisteredObjectsOfGivenType<OpenSim::ModelComponent>(ptrs);

    std::vector<std::shared_ptr<OpenSim::Component const>> rv;

    for (int i = 0; i < ptrs.size(); ++i)
    {
        OpenSim::Component const& c = *ptrs[i];
        std::string const& classname = c.getConcreteClassName();

        if (osc::Contains(blacklisted, classname))
        {
            // it's blacklisted in the UI
            continue;
        }

        if (osc::Contains(grouped, c.getConcreteClassName()))
        {
            // it's already grouped
            continue;
        }

        rv.emplace_back(c.clone());
    }

    osc::Sort(rv, osc::IsConcreteClassNameLexographicallyLowerThan<std::shared_ptr<OpenSim::Component const>>);

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
    static std::vector<std::shared_ptr<OpenSim::Component const>> const s_Protos = CreateOtherComponentLut();
    return s_Protos;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Component>::nameStrings() noexcept
{
    static std::vector<osc::CStringView> const s_Names = CreateNameViews(prototypes());
    return s_Names;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Component>::nameCStrings() noexcept
{
    static std::vector<char const*> const s_CStrs = CreateCStrings(nameStrings());
    return s_CStrs;
}

template<>
nonstd::span<osc::CStringView const> osc::TypeRegistry<OpenSim::Component>::descriptionStrings() noexcept
{
    static std::vector<osc::CStringView> const s_Descriptions = CreateDescriptionViews(prototypes());
    return s_Descriptions;
}

template<>
nonstd::span<char const* const> osc::TypeRegistry<OpenSim::Component>::descriptionCStrings() noexcept
{
    static std::vector<char const*> const s_DescriptionCStrs = CreateCStrings(descriptionStrings());
    return s_DescriptionCStrs;
}

template<>
std::optional<size_t> osc::TypeRegistry<OpenSim::Component>::indexOf(OpenSim::Component const& cg) noexcept
{
    return ::IndexOf(prototypes(), typeid(cg));
}
