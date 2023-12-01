#pragma once

#include <oscar/Utils/CStringView.hpp>

namespace osc::mi
{
    // a collection of user-facing strings that are presented in the model graph
    class ModelGraphStrings final {
    public:
        static constexpr CStringView c_GroundLabel = "Ground";
        static constexpr CStringView c_GroundLabelPluralized = "Ground";
        static constexpr CStringView c_GroundLabelOptionallyPluralized = "Ground(s)";
        static constexpr CStringView c_GroundDescription = "Ground is an inertial reference frame in which the motion of all frames and points may conveniently and efficiently be expressed. It is always defined to be at (0, 0, 0) in 'worldspace' and cannot move. All bodies in the model must eventually attach to ground via joints.";

        static constexpr CStringView c_MeshLabel = "Mesh";
        static constexpr CStringView c_MeshLabelPluralized = "Meshes";
        static constexpr CStringView c_MeshLabelOptionallyPluralized = "Mesh(es)";
        static constexpr CStringView c_MeshDescription = "Meshes are decorational components in the model. They can be translated, rotated, and scaled. Typically, meshes are 'attached' to other elements in the model, such as bodies. When meshes are 'attached' to something, they will 'follow' the thing they are attached to.";
        static constexpr CStringView c_MeshAttachmentCrossrefName = "parent";

        static constexpr CStringView c_BodyLabel = "Body";
        static constexpr CStringView c_BodyLabelPluralized = "Bodies";
        static constexpr CStringView c_BodyLabelOptionallyPluralized = "Body(s)";
        static constexpr CStringView c_BodyDescription = "Bodies are active elements in the model. They define a 'frame' (effectively, a location + orientation) with a mass.\n\nOther body properties (e.g. inertia) can be edited in the main OpenSim Creator editor after you have converted the model into an OpenSim model.";

        static constexpr CStringView c_JointLabel = "Joint";
        static constexpr CStringView c_JointLabelPluralized = "Joints";
        static constexpr CStringView c_JointLabelOptionallyPluralized = "Joint(s)";
        static constexpr CStringView c_JointDescription = "Joints connect two physical frames (i.e. bodies and ground) together and specifies their relative permissible motion (e.g. PinJoints only allow rotation along one axis).\n\nIn OpenSim, joints are the 'edges' of a directed topology graph where bodies are the 'nodes'. All bodies in the model must ultimately connect to ground via joints.";
        static constexpr CStringView c_JointParentCrossrefName = "parent";
        static constexpr CStringView c_JointChildCrossrefName = "child";

        static constexpr CStringView c_StationLabel = "Station";
        static constexpr CStringView c_StationLabelPluralized = "Stations";
        static constexpr CStringView c_StationLabelOptionallyPluralized = "Station(s)";
        static constexpr CStringView c_StationDescription = "Stations are points of interest in the model. They can be used to compute a 3D location in the frame of the thing they are attached to.\n\nThe utility of stations is that you can use them to visually mark points of interest. Those points of interest will then be defined with respect to whatever they are attached to. This is useful because OpenSim typically requires relative coordinates for things in the model (e.g. muscle paths).";
        static constexpr CStringView c_StationParentCrossrefName = "parent";

        static constexpr CStringView c_TranslationDescription = "Translation of the component in ground. OpenSim defines this as 'unitless'; however, OpenSim models typically use meters.";
    };
}
