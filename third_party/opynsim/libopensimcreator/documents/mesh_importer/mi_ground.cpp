#include "mi_ground.h"

#include <libopensimcreator/documents/mesh_importer/mi_class.h>
#include <libopensimcreator/documents/mesh_importer/mi_ids.h>
#include <libopensimcreator/documents/mesh_importer/mi_strings.h>
#include <libopensimcreator/platform/msmicons.h>

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/uid.h>

#include <iostream>

using osc::CStringView;
using osc::MiClass;
using osc::UID;

MiClass osc::MiGround::create_class()
{
    return {
        MiStrings::c_ground_label,
        MiStrings::c_ground_label_pluralized,
        MiStrings::c_ground_label_optionally_pluralized,
        MSMICONS_DOT_CIRCLE,
        MiStrings::c_ground_description,
    };
}

UID osc::MiGround::impl_get_id() const
{
    return MiIDs::ground();
}

std::ostream& osc::MiGround::impl_write_to_stream(std::ostream& o) const
{
    return o << MiStrings::c_ground_label << "()";
}

CStringView osc::MiGround::impl_get_label() const
{
    return MiStrings::c_ground_label;
}
