#pragma once

#include "oscar/Graphics/BlitFlags.hpp"
#include "oscar/Graphics/CubemapFace.hpp"
#include "oscar/Graphics/MaterialPropertyBlock.hpp"

#include <glm/mat4x4.hpp>

#include <optional>

namespace osc { class Camera; }
namespace osc { class Image; }
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
        std::optional<MaterialPropertyBlock> const& = std::nullopt
    );

    void DrawMesh(
        Mesh const&,
        glm::mat4 const&,
        Material const&,
        Camera&,
        std::optional<MaterialPropertyBlock> const& = std::nullopt
    );

    // blit: use a shader to copy a GPU texture to a GPU render texture or
    // the screen

    void Blit(
        Texture2D const&,
        RenderTexture& dest
    );

    void BlitToScreen(
        RenderTexture const&,
        Rect const&,
        BlitFlags = BlitFlags::None
    );

    void BlitToScreen(
        RenderTexture const&,
        Rect const&,
        Material const&,  // assigns the source RenderTexture to uniform "uTexture" (can be sampler2D or samplerCube depending on source)
        BlitFlags = BlitFlags::None
    );

    void BlitToScreen(
        Texture2D const&,
        Rect const&
    );

    // copy: copy one GPU texture to another GPU texture

    void CopyTexture(
        RenderTexture const&,
        Texture2D&
    );

    void CopyTexture(
        RenderTexture const&,
        Texture2D&,
        CubemapFace
    );
}
