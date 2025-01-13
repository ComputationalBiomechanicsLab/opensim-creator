#include "Ground.h"

#include <libOpenSimCreator/Documents/MeshImporter/MIClass.h>
#include <libOpenSimCreator/Documents/MeshImporter/MIIDs.h>
#include <libOpenSimCreator/Documents/MeshImporter/MIStrings.h>

#include <liboscar/Platform/IconCodepoints.h>
#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/UID.h>

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
        OSC_ICON_DOT_CIRCLE,
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
