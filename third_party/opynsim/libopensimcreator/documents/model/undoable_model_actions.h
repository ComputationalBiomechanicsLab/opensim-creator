#pragma once

#include <libopynsim/documents/landmarks/named_landmark.h>
#include <liboscar/maths/euler_angles.h>
#include <liboscar/maths/vector.h>

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace OpenSim
{
    class Component;
    class ComponentPath;
    class ContactGeometry;
    class Coordinate;
    class Geometry;
    class GeometryPath;
    class Joint;
    class Marker;
    class Mesh;
    class Model;
    class Object;
    class PathPoint;
    class PhysicalFrame;
    class PhysicalOffsetFrame;
    class Scholz2015GeometryPathObstacle;
    class Station;
    class WrapObject;
}

namespace opyn
{
    class ModelStatePair;
}

namespace osc
{
    class ModelStatePairWithSharedEnvironment;
    class ObjectPropertyEdit;
    class SceneCache;
    class UndoableModelStatePair;
    class Widget;

    // prompt the user for a save location and then save the model to the specified location
    void action_save_current_model_as(const std::shared_ptr<opyn::ModelStatePair>&);

    // create a new model and show it in a new tab
    void action_new_model(Widget& parent);

    // prompt a user to open a model file and open it in a new tab
    void action_open_model(Widget&);

    // open the specified model in a loading tab
    void action_open_model(Widget&, const std::filesystem::path&);

    // Tries to save the given model, potentially asynchronously, as an on-disk file
    // then calls `callback` with `true` if the save was successful; otherwise, calls it
    // with `false`.
    void action_save_model_async(
        const std::shared_ptr<opyn::ModelStatePair>&,
        std::function<void(bool)> callback = [](bool) {});

    // try to delete an undoable-model's current selection
    //
    // "try", because some things are difficult to delete from OpenSim models
    void action_try_delete_selection_from_edited_model(opyn::ModelStatePair&);

    // disable all wrapping surfaces in the current model
    void action_disable_all_wrapping_surfaces(opyn::ModelStatePair&);

    // enable all wrapping surfaces in the current model
    void action_enable_all_wrapping_surfaces(opyn::ModelStatePair&);

    // loads an STO file against the current model and opens it in a new tab
    bool action_load_sto_file_against_model(
        Widget&, const ModelStatePairWithSharedEnvironment&, const std::filesystem::path& sto_path);

    // start simulating the given model in a forward-dynamic simulator tab
    bool action_start_simulating_model(Widget&, const ModelStatePairWithSharedEnvironment&);

    // reload the given model from its backing file (if applicable)
    bool action_update_model_from_backing_file(UndoableModelStatePair&);

    // copies the full absolute path to the osim to the clipboard
    bool action_copy_model_path_to_clipboard(const opyn::ModelStatePair&);

    // try to automatically set the model's scale factor based on how big the scene is
    bool action_autoscale_scene_scale_factor(opyn::ModelStatePair&);

    // toggle model frame visibility
    bool action_toggle_frames(opyn::ModelStatePair&);

    // toggle model marker visibility
    bool action_toggle_markers(opyn::ModelStatePair&);

    // toggle contact geometry visibility
    bool action_toggle_contact_geometry(opyn::ModelStatePair&);

    // toggle force visibility (#887)
    bool action_toggle_forces(opyn::ModelStatePair&);

    // toggle model wrap geometry visibility
    bool action_toggle_wrap_geometry(opyn::ModelStatePair&);

    // open the parent directory of the model's backing file (if applicable) in a file explorer
    bool action_open_osim_parent_directory(const OpenSim::Model&);

    // open the model's backing file (if applicable) in an OS-determined default for osims
    bool action_open_osim_in_external_editor(const OpenSim::Model&);

    // force a reload of the model, and its associated assets, from its backing file
    bool action_reload_osim_from_disk(UndoableModelStatePair&, SceneCache&);

    // add an offset frame to the current selection (if applicable)
    bool action_add_offset_frame_to_physical_frame(
        opyn::ModelStatePair&, const OpenSim::ComponentPath& phys_frame_path);

    // returns true if the specified joint (if valid) can be re-zeroed
    bool can_rezero_joint(opyn::ModelStatePair&, const OpenSim::ComponentPath& joint_path);

    // re-zeroes the selected joint (if applicable)
    bool action_rezero_joint(opyn::ModelStatePair&, const OpenSim::ComponentPath& joint_path);

    // adds a parent offset frame to the selected joint (if applicable)
    bool action_add_parent_offset_frame_to_joint(
        opyn::ModelStatePair&, const OpenSim::ComponentPath& joint_path);

    // adds a child offset frame to the selected joint (if applicable)
    bool action_add_child_offset_frame_to_joint(
        opyn::ModelStatePair&, const OpenSim::ComponentPath& joint_path);

    // sets the name of the selected component (if applicable)
    bool action_set_component_name(
        opyn::ModelStatePair&, const OpenSim::ComponentPath& component_path, const std::string&);

    // changes the type of the selected joint (if applicable) to the provided joint
    bool action_change_joint_type_to(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& joint_path,
        std::unique_ptr<OpenSim::Joint>);

    // attaches geometry to the selected physical frame (if applicable)
    bool action_attach_geometry_to_physical_frame(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& phys_frame_path,
        std::unique_ptr<OpenSim::Geometry>);

    // assigns contact geometry to the selected HCF (if applicable)
    bool action_assign_contact_geometry_to_hcf(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& hcf_path,
        const OpenSim::ComponentPath& contact_geom_path);

    // applies a property edit to the model
    bool action_apply_property_edit(opyn::ModelStatePair&, ObjectPropertyEdit&);

    // adds a path point to the selected geometry path
    bool action_add_path_point_to_geometry_path(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& geometry_path_path,
        const OpenSim::ComponentPath& point_phys_frame);

    // adds a path point to the selected path actuator (if applicable)
    bool action_add_path_point_to_path_actuator(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& path_actuator_path,
        const OpenSim::ComponentPath& point_phys_frame);

    // flags for reassignment
    enum class SocketReassignmentFlags {
        None = 0,
        TryReexpressComponentInNewConnectee = 1 << 0,
    };
    constexpr bool operator&(SocketReassignmentFlags lhs, SocketReassignmentFlags rhs)
    {
        return (std::to_underlying(lhs) & std::to_underlying(rhs)) != 0;
    }

    // attempts to reassign a component's socket connection (returns false and writes to `error` on
    // failure)
    bool action_reassign_component_socket(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& component_abs_path,
        const std::string& socket_name,
        const OpenSim::Object& connectee,
        SocketReassignmentFlags flags,
        std::string& error);

    // details of a body that should be added to a model
    struct BodyDetails final {

        BodyDetails();

        Vector3 center_of_mass;
        Vector3 inertia{1.0f};
        float mass = 1.0f;
        std::string parent_frame_abs_path;
        std::string body_name = "new_body";
        size_t joint_type_index;
        std::string joint_name;
        std::unique_ptr<OpenSim::Geometry> maybe_geometry;
        bool add_offset_frames = true;
    };

    // add a new body to the model
    bool action_add_body_to_model(opyn::ModelStatePair&, const BodyDetails&);

    // add the given component into the model graph, or throw
    bool action_add_component_to_model(opyn::ModelStatePair&, std::unique_ptr<OpenSim::Component>);
    bool action_add_component_to_model(
        opyn::ModelStatePair&,
        std::unique_ptr<OpenSim::Component>,
        const OpenSim::ComponentPath& desired_parent);

    // add the given `OpenSim::WrapObject` to the `WrapObjectSet` of
    // the `OpenSim::PhysicalFrame` located at `physicalFramePath`
    bool action_add_wrap_object_to_physical_frame(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& physical_frame_path,
        std::unique_ptr<OpenSim::WrapObject> wrap_obj_ptr);

    // add the given `OpenSim::WrapObject` to the `OpenSim::GeometryPath`'s
    // wrap object set, which makes the path wrap around the wrap object
    bool action_add_wrap_object_to_geometry_path_wraps(
        opyn::ModelStatePair&, const OpenSim::GeometryPath&, const OpenSim::WrapObject&);

    // remove the given `OpenSim::WrapObject` from the `OpenSim::GeometryPath`'s
    // wrap object set
    //
    // does nothing if the `OpenSim::WrapObject` isn't in the path's wrap set
    bool action_remove_wrap_object_from_geometry_path_wraps(
        opyn::ModelStatePair&, const OpenSim::GeometryPath&, const OpenSim::WrapObject&);

    // Zeroes all `OpenSim::Coordinates` in the model. If a coordinate is clamped
    // then this tries to get as close to zero as possible while obeying the clamp
    bool action_zero_all_coordinates(opyn::ModelStatePair&);

    // set the speed of a coordinate
    bool action_set_coordinate_speed(
        opyn::ModelStatePair&, const OpenSim::Coordinate&, double new_speed);

    // set the speed of a coordinate and ensure it is saved
    bool action_set_coordinate_speed_and_save(
        opyn::ModelStatePair&, const OpenSim::Coordinate&, double new_speed);

    // set a coordinate (un)locked
    bool action_set_coordinate_locked_and_save(
        opyn::ModelStatePair&, const OpenSim::Coordinate&, bool);

    // set the value of a coordinate
    bool action_set_coordinate_value(
        opyn::ModelStatePair&, const OpenSim::Coordinate&, double new_value);

    // set the value of a coordinate and ensure it is saved
    bool action_set_coordinate_value_and_save(
        opyn::ModelStatePair&, const OpenSim::Coordinate&, double new_value);

    // sets the `Appearance` property of the pointed-to component, and all its children, to have
    // visible = bool
    bool action_set_component_and_all_childrens_is_visible_to(
        opyn::ModelStatePair&, const OpenSim::ComponentPath&, bool new_visibility);

    // sets the `Appearance` property of all components in the model to `visible = false`, followed
    // by setting the `Appearance` property of the pointed-to component, and all its children, to
    // `visible = true`
    bool action_show_only_component_and_all_children(
        opyn::ModelStatePair&, const OpenSim::ComponentPath&);

    // sets the `Appearance` property of all components in the model to `visible = visible` if that
    // component has the given concrete class name
    bool action_set_component_and_all_children_with_given_concrete_class_name_is_visible_to(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath&,
        std::string_view concrete_class_name,
        bool new_visibility);

    // sets the location of the given station in its parent frame to its old location plus the
    // provided delta
    //
    // (does not save the change to undo/redo storage)
    bool action_translate_station(
        opyn::ModelStatePair&, const OpenSim::Station&, const Vector3& delta_position);

    // sets the location of the given station in its parent frame to its old location plus the
    // provided vector
    //
    // (saves the change to undo/redo storage)
    bool action_translate_station_and_save(
        opyn::ModelStatePair&, const OpenSim::Station&, const Vector3& delta_position);

    // sets the location of the given path point in its parent frame to its old location plus the
    // provided delta
    //
    // (does not save the change to undo/redo storage)
    bool action_translate_path_point(
        opyn::ModelStatePair&, const OpenSim::PathPoint&, const Vector3& delta_position);

    // sets the location of the given path point in its parent frame to its old location plus the
    // provided delta
    //
    // (saves the change to undo/redo storage)
    bool action_translate_path_point_and_save(
        opyn::ModelStatePair&, const OpenSim::PathPoint&, const Vector3& delta_position);

    bool action_transform_pof_v2(
        opyn::ModelStatePair&,
        const OpenSim::PhysicalOffsetFrame&,
        const Vector3& new_translation,
        const EulerAngles& new_eulers);
    bool action_transform_wrap_object(
        opyn::ModelStatePair&,
        const OpenSim::WrapObject&,
        const Vector3& delta_position,
        const EulerAngles& new_eulers);
    bool action_transform_contact_geometry(
        opyn::ModelStatePair&,
        const OpenSim::ContactGeometry&,
        const Vector3& delta_position,
        const EulerAngles& new_eulers);
    bool action_fit_sphere_to_mesh(opyn::ModelStatePair&, const OpenSim::Mesh&);
    bool action_fit_ellipsoid_to_mesh(opyn::ModelStatePair&, const OpenSim::Mesh&);
    bool action_fit_plane_to_mesh(opyn::ModelStatePair&, const OpenSim::Mesh&);
    bool action_import_landmarks(
        opyn::ModelStatePair&,
        std::span<const opyn::NamedLandmark>,
        std::optional<std::string> maybe_name,
        std::optional<std::string> maybe_target_frame_abs_path);
    void action_export_model_graph_to_dotviz(const std::shared_ptr<opyn::ModelStatePair>&);
    bool action_export_model_graph_to_dotviz_clipboard(const OpenSim::Model&);
    bool action_export_model_multibody_system_as_dotviz(const OpenSim::Model&);
    bool action_bake_station_defined_frames(opyn::ModelStatePair&);
    bool action_move_marker_to_model_marker_set(opyn::ModelStatePair&, const OpenSim::Marker&);

    bool action_translate_contact_hint(
        opyn::ModelStatePair&,
        const OpenSim::Scholz2015GeometryPathObstacle&,
        const Vector3& delta_position);
    bool action_translate_contact_hint_and_save(
        opyn::ModelStatePair&,
        const OpenSim::Scholz2015GeometryPathObstacle&,
        const Vector3& delta_position);
}
