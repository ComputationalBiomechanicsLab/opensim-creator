#pragma once

#include <OpenSimCreator/Documents/MeshImporter/MIClass.hpp>
#include <OpenSimCreator/Documents/MeshImporter/MIVariant.hpp>

#include <oscar/Maths/Vec3.hpp>

#include <array>
#include <variant>

namespace osc::mi { class Mesh; }
namespace osc::mi { class MIObject; }

namespace osc::mi
{
    // returns true if a mesh can be attached to the given object
    bool CanAttachMeshTo(MIObject const&);

    // returns `true` if a `StationEl` can be attached to the object
    bool CanAttachStationTo(MIObject const&);

    std::array<MIClass, std::variant_size_v<SceneElVariant>> const& GetSceneElClasses();

    Vec3 AverageCenter(Mesh const&);
    Vec3 MassCenter(Mesh const&);
}
