#include "mi_body.h"

#include <libopensimcreator/documents/mesh_importer/mi_class.h>
#include <libopensimcreator/documents/mesh_importer/mi_strings.h>
#include <libopensimcreator/platform/msmicons.h>

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/maths/transform.h>
#include <liboscar/utilities/uid.h>

#include <ostream>
#include <string>
#include <string_view>

using osc::MiClass;

osc::MiBody::MiBody(UID id, const std::string& name, const Transform& xform) :

    id_{id},
    name_{opyn::sanitize_to_open_sim_component_name(name)},
    xform_{xform}
{}

void osc::MiBody::impl_set_label(std::string_view sv)
{
    name_ = opyn::sanitize_to_open_sim_component_name(sv);
}

MiClass osc::MiBody::create_class()
{
    return {
        MiStrings::c_body_label,
        MiStrings::c_body_label_pluralized,
        MiStrings::c_body_label_optionally_pluralized,
        MSMICONS_CIRCLE,
        MiStrings::c_body_description,
    };
}

std::ostream& osc::MiBody::impl_write_to_stream(std::ostream& o) const
{
    return o << "MiBody(ID = " << id_         //
             << ", Name = " << name_          //
             << ", m_Transform = " << xform_  //
             << ", Mass = " << mass_          //
             << ')';
}
