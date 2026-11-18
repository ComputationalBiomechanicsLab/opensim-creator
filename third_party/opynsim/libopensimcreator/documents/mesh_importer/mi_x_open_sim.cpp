#include "mi_x_open_sim.h"

#include <libopensimcreator/documents/mesh_importer/mi_body.h>
#include <libopensimcreator/documents/mesh_importer/mi_document.h>
#include <libopensimcreator/documents/mesh_importer/mi_document_helpers.h>
#include <libopensimcreator/documents/mesh_importer/mi_joint.h>
#include <libopensimcreator/documents/mesh_importer/mi_mesh.h>
#include <libopensimcreator/documents/mesh_importer/mi_ids.h>
#include <libopensimcreator/documents/mesh_importer/mi_station.h>

#include <libopynsim/component_registry/component_registry.h>
#include <libopynsim/component_registry/static_component_registries.h>
#include <libopynsim/graphics/simbody_mesh_loader.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <libopynsim/utilities/simbody_x_oscar.h>
#include <liboscar/graphics/mesh.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/matrix4x4.h>
#include <liboscar/maths/matrix_functions.h>
#include <liboscar/maths/transform.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/log.h>
#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/uid.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Simulation/Model/AbstractPathPoint.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Ground.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/OffsetFrame.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/WeldJoint.h>
#include <SimTKcommon/SmallMatrix.h>

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

using namespace osc;

namespace
{
    // stand-in method that should be replaced by actual support for scale-less transforms
    // (dare i call them.... frames ;))
    Transform ignore_scale(const Transform& t)
    {
        return t.with_scale(1.0f);
    }

    // attaches a mesh to a parent `OpenSim::PhysicalFrame` that is part of an `OpenSim::Model`
    void attach_mesh_el_to_frame(
        const MiMesh& mesh_el,
        const Transform& parent_xform,
        OpenSim::PhysicalFrame& parent_phys_frame)
    {
        // create a POF that attaches to the body
        auto mesh_phys_offset_frame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        mesh_phys_offset_frame->setParentFrame(parent_phys_frame);
        mesh_phys_offset_frame->setName(std::string{mesh_el.get_label()} + "_offset");

        // set the POFs transform to be equivalent to the mesh's (in-ground) transform,
        // but in the parent frame
        const auto mesh2ground = to<SimTK::Transform>(mesh_el.get_x_form());
        const auto parent2ground = to<SimTK::Transform>(parent_xform);
        mesh_phys_offset_frame->setOffsetTransform(parent2ground.invert() * mesh2ground);

        // attach the mesh data to the transformed POF
        auto mesh = std::make_unique<OpenSim::Mesh>(mesh_el.get_path().string());
        mesh->setName(std::string{mesh_el.get_label()});
        mesh->set_scale_factors(to<SimTK::Vec3>(mesh_el.get_x_form().scale));
        opyn::attach_geometry(*mesh_phys_offset_frame, std::move(mesh));

        // make it a child of the parent's physical frame
        opyn::add_component(parent_phys_frame, std::move(mesh_phys_offset_frame));
    }

    // create a body for the `model`, but don't add it to the model yet
    //
    // *may* add any attached meshes to the model, though
    std::unique_ptr<OpenSim::Body> create_detatched_body(
        const MiDocument& doc, const MiBody& body_el)
    {
        auto added_body = std::make_unique<OpenSim::Body>();

        added_body->setName(std::string{body_el.get_label()});
        added_body->setMass(body_el.get_mass());

        // set the inertia of the emitted body to be nonzero
        //
        // the reason we do this is because having a zero inertia on a body can cause
        // the simulator to freak out in some scenarios.
        {
            const double moment = 0.01 * body_el.get_mass();
            const SimTK::Vec3 moments{moment, moment, moment};
            const SimTK::Vec3 products{0.0, 0.0, 0.0};
            added_body->setInertia(SimTK::Inertia{moments, products});
        }

        // connect meshes to the body, if necessary
        //
        // the body's orientation is going to be handled when the joints are added (by adding
        // relevant offset frames etc.)
        for (const MiMesh& mesh : doc.iter<MiMesh>()) {
            if (mesh.get_parent_id() == body_el.get_id()) {
                attach_mesh_el_to_frame(mesh, body_el.get_x_form(), *added_body);
            }
        }

        return added_body;
    }

    // result of a lookup for (effectively) a physicalframe
    struct JointAttachmentCachedLookupResult final {

        // can be nullptr (indicating Ground)
        const MiBody* bodyEl = nullptr;

        // can be nullptr (indicating ground/cache hit)
        std::unique_ptr<OpenSim::Body> createdBody;

        // always != nullptr, can point to `createdBody`, or an existing body from the cache, or
        // Ground
        OpenSim::PhysicalFrame* physicalFrame = nullptr;
    };

    // cached lookup of a physical frame
    //
    // if the frame/body doesn't exist yet, constructs it
    JointAttachmentCachedLookupResult lookup_phys_frame(
        const MiDocument& doc,
        OpenSim::Model& model,
        std::unordered_map<UID, OpenSim::Body*>& visited_bodies,
        UID el_id)
    {
        // figure out what the parent body is. There's 3 possibilities:
        //
        // - null (ground)
        // - found, visited before (get it, but don't make it or add it to the model)
        // - found, not visited before (make it, add it to the model, cache it)

        JointAttachmentCachedLookupResult rv;
        rv.bodyEl = doc.try_get_by_id<MiBody>(el_id);
        rv.createdBody = nullptr;
        rv.physicalFrame = nullptr;

        if (rv.bodyEl) {
            const auto it = visited_bodies.find(el_id);
            if (it == visited_bodies.end()) {
                // haven't visited the body before
                rv.createdBody = create_detatched_body(doc, *rv.bodyEl);
                rv.physicalFrame = rv.createdBody.get();

                // add it to the cache
                visited_bodies.emplace(el_id, rv.createdBody.get());
            } else {
                // visited the body before, use cached result
                rv.createdBody = nullptr;  // it's not this function's responsibility to add it
                rv.physicalFrame = it->second;
            }
        } else {
            // the object is connected to ground
            rv.createdBody = nullptr;
            rv.physicalFrame = &model.updGround();
        }

        return rv;
    }

    // compute the name of a joint from its attached frames
    std::string calc_joint_name(
        const MiJoint& joint_el,
        const OpenSim::PhysicalFrame& parent_frame,
        const OpenSim::PhysicalFrame& child_frame)
    {
        if (!joint_el.get_user_assigned_name().empty()) {
            return std::string{joint_el.get_user_assigned_name()};
        } else {
            return child_frame.getName() + "_to_" + parent_frame.getName();
        }
    }

    // expresses if a joint has a degree of freedom (i.e. != -1) and the coordinate index of
    // that degree of freedom
    struct JointDegreesOfFreedom final {
        std::array<int, 3> orientation = {-1, -1, -1};
        std::array<int, 3> translation = {-1, -1, -1};
    };

    // returns the indices of each degree of freedom that the joint supports
    JointDegreesOfFreedom get_degrees_of_freedom(const OpenSim::Joint& joint)
    {
        if (dynamic_cast<const OpenSim::FreeJoint*>(&joint)) {
            return JointDegreesOfFreedom{{0, 1, 2}, {3, 4, 5}};
        } else if (dynamic_cast<const OpenSim::PinJoint*>(&joint)) {
            return JointDegreesOfFreedom{{-1, -1, 0}, {-1, -1, -1}};
        } else {
            return JointDegreesOfFreedom{};  // unknown joint type
        }
    }

    // sets the names of a joint's coordinates
    void set_joint_coordinate_names(OpenSim::Joint& joint, const std::string& prefix)
    {
        constexpr auto c_translation_names = std::to_array({"_tx", "_ty", "_tz"});
        constexpr auto c_rotation_names = std::to_array({"_rx", "_ry", "_rz"});

        const JointDegreesOfFreedom dofs = get_degrees_of_freedom(joint);

        // translations
        for (int i = 0; i < 3; ++i) {
            if (dofs.translation[i] != -1) {
                joint.upd_coordinates(dofs.translation[i]).setName(prefix + c_translation_names[i]);
            }
        }

        // rotations
        for (int i = 0; i < 3; ++i) {
            if (dofs.orientation[i] != -1) {
                joint.upd_coordinates(dofs.orientation[i]).setName(prefix + c_rotation_names[i]);
            }
        }
    }

    // recursively attaches `joint` to `model` by:
    //
    // - adding child bodies, if necessary
    // - adding an offset frames for each side of the joint
    // - computing relevant offset values for the offset frames, to ensure the bodies/joint-center
    //   end up in the right place
    // - setting the joint's default coordinate values based on any differences
    // - RECURSING by figuring out which joints have this joint's child as a parent
    void attach_joint_recursive(
        const MiDocument& doc,
        OpenSim::Model& model,
        const MiJoint& joint,
        std::unordered_map<UID, OpenSim::Body*>& visited_bodies,
        std::unordered_set<UID>& visited_joints)
    {
        {
            const bool was_inserted = visited_joints.emplace(joint.get_id()).second;
            if (!was_inserted) {
                // graph cycle detected: joint was already previously visited and shouldn't be
                // traversed again
                return;
            }
        }

        // lookup each side of the joint, creating the bodies if necessary
        const JointAttachmentCachedLookupResult parent =
            lookup_phys_frame(doc, model, visited_bodies, joint.get_parent_id());
        JointAttachmentCachedLookupResult child =
            lookup_phys_frame(doc, model, visited_bodies, joint.get_child_id());

        // create the parent OpenSim::PhysicalOffsetFrame
        auto parent_pof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        parent_pof->setName(parent.physicalFrame->getName() + "_offset");
        parent_pof->setParentFrame(*parent.physicalFrame);
        Matrix4x4 to_parent_pof_in_parent =
            inverse_matrix4x4_cast(ignore_scale(doc.get_x_form_by_id(joint.get_parent_id()))) *
            matrix4x4_cast(ignore_scale(joint.get_x_form()));
        parent_pof->set_translation(to<SimTK::Vec3>(Vector3{to_parent_pof_in_parent[3]}));
        parent_pof->set_orientation(to<SimTK::Vec3>(extract_eulers_xyz(to_parent_pof_in_parent)));

        // create the child OpenSim::PhysicalOffsetFrame
        auto child_pof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        child_pof->setName(child.physicalFrame->getName() + "_offset");
        child_pof->setParentFrame(*child.physicalFrame);
        const Matrix4x4 to_child_pof_in_child =
            inverse_matrix4x4_cast(ignore_scale(doc.get_x_form_by_id(joint.get_child_id()))) *
            matrix4x4_cast(ignore_scale(joint.get_x_form()));
        child_pof->set_translation(to<SimTK::Vec3>(Vector3{to_child_pof_in_child[3]}));
        child_pof->set_orientation(to<SimTK::Vec3>(extract_eulers_xyz(to_child_pof_in_child)));

        // create a relevant OpenSim::Joint (based on the type index, e.g. could be a FreeJoint)
        auto joint_uniq_ptr = opyn::get_component_registry<OpenSim::Joint>()
                                  .entry_with_classname(joint.get_specific_type_name())
                                  .instantiate();

        // set its name
        const std::string joint_name =
            calc_joint_name(joint, *parent.physicalFrame, *child.physicalFrame);
        joint_uniq_ptr->setName(joint_name);

        // set joint coordinate names
        set_joint_coordinate_names(*joint_uniq_ptr, joint_name);

        // add + connect the joint to the POFs
        //
        // care: ownership change happens here (#642)
        OpenSim::PhysicalOffsetFrame& parent_ref =
            opyn::add_frame(*joint_uniq_ptr, std::move(parent_pof));
        const OpenSim::PhysicalOffsetFrame& child_ref =
            opyn::add_frame(*joint_uniq_ptr, std::move(child_pof));
        joint_uniq_ptr->connectSocket_parent_frame(parent_ref);
        joint_uniq_ptr->connectSocket_child_frame(child_ref);

        // if a child body was created during this step (e.g. because it's not a cyclic connection)
        // then add it to the model
        OSC_ASSERT_ALWAYS(
            parent.createdBody == nullptr &&
            "at this point in the algorithm, all parents should have already been created");
        if (child.createdBody) {
            opyn::add_body(model, std::move(child.createdBody));  // add created body to model
        }

        // add the joint to the model
        opyn::add_joint(model, std::move(joint_uniq_ptr));

        // if there are any meshes attached to the joint, attach them to the parent
        for (const MiMesh& mesh : doc.iter<MiMesh>()) {
            if (mesh.get_parent_id() == joint.get_id()) {
                attach_mesh_el_to_frame(mesh, joint.get_x_form(), parent_ref);
            }
        }

        // recurse by finding where the child of this joint is the parent of some other joint
        OSC_ASSERT_ALWAYS(
            child.bodyEl != nullptr && "child should always be an identifiable body object");
        for (const MiJoint& other_joint : doc.iter<MiJoint>()) {
            if (other_joint.get_parent_id() == child.bodyEl->get_id()) {
                attach_joint_recursive(doc, model, other_joint, visited_bodies, visited_joints);
            }
        }
    }

    // attaches `BodyEl` into `model` by directly attaching it to ground with a WeldJoint
    void attach_body_directly_to_ground(
        const MiDocument& doc,
        OpenSim::Model& model,
        const MiBody& body_el,
        std::unordered_map<UID, OpenSim::Body*>& visited_bodies)
    {
        std::unique_ptr<OpenSim::Body> added_body = create_detatched_body(doc, body_el);
        auto weld_joint = std::make_unique<OpenSim::WeldJoint>();
        auto parent_frame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        auto child_frame = std::make_unique<OpenSim::PhysicalOffsetFrame>();

        // set names
        weld_joint->setName(std::string{body_el.get_label()} + "_to_ground");
        parent_frame->setName("ground_offset");
        child_frame->setName(std::string{body_el.get_label()} + "_offset");

        // make the parent have the same coordinate frame as the placed body
        parent_frame->setOffsetTransform(to<SimTK::Transform>(body_el.get_x_form()));

        // attach the parent directly to ground and the child directly to the body
        // and make them the two attachments of the joint
        parent_frame->setParentFrame(model.getGround());
        child_frame->setParentFrame(*added_body);
        weld_joint->connectSocket_parent_frame(*parent_frame);
        weld_joint->connectSocket_child_frame(*child_frame);

        // populate the "already visited bodies" cache
        visited_bodies[body_el.get_id()] = added_body.get();

        // add the components into the OpenSim::Model
        opyn::add_frame(*weld_joint, std::move(parent_frame));
        opyn::add_frame(*weld_joint, std::move(child_frame));
        opyn::add_body(model, std::move(added_body));
        opyn::add_joint(model, std::move(weld_joint));
    }

    void add_station_to_model(
        const MiDocument& doc,
        ModelCreationFlags flags,
        OpenSim::Model& model,
        const MiStation& station_el,
        std::unordered_map<UID, OpenSim::Body*>& visited_bodies)
    {

        const JointAttachmentCachedLookupResult res =
            lookup_phys_frame(doc, model, visited_bodies, station_el.get_parent_id());
        OSC_ASSERT_ALWAYS(
            res.physicalFrame != nullptr && "all physical frames should have been added by this "
                                            "point in the model-building process");

        const auto parent_xform =
            to<SimTK::Transform>(doc.get_by_id(station_el.get_parent_id()).get_x_form(doc));
        const auto station_xform = to<SimTK::Transform>(station_el.get_x_form());
        const SimTK::Vec3 location_in_parent = (parent_xform.invert() * station_xform).p();

        if (flags & ModelCreationFlags::ExportStationsAsMarkers) {
            // export as markers in the model's markerset (overridden behavior)
            opyn::add_marker(
                model, to_string(station_el.get_label()), *res.physicalFrame, location_in_parent);
        } else {
            // export as stations in the given frame (default behavior)
            auto station =
                std::make_unique<OpenSim::Station>(*res.physicalFrame, location_in_parent);
            station->setName(to_string(station_el.get_label()));

            opyn::add_component(*res.physicalFrame, std::move(station));
        }
    }

    // tries to find the first body connected to the given PhysicalFrame by assuming
    // that the frame is either already a body or is an offset to a body
    const OpenSim::PhysicalFrame* try_inclusive_recurse_to_body_or_ground(
        const OpenSim::Frame& f, std::unordered_set<const OpenSim::Frame*> visited_frames)
    {
        if (!visited_frames.emplace(&f).second) {
            return nullptr;
        }

        if (const auto* body = dynamic_cast<const OpenSim::Body*>(&f)) {
            return body;
        } else if (const auto* ground = dynamic_cast<const OpenSim::Ground*>(&f)) {
            return ground;
        } else if (const auto* pof = dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&f)) {
            return try_inclusive_recurse_to_body_or_ground(pof->getParentFrame(), visited_frames);
        } else if (const auto* station = dynamic_cast<const OpenSim::Station*>(&f)) {
            return try_inclusive_recurse_to_body_or_ground(
                station->getParentFrame(), visited_frames);
        } else {
            return nullptr;
        }
    }

    // tries to find the first body connected to the given PhysicalFrame by assuming
    // that the frame is either already a body or is an offset to a body
    const OpenSim::PhysicalFrame* try_inclusive_recurse_to_body_or_ground(const OpenSim::Frame& f)
    {
        return try_inclusive_recurse_to_body_or_ground(f, {});
    }

    MiDocument create_mesh_importer_document_from_model(std::unique_ptr<OpenSim::Model> ptr)
    {
        OpenSim::Model& m = *ptr;

        // init model+state
        opyn::initialize_model(m);
        const SimTK::State& st = opyn::initialize_state(m);

        // this is what this method populates
        MiDocument rv;

        // used to figure out how a body in the OpenSim::Model maps into the docuemnt
        std::unordered_map<const OpenSim::Body*, UID> body_lookup;

        // used to figure out how a joint in the OpenSim::Model maps into the document
        std::unordered_map<const OpenSim::Joint*, UID> joint_lookup;

        // import all the bodies from the model file
        for (const OpenSim::Body& b : m.getComponentList<OpenSim::Body>()) {
            const std::string name = b.getName();
            const auto xform = to<Transform>(b.getTransformInGround(st));

            auto& el = rv.emplace<MiBody>(UID{}, name, xform);
            el.set_mass(b.getMass());

            body_lookup.emplace(&b, el.get_id());
        }

        // then try and import all the joints (by looking at their connectivity)
        for (const OpenSim::Joint& j : m.getComponentList<OpenSim::Joint>()) {
            const OpenSim::PhysicalFrame& parent_frame = j.getParentFrame();
            const OpenSim::PhysicalFrame& child_frame = j.getChildFrame();

            const OpenSim::PhysicalFrame* const parent_body_or_ground =
                try_inclusive_recurse_to_body_or_ground(parent_frame);
            const OpenSim::PhysicalFrame* const child_body_or_ground =
                try_inclusive_recurse_to_body_or_ground(child_frame);

            if (!parent_body_or_ground || !child_body_or_ground) {
                // can't find what they're connected to
                continue;
            }

            UID parent = MiIDs::empty();
            if (dynamic_cast<const OpenSim::Ground*>(parent_body_or_ground)) {
                parent = MiIDs::ground();
            } else {
                if (const auto* body = lookup_or_nullptr(
                        body_lookup, dynamic_cast<const OpenSim::Body*>(parent_body_or_ground))) {

                    parent = *body;
                } else {
                    continue;  // joint is attached to a body that isn't ground or cached?
                }
            }

            UID child = MiIDs::empty();
            if (dynamic_cast<const OpenSim::Ground*>(child_body_or_ground)) {
                // ground can't be a child in a joint
                continue;
            } else {
                if (const auto* body = lookup_or_nullptr(
                        body_lookup, dynamic_cast<const OpenSim::Body*>(child_body_or_ground))) {

                    child = *body;
                } else {
                    continue;  // joint is attached to a body that isn't ground or cached?
                }
            }

            if (parent == MiIDs::empty() || child == MiIDs::empty()) {
                // something horrible happened above
                continue;
            }

            const auto xform = to<Transform>(parent_frame.getTransformInGround(st));

            auto& joint_el = rv.emplace<MiJoint>(
                UID{}, j.getConcreteClassName(), j.getName(), parent, child, xform);
            joint_lookup.emplace(&j, joint_el.get_id());
        }

        // then try to import all the meshes
        for (const OpenSim::Mesh& mesh : m.getComponentList<OpenSim::Mesh>()) {
            const std::optional<std::filesystem::path> maybe_mesh_path =
                opyn::find_geometry_file_abs_path(m, mesh);

            if (!maybe_mesh_path) {
                continue;
            }

            const std::filesystem::path& real_location = *maybe_mesh_path;

            osc::Mesh mesh_data;
            try {
                mesh_data = opyn::load_mesh_via_simbody(real_location.string());
            } catch (const std::exception& ex) {
                log_error("error loading mesh: {}", ex.what());
                continue;
            }

            const OpenSim::Frame& frame = mesh.getFrame();
            const OpenSim::PhysicalFrame* const frame_body_or_ground =
                try_inclusive_recurse_to_body_or_ground(frame);

            if (!frame_body_or_ground) {
                // can't find what it's connected to?
                continue;
            }

            UID attachment = MiIDs::empty();
            if (dynamic_cast<const OpenSim::Ground*>(frame_body_or_ground)) {
                attachment = MiIDs::ground();
            } else {
                if (const auto* body = lookup_or_nullptr(
                        body_lookup, dynamic_cast<const OpenSim::Body*>(frame_body_or_ground))) {

                    attachment = *body;
                } else {
                    continue;  // mesh is attached to something that isn't a ground or a body?
                }
            }

            if (attachment == MiIDs::empty()) {
                // couldn't figure out what to attach to
                continue;
            }

            auto& el = rv.emplace<MiMesh>(UID{}, attachment, mesh_data, real_location);
            auto new_transform = to<Transform>(frame.getTransformInGround(st));
            new_transform.scale = to<Vector3>(mesh.get_scale_factors());

            el.set_xform(new_transform);
            el.set_label(mesh.getName());
        }

        // then try to import all the stations
        for (const OpenSim::Station& station : m.getComponentList<OpenSim::Station>()) {
            // edge-case: it's a path point: ignore it because it will spam the converter
            if (dynamic_cast<const OpenSim::AbstractPathPoint*>(&station)) {
                continue;
            }

            if (opyn::owner_is<OpenSim::AbstractPathPoint>(station)) {
                continue;
            }

            const OpenSim::PhysicalFrame& frame = station.getParentFrame();
            const OpenSim::PhysicalFrame* const frame_body_or_ground =
                try_inclusive_recurse_to_body_or_ground(frame);

            UID attachment = MiIDs::empty();
            if (dynamic_cast<const OpenSim::Ground*>(frame_body_or_ground)) {
                attachment = MiIDs::ground();
            } else {
                if (const auto it =
                        body_lookup.find(dynamic_cast<const OpenSim::Body*>(frame_body_or_ground));
                    it != body_lookup.end()) {

                    attachment = it->second;
                } else {
                    // station is attached to something that isn't ground or a cached body
                    continue;
                }
            }

            if (attachment == MiIDs::empty()) {
                // can't figure out what to attach to
                continue;
            }

            const auto position = to<Vector3>(station.findLocationInFrame(st, m.getGround()));
            const std::string name = station.getName();

            rv.emplace<MiStation>(attachment, position, name);
        }

        return rv;
    }
}

MiDocument osc::create_model_from_osim_file(const std::filesystem::path& p)
{
    return create_mesh_importer_document_from_model(opyn::load_model(p));
}

std::unique_ptr<OpenSim::Model> osc::create_open_sim_model_from_mesh_importer_document(
    const MiDocument& doc, ModelCreationFlags flags, std::vector<std::string>& issues_out)
{
    if (get_issues(doc, issues_out)) {
        log_error("cannot create an osim model: issues detected");
        for (const std::string& issue : issues_out) {
            log_error("issue: {}", issue);
        }
        return nullptr;
    }

    // create the output model
    auto model = std::make_unique<OpenSim::Model>();
    model->updDisplayHints().upd_show_frames() = true;

    // add any meshes that are directly connected to ground (i.e. meshes that are not attached to a
    // body)
    for (const MiMesh& mesh_el : doc.iter<MiMesh>()) {
        if (mesh_el.get_parent_id() == MiIDs::ground()) {
            attach_mesh_el_to_frame(mesh_el, Transform{}, model->updGround());
        }
    }

    // keep track of any bodies/joints already visited (there might be cycles)
    std::unordered_map<UID, OpenSim::Body*> visited_bodies;
    std::unordered_set<UID> visited_joints;

    // directly connect any bodies that participate in no joints into the model with a default joint
    for (const MiBody& body_el : doc.iter<MiBody>()) {
        if (!is_a_child_attachment_in_any_joint(doc, body_el)) {
            attach_body_directly_to_ground(doc, *model, body_el, visited_bodies);
        }
    }

    // add bodies that do participate in joints into the model
    //
    // note: these bodies may use the non-participating bodies (above) as parents
    for (const MiJoint& joint_el : doc.iter<MiJoint>()) {
        if (joint_el.get_parent_id() == MiIDs::ground() ||
            visited_bodies.contains(joint_el.get_parent_id())) {

            attach_joint_recursive(doc, *model, joint_el, visited_bodies, visited_joints);
        }
    }

    // add stations into the model
    for (const MiStation& el : doc.iter<MiStation>()) {
        add_station_to_model(doc, flags, *model, el, visited_bodies);
    }

    // invalidate all properties, so that model.finalizeFromProperties() *must*
    // reload everything with no caching
    //
    // otherwise, parts of the model (cough cough, OpenSim::Geometry::finalizeFromProperties)
    // will fail to load data because it will internally set itself as up to date, even though
    // it failed to load a mesh file because a parent was missing. See #330
    for (OpenSim::Component& c : model->updComponentList()) {
        for (int i = 0; i < c.getNumProperties(); ++i) {
            c.updPropertyByIndex(i);
        }
    }

    // ensure returned model is initialized from latest document
    model->finalizeConnections();  // ensure all sockets are finalized to paths (#263)
    opyn::initialize_model(*model);
    opyn::initialize_state(*model);

    return model;
}

Vector3 osc::get_joint_axis_lengths(const MiJoint& joint)
{
    const auto& registry = opyn::get_component_registry<OpenSim::Joint>();

    JointDegreesOfFreedom dofs{};
    if (const auto idx = opyn::index_of(registry, joint.get_specific_type_name())) {
        dofs = get_degrees_of_freedom(registry[*idx].prototype());
    }

    Vector3 rv{};
    for (int i = 0; i < 3; ++i) {
        rv[i] = dofs.orientation[static_cast<size_t>(i)] == -1 ? 0.6f : 1.0f;
    }
    return rv;
}
