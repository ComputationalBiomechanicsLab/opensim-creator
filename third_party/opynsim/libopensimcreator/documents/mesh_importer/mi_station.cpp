#include "mi_station.h"

#include <libopensimcreator/documents/mesh_importer/mi_crossref_descriptor.h>
#include <libopensimcreator/documents/mesh_importer/mi_crossref_direction.h>
#include <libopensimcreator/documents/mesh_importer/mi_strings.h>
#include <libopensimcreator/platform/msmicons.h>

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/maths/vector.h>
#include <liboscar/maths/vector_functions.h>
#include <liboscar/utilities/uid.h>

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using osc::MiClass;
using osc::MiCrossrefDescriptor;

osc::MiStation::MiStation(
    UID id, UID attachment, const Vector3& position, const std::string& name) :

    id_{id},
    attachment_{attachment},
    position_{position},
    name_{opyn::sanitize_to_open_sim_component_name(name)}
{}

osc::MiStation::MiStation(UID attachment, const Vector3& position, const std::string& name) :

    attachment_{attachment},
    position_{position},
    name_{opyn::sanitize_to_open_sim_component_name(name)}
{}

MiClass osc::MiStation::create_class()
{
    return {
        MiStrings::c_station_label,
        MiStrings::c_station_label_pluralized,
        MiStrings::c_station_label_optionally_pluralized,
        MSMICONS_MAP_PIN,
        MiStrings::c_station_description,
    };
}

std::vector<MiCrossrefDescriptor> osc::MiStation::impl_get_cross_references() const
{
    return {
        {attachment_, MiStrings::c_station_parent_crossref_name, MiCrossrefDirection::ToParent},
    };
}

std::ostream& osc::MiStation::impl_write_to_stream(std::ostream& o) const
{
    return o << "MiStation("                      //
             << "ID = " << id_                    //
             << ", Attachment = " << attachment_  //
             << ", Position = " << position_      //
             << ", Name = " << name_              //
             << ')';
}

void osc::MiStation::impl_set_label(std::string_view sv)
{
    name_ = opyn::sanitize_to_open_sim_component_name(sv);
}
