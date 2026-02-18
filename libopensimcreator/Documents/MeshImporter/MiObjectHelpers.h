#pragma once

#include <libopensimcreator/Documents/MeshImporter/MiClass.h>
#include <libopensimcreator/Documents/MeshImporter/MiVariantReference.h>

#include <liboscar/maths/vector3.h>

#include <array>
#include <variant>

namespace osc { class MiMesh; }
namespace osc { class MiObject; }

namespace osc
{
    // returns true if a mesh can be attached to the given object
    bool CanAttachMeshTo(const MiObject&);

    // returns `true` if a `MiStation` can be attached to the object
    bool CanAttachStationTo(const MiObject&);

    const std::array<MiClass, std::variant_size_v<MiVariantReference>>& GetSceneElClasses();

    Vector3 AverageCenter(const MiMesh&);
    Vector3 mass_center_of(const MiMesh&);
}
