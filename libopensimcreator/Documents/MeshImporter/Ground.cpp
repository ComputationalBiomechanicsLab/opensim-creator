#include "Ground.h"

#include <libopensimcreator/Documents/MeshImporter/MIClass.h>
#include <libopensimcreator/Documents/MeshImporter/MIIDs.h>
#include <libopensimcreator/Documents/MeshImporter/MIStrings.h>
#include <libopensimcreator/Platform/msmicons.h>

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/uid.h>

#include <iostream>

using osc::mi::MIClass;
using osc::CStringView;
using osc::UID;

MIClass osc::mi::Ground::CreateClass()
{
    return
    {
        MIStrings::c_GroundLabel,
        MIStrings::c_GroundLabelPluralized,
        MIStrings::c_GroundLabelOptionallyPluralized,
        MSMICONS_DOT_CIRCLE,
        MIStrings::c_GroundDescription,
    };
}

UID osc::mi::Ground::implGetID() const
{
    return MIIDs::Ground();
}

std::ostream& osc::mi::Ground::implWriteToStream(std::ostream& o) const
{
    return o << MIStrings::c_GroundLabel << "()";
}

CStringView osc::mi::Ground::implGetLabel() const
{
    return MIStrings::c_GroundLabel;
}
