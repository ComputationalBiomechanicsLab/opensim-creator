#pragma once

#include <OpenSimCreator/ModelGraph/SceneElCRTP.hpp>
#include <OpenSimCreator/ModelGraph/SceneElFlags.hpp>
#include <OpenSimCreator/ModelGraph/ModelGraphIDs.hpp>
#include <OpenSimCreator/ModelGraph/ModelGraphStrings.hpp>

#include <IconsFontAwesome5.h>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <iostream>

namespace osc
{
    // "Ground" of the scene (i.e. origin)
    class GroundEl final : public SceneElCRTP<GroundEl> {
    private:
        friend class SceneElCRTP<GroundEl>;
        static SceneElClass CreateClass()
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

        SceneElFlags implGetFlags() const final
        {
            return SceneElFlags::None;
        }

        UID implGetID() const final
        {
            return ModelGraphIDs::Ground();
        }

        std::ostream& implWriteToStream(std::ostream& o) const final
        {
            return o << ModelGraphStrings::c_GroundLabel << "()";
        }

        CStringView implGetLabel() const final
        {
            return ModelGraphStrings::c_GroundLabel;
        }

        Transform implGetXform(ISceneElLookup const&) const final
        {
            return Identity<Transform>();
        }

        AABB implCalcBounds(ISceneElLookup const&) const final
        {
            return AABB{};
        }
    };
}
