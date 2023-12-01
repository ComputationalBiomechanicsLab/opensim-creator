#include "Ground.hpp"

#include <OpenSimCreator/Documents/MeshImporter/MIClass.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIIDs.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIStrings.hpp>

#include <IconsFontAwesome5.h>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <iostream>

using osc::mi::MIClass;
using osc::CStringView;
using osc::UID;

MIClass osc::mi::Ground::CreateClass()
{
    return
    {
        ModelGraphStrings::c_GroundLabel,
        ModelGraphStrings::c_GroundLabelPluralized,
        ModelGraphStrings::c_GroundLabelOptionallyPluralized,
        ICON_FA_DOT_CIRCLE,
        ModelGraphStrings::c_GroundDescription,
    };
}

UID osc::mi::Ground::implGetID() const
{
    return MIIDs::Ground();
}

std::ostream& osc::mi::Ground::implWriteToStream(std::ostream& o) const
{
    return o << ModelGraphStrings::c_GroundLabel << "()";
}

CStringView osc::mi::Ground::implGetLabel() const
{
    return ModelGraphStrings::c_GroundLabel;
}
