#include "src/Graphics/MaterialPropertyBlock.hpp"

#include <glm/mat4x4.hpp>

#include <optional>

namespace osc { class Camera; }
namespace osc { class Mesh; }
namespace osc { class Material; }
namespace osc { struct Rect; }
namespace osc { class RenderTexture; }
namespace osc { struct Transform; }

// rendering functions
//
// these perform the necessary backend steps to get something useful done
namespace osc::Graphics
{
    void DrawMesh(
        Mesh const&,
        Transform const&,
        Material const&,
        Camera&,
        std::optional<MaterialPropertyBlock> = std::nullopt);

    void DrawMesh(
        Mesh const&,
        glm::mat4 const&,
        Material const&,
        Camera&,
        std::optional<MaterialPropertyBlock> = std::nullopt);

    enum class BlitFlags {
        None,
        AlphaBlend,
    };

    void BlitToScreen(
        RenderTexture const&,
        Rect const&,
        BlitFlags = BlitFlags::None
    );

    // assigns the source RenderTexture to uniform "uTexture"
    void BlitToScreen(
        RenderTexture const&,
        Rect const&,
        Material const&,
        BlitFlags = BlitFlags::None
    );
}
