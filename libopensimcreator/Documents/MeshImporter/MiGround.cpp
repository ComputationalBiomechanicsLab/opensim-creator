#include "MiGround.h"

#include <libopensimcreator/Documents/MeshImporter/MiClass.h>
#include <libopensimcreator/Documents/MeshImporter/MiIDs.h>
#include <libopensimcreator/Documents/MeshImporter/MiStrings.h>
#include <libopensimcreator/Platform/msmicons.h>

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/uid.h>

#include <iostream>

using osc::MiClass;
using osc::CStringView;
using osc::UID;

MiClass osc::MiGround::CreateClass()
{
    return
    {
        MiStrings::c_GroundLabel,
        MiStrings::c_GroundLabelPluralized,
        MiStrings::c_GroundLabelOptionallyPluralized,
        MSMICONS_DOT_CIRCLE,
        MiStrings::c_GroundDescription,
    };
}

UID osc::MiGround::implGetID() const
{
    return MiIDs::Ground();
}

std::ostream& osc::MiGround::implWriteToStream(std::ostream& o) const
{
    return o << MiStrings::c_GroundLabel << "()";
}

CStringView osc::MiGround::implGetLabel() const
{
    return MiStrings::c_GroundLabel;
}
