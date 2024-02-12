#pragma once

#include <oscar/Graphics/BlitFlags.h>
#include <oscar/Graphics/CubemapFace.h>
#include <oscar/Graphics/MaterialPropertyBlock.h>
#include <oscar/Maths/Mat4.h>

#include <cstddef>
#include <optional>

namespace osc { class Camera; }
namespace osc { class Cubemap; }
namespace osc { class Mesh; }
namespace osc { class Material; }
namespace osc { struct Rect; }
namespace osc { class RenderTexture; }
namespace osc { class Texture2D; }
namespace osc { struct Transform; }

// rendering functions
//
// these perform the necessary backend steps to get something useful done
namespace osc::Graphics
{
    // draw: enqueue drawable elements onto the camera ready for rendering

    void DrawMesh(
        Mesh const&,
        Transform const&,
        Material const&,
        Camera&,
        std::optional<MaterialPropertyBlock> const& = std::nullopt,
        std::optional<size_t> maybeSubMeshIndex = std::nullopt
    );

    void DrawMesh(
        Mesh const&,
        Mat4 const&,
        Material const&,
        Camera&,
        std::optional<MaterialPropertyBlock> const& = std::nullopt,
        std::optional<size_t> maybeSubMeshIndex = std::nullopt
    );

    // blit: use a shader to copy a GPU texture to a GPU render texture or
    // the screen

    void Blit(
        Texture2D const&,
        RenderTexture&
    );

    void BlitToScreen(
        RenderTexture const&,
        Rect const&,
        BlitFlags = BlitFlags::None
    );

    // assigns the source RenderTexture to the texture uniform "uTexture"
    //
    // (can be sampler2D or samplerCube, depending on the source RenderTexture)
    void BlitToScreen(
        RenderTexture const&,
        Rect const&,
        Material const&,
        BlitFlags = BlitFlags::None
    );

    void BlitToScreen(
        Texture2D const&,
        Rect const&
    );

    // copy: copy a GPU texture to a (potentially, CPU-accessible) texture

    void CopyTexture(
        RenderTexture const&,
        Texture2D&
    );

    void CopyTexture(
        RenderTexture const&,
        Texture2D&,
        CubemapFace
    );

    void CopyTexture(
        RenderTexture const&,
        Cubemap&,
        size_t mip
    );
}
