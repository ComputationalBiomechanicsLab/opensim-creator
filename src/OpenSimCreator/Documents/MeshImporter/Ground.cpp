#include "Ground.h"

#include <OpenSimCreator/Documents/MeshImporter/MIClass.h>
#include <OpenSimCreator/Documents/MeshImporter/MIIDs.h>
#include <OpenSimCreator/Documents/MeshImporter/MIStrings.h>

#include <IconsFontAwesome5.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

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
        ICON_FA_DOT_CIRCLE,
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
