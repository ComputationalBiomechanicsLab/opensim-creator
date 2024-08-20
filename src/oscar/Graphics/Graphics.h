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
namespace osc::graphics
{
    // draw: enqueue drawable elements onto the camera ready for rendering

    void draw(
        const Mesh&,
        const Transform&,
        const Material&,
        Camera&,
        const std::optional<MaterialPropertyBlock>& = std::nullopt,
        std::optional<size_t> maybe_submesh_index = std::nullopt
    );

    void draw(
        const Mesh&,
        const Mat4&,
        const Material&,
        Camera&,
        const std::optional<MaterialPropertyBlock>& = std::nullopt,
        std::optional<size_t> maybe_submesh_index = std::nullopt
    );

    // blit: use a shader to copy a GPU texture to a GPU render texture or
    // the screen

    void blit(
        const Texture2D&,
        RenderTexture&
    );

    void blit_to_screen(
        const RenderTexture&,
        const Rect&,
        BlitFlags = {}
    );

    // assigns the source RenderTexture to the texture uniform "uTexture"
    //
    // (the glsl uniform may be `sampler2D` or `samplerCube`, depending on the source `RenderTexture`)
    void blit_to_screen(
        const RenderTexture&,
        const Rect&,
        const Material&,
        BlitFlags = {}
    );

    void blit_to_screen(
        const Texture2D&,
        const Rect&
    );

    // copy: copy a GPU texture to a (potentially, CPU-accessible) texture

    void copy_texture(
        const RenderTexture&,
        Texture2D&
    );

    void copy_texture(
        const RenderTexture&,
        Texture2D&,
        CubemapFace
    );

    void copy_texture(
        const RenderTexture&,
        Cubemap&,
        size_t mip
    );
}
