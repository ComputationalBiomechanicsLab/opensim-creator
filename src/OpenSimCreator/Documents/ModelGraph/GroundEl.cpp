#include "GroundEl.hpp"

#include <OpenSimCreator/Documents/ModelGraph/ModelGraphStrings.hpp>
#include <OpenSimCreator/Documents/ModelGraph/ModelGraphIDs.hpp>
#include <OpenSimCreator/Documents/ModelGraph/SceneElClass.hpp>

#include <IconsFontAwesome5.h>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <iostream>

osc::SceneElClass osc::GroundEl::CreateClass()
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

osc::UID osc::GroundEl::implGetID() const
{
    return ModelGraphIDs::Ground();
}

std::ostream& osc::GroundEl::implWriteToStream(std::ostream& o) const
{
    return o << ModelGraphStrings::c_GroundLabel << "()";
}

osc::CStringView osc::GroundEl::implGetLabel() const
{
    return ModelGraphStrings::c_GroundLabel;
}
