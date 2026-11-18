#pragma once

#include <liboscar/utilities/c_string_view.h>

namespace osc
{
    // a collection of user-facing strings that are used by the mesh importer
    class MiStrings final {
    public:
        static constexpr CStringView c_ground_label = "Ground";
        static constexpr CStringView c_ground_label_pluralized = "Ground";
        static constexpr CStringView c_ground_label_optionally_pluralized = "Ground(s)";
        static constexpr CStringView c_ground_description =
            "Ground is an inertial reference frame in which the motion of all frames and points "
            "may conveniently and efficiently be expressed. It is always defined to be at (0, 0, "
            "0) in 'world space' and cannot move. All bodies in the model must eventually attach "
            "to ground via joints.";

        static constexpr CStringView c_mesh_label = "Mesh";
        static constexpr CStringView c_mesh_label_pluralized = "Meshes";
        static constexpr CStringView c_mesh_label_optionally_pluralized = "Mesh(es)";
        static constexpr CStringView c_mesh_description =
            "Meshes are decorational components in the model. They can be translated, rotated, and "
            "scaled. Typically, meshes are 'attached' to other objects in the model, such as "
            "bodies. When meshes are 'attached' to something, they will 'follow' the thing they "
            "are attached to.";
        static constexpr CStringView c_mesh_attachment_crossref_name = "parent";

        static constexpr CStringView c_body_label = "Body";
        static constexpr CStringView c_body_label_pluralized = "Bodies";
        static constexpr CStringView c_body_label_optionally_pluralized = "Body(s)";
        static constexpr CStringView c_body_description =
            "Bodies are active elements in the model. They define a 'frame' (effectively, a "
            "location + orientation) with a mass.\n\nOther body properties (e.g. inertia) can be "
            "edited in the main OpenSim Creator editor after you have converted the model into an "
            "OpenSim model.";

        static constexpr CStringView c_joint_label = "Joint";
        static constexpr CStringView c_joint_label_pluralized = "Joints";
        static constexpr CStringView c_joint_label_optionally_pluralized = "Joint(s)";
        static constexpr CStringView c_joint_description =
            "Joints connect two physical frames (i.e. bodies and ground) together and specifies "
            "their relative permissible motion (e.g. PinJoints only allow rotation along one "
            "axis).\n\nIn OpenSim, joints are the 'edges' of a directed topology graph where "
            "bodies are the 'nodes'. All bodies in the model must ultimately connect to ground via "
            "joints.";
        static constexpr CStringView c_joint_parent_crossref_name = "parent";
        static constexpr CStringView c_joint_child_crossref_name = "child";

        static constexpr CStringView c_station_label = "Station";
        static constexpr CStringView c_station_label_pluralized = "Stations";
        static constexpr CStringView c_station_label_optionally_pluralized = "Station(s)";
        static constexpr CStringView c_station_description =
            "Stations are points of interest in the model. They can be used to compute a 3D "
            "location in the frame of the thing they are attached to.\n\nThe utility of stations "
            "is that you can use them to visually mark points of interest. Those points of "
            "interest will then be defined with respect to whatever they are attached to. This is "
            "useful because OpenSim typically requires relative coordinates for things in the "
            "model (e.g. muscle paths).";
        static constexpr CStringView c_station_parent_crossref_name = "parent";

        static constexpr CStringView c_translation_description =
            "Translation of the component in ground. OpenSim defines this as 'unitless'; however, "
            "OpenSim models typically use meters.";
    };
}
