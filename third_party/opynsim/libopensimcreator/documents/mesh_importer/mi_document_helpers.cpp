#include "mi_document_helpers.h"

#include <libopensimcreator/documents/mesh_importer/mi_body.h>
#include <libopensimcreator/documents/mesh_importer/mi_document.h>
#include <libopensimcreator/documents/mesh_importer/mi_joint.h>
#include <libopensimcreator/documents/mesh_importer/mi_ids.h>
#include <libopensimcreator/documents/mesh_importer/mi_mesh.h>
#include <libopensimcreator/documents/mesh_importer/mi_object.h>
#include <libopensimcreator/documents/mesh_importer/mi_station.h>

#include <liboscar/graphics/scene/scene_decoration_flags.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/transform.h>
#include <liboscar/maths/vector.h>
#include <liboscar/shims/cpp23/ranges.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/std_variant_helpers.h>
#include <liboscar/utilities/uid.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <variant>

using namespace osc;

bool osc::is_a_child_attachment_in_any_joint(const MiDocument& doc, const MiObject& obj)
{
    return cpp23::contains(doc.iter<MiJoint>(), obj.get_id(), &MiJoint::get_child_id);
}

bool osc::is_garbage_joint(const MiDocument& doc, const MiJoint& joint)
{
    if (joint.get_child_id() == MiIDs::ground()) {
        return true;  // ground cannot be a child in a joint
    }

    if (joint.get_parent_id() == joint.get_child_id()) {
        return true;  // is directly attached to itself
    }

    if (joint.get_parent_id() != MiIDs::ground() && !doc.contains<MiBody>(joint.get_parent_id())) {
        return true;  // has a parent ID that's invalid for this document
    }

    if (!doc.contains<MiBody>(joint.get_child_id())) {
        return true;  // has a child ID that's invalid for this document
    }

    return false;
}

bool osc::is_joint_attached_to_ground(
    const MiDocument& doc, const MiJoint& joint, std::unordered_set<UID>& previous_visits)
{
    OSC_ASSERT_ALWAYS(!is_garbage_joint(doc, joint));

    if (joint.get_parent_id() == MiIDs::ground()) {
        return true;  // it's directly attached to ground
    }

    const auto* const parent = doc.try_get_by_id<MiBody>(joint.get_parent_id());
    if (!parent) {
        return false;  // joint's parent is garbage
    }

    // else: recurse to parent
    return is_body_attached_to_ground(doc, *parent, previous_visits);
}

bool osc::is_body_attached_to_ground(
    const MiDocument& doc, const MiBody& body, std::unordered_set<UID>& previously_visited_joints)
{
    bool child_in_at_least_one_joint = false;

    for (const MiJoint& joint : doc.iter<MiJoint>()) {
        OSC_ASSERT(!is_garbage_joint(doc, joint));

        if (joint.get_child_id() == body.get_id()) {
            child_in_at_least_one_joint = true;

            const bool already_visited = !previously_visited_joints.emplace(joint.get_id()).second;
            if (already_visited) {
                continue;  // skip this joint: was previously visited
            }

            if (is_joint_attached_to_ground(doc, joint, previously_visited_joints)) {
                return true;  // recurse
            }
        }
    }

    return !child_in_at_least_one_joint;
}

bool osc::get_issues(const MiDocument& doc, std::vector<std::string>& issues_out)
{
    issues_out.clear();

    for (const MiJoint& joint : doc.iter<MiJoint>()) {
        if (is_garbage_joint(doc, joint)) {
            std::stringstream ss;
            ss << joint.get_label() << ": joint is garbage (this is an implementation error)";
            throw std::runtime_error{std::move(ss).str()};
        }
    }

    for (const MiBody& body : doc.iter<MiBody>()) {
        std::unordered_set<UID> previously_visited_joints;
        if (!is_body_attached_to_ground(doc, body, previously_visited_joints)) {
            std::stringstream ss;
            ss << body.get_label()
               << ": body is not attached to ground: it is connected by a joint that, itself, does "
                  "not connect to ground";
            issues_out.push_back(std::move(ss).str());
        }
    }

    return !issues_out.empty();
}

std::string osc::get_context_menu_sub_header_text(const MiDocument& doc, const MiObject& obj)
{
    std::stringstream ss;
    // clang-format off
    std::visit(Overload
    {
        [&ss](const MiGround&)
        {
            ss << "(scene origin)";
        },
        [&ss, &doc](const MiMesh& m)
        {
            ss << '(' << MiMesh::get_class().get_name() << ", " << m.get_path().filename().string() << ", attached to " << doc.get_label_by_id(m.get_parent_id()) << ')';
        },
        [&ss](const MiBody&)
        {
            ss << '(' << MiBody::get_class().get_name() << ')';
        },
        [&ss, &doc](const MiJoint& j)
        {
            ss << '(' << j.get_specific_type_name() << ", " << doc.get_label_by_id(j.get_child_id()) << " --> " << doc.get_label_by_id(j.get_parent_id()) << ')';
        },
        [&ss, &doc](const MiStation& s)
        {
            ss << '(' << MiStation::get_class().get_name() << ", attached to " << doc.get_label_by_id(s.get_parent_id()) << ')';
        },
    }, obj.to_variant());
    // clang-format on
    return std::move(ss).str();
}

bool osc::is_in_selection_group_of(const MiDocument& doc, UID parent, UID id)
{
    if (id == MiIDs::empty() || parent == MiIDs::empty()) {
        return false;
    }

    if (id == parent) {
        return true;
    }

    const MiBody* body = nullptr;
    if (const auto* be = doc.try_get_by_id<MiBody>(parent)) {
        body = be;
    } else if (const auto* me = doc.try_get_by_id<MiMesh>(parent)) {
        body = doc.try_get_by_id<MiBody>(me->get_parent_id());
    }

    if (!body) {
        return false;  // parent isn't attached to any body (or isn't a body)
    }

    if (const auto* be = doc.try_get_by_id<MiBody>(id)) {
        return be->get_id() == body->get_id();
    } else if (const auto* me = doc.try_get_by_id<MiMesh>(id)) {
        return me->get_parent_id() == body->get_id();
    } else {
        return false;
    }
}

void osc::select_anything_grouped_with(MiDocument& doc, UID id)
{
    for_each_id_in_selection_group(doc, id, [&doc](UID other) { doc.select(other); });
}

UID osc::get_station_attachment_parent(const MiDocument& doc, const MiObject& obj)
{
    // clang-format off
    return std::visit(Overload
    {
        [](const MiGround&) { return MiIDs::ground(); },
        [&doc](const MiMesh& mesh_el) { return doc.contains<MiBody>(mesh_el.get_parent_id()) ? mesh_el.get_parent_id() : MiIDs::ground(); },
        [](const MiBody& body_el) { return body_el.get_id(); },
        [](const MiJoint&) { return MiIDs::ground(); },
        [](const MiStation&) { return MiIDs::ground(); },
    }, obj.to_variant());
    // clang-format on
}

void osc::point_axis_towards(MiDocument& doc, UID id, int axis, UID other)
{
    const Vector3 choice_pos = doc.get_pos_by_id(other);
    const Transform source_xform = {.translation = doc.get_pos_by_id(id)};

    doc.upd_by_id(id).set_xform(doc, point_axis_towards(source_xform, axis, choice_pos));
}

SceneDecorationFlags osc::compute_flags(
    const MiDocument& doc, UID id, std::optional<UID> maybe_hover_id)
{
    if (id == MiIDs::empty()) {
        return SceneDecorationFlag::Default;
    }

    const UID hover_id = maybe_hover_id ? *maybe_hover_id : MiIDs::empty();
    SceneDecorationFlags rv = SceneDecorationFlag::Default;
    if (doc.is_selected(id)) {
        rv |= SceneDecorationFlag::RimHighlight0;
    }
    if (id == hover_id or is_in_selection_group_of(doc, hover_id, id)) {
        rv |= SceneDecorationFlag::RimHighlight1;
    }
    return rv;
}
