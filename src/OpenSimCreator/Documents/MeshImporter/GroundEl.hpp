#pragma once

#include <OpenSimCreator/Documents/MeshImporter/SceneElCRTP.hpp>
#include <OpenSimCreator/Documents/MeshImporter/SceneElFlags.hpp>

#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <iosfwd>

namespace osc
{
    // "Ground" of the scene (i.e. origin)
    class GroundEl final : public SceneElCRTP<GroundEl> {
    private:
        friend class SceneElCRTP<GroundEl>;
        static SceneElClass CreateClass();

        SceneElFlags implGetFlags() const final
        {
            return SceneElFlags::None;
        }

        UID implGetID() const final;

        std::ostream& implWriteToStream(std::ostream&) const final;

        CStringView implGetLabel() const final;

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
