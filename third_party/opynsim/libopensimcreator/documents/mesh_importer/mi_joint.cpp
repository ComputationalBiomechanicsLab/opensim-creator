#include "mi_joint.h"

#include <libopensimcreator/documents/mesh_importer/mi_crossref_descriptor.h>
#include <libopensimcreator/documents/mesh_importer/mi_crossref_direction.h>
#include <libopensimcreator/documents/mesh_importer/mi_strings.h>
#include <libopensimcreator/platform/msmicons.h>

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/maths/transform.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/uid.h>

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

using osc::MiClass;
using osc::MiCrossrefDescriptor;

osc::MiJoint::MiJoint(
    UID id,
    std::string joint_type_name,
    const std::string& user_assigned_name,  // can be empty
    UID parent,
    UID child,
    const Transform& xform) :

    id_{id},
    joint_type_name_{std::move(joint_type_name)},
    user_assigned_name_{opyn::sanitize_to_open_sim_component_name(user_assigned_name)},
    parent_{parent},
    child_{child},
    xform_{xform}
{}

MiClass osc::MiJoint::create_class()
{
    return {
        MiStrings::c_joint_label,
        MiStrings::c_joint_label_pluralized,
        MiStrings::c_joint_label_optionally_pluralized,
        MSMICONS_LINK,
        MiStrings::c_joint_description,
    };
}

std::vector<MiCrossrefDescriptor> osc::MiJoint::impl_get_cross_references() const
{
    return {
        {parent_, MiStrings::c_joint_parent_crossref_name, MiCrossrefDirection::ToParent},
        {child_, MiStrings::c_joint_child_crossref_name, MiCrossrefDirection::ToChild},
    };
}

std::ostream& osc::MiJoint::impl_write_to_stream(std::ostream& o) const
{
    return o << "MiJoint(ID = " << id_                          //
             << ", JointTypeName = " << joint_type_name_        //
             << ", UserAssignedName = " << user_assigned_name_  //
             << ", Parent = " << parent_                        //
             << ", Child = " << child_                          //
             << ", Transform = " << xform_                      //
             << ')';
}

void osc::MiJoint::impl_set_label(std::string_view sv)
{
    user_assigned_name_ = opyn::sanitize_to_open_sim_component_name(sv);
}
