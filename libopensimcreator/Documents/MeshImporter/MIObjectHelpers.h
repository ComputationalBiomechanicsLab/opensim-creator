#pragma once

#include <libopensimcreator/Documents/MeshImporter/MIClass.h>
#include <libopensimcreator/Documents/MeshImporter/MIVariant.h>

#include <liboscar/maths/vector3.h>

#include <array>
#include <variant>

namespace osc::mi { class Mesh; }
namespace osc::mi { class MIObject; }

namespace osc::mi
{
    // returns true if a mesh can be attached to the given object
    bool CanAttachMeshTo(const MIObject&);

    // returns `true` if a `StationEl` can be attached to the object
    bool CanAttachStationTo(const MIObject&);

    const std::array<MIClass, std::variant_size_v<SceneElVariant>>& GetSceneElClasses();

    Vector3 AverageCenter(const Mesh&);
    Vector3 mass_center_of(const Mesh&);
}
