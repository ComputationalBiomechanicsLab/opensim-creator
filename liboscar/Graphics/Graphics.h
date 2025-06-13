#pragma once

#include <liboscar/Graphics/BlitFlags.h>
#include <liboscar/Graphics/CubemapFace.h>
#include <liboscar/Graphics/MaterialPropertyBlock.h>
#include <liboscar/Maths/Mat4.h>

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
    // Queues the given `Mesh` + `Transform` + `Material` + extras against
    // the `Camera`.
    //
    // Once everything is queued against the `Camera`, the caller should call
    // `Camera::render()` or `Camera::render_to()` to flush the queue.
    void draw(
        const Mesh&,
        const Transform&,
        const Material&,
        Camera&,
        const std::optional<MaterialPropertyBlock>& = std::nullopt,
        std::optional<size_t> maybe_submesh_index = std::nullopt
    );

    // Queues the given `Mesh` + `Mat4` + `Material` + extras against
    // the `Camera`.
    //
    // Once everything is queued against the `Camera`, the caller should call
    // `Camera::render()` or `Camera::render_to()` to flush the queue.
    void draw(
        const Mesh&,
        const Mat4&,
        const Material&,
        Camera&,
        const std::optional<MaterialPropertyBlock>& = std::nullopt,
        std::optional<size_t> maybe_submesh_index = std::nullopt
    );

    // Blits (copies) the `Texture` to the `RenderTexture`.
    void blit(
        const Texture2D&,
        RenderTexture&
    );

    // Blits the texture into a rectangular subspace of the main window.
    //
    // The rectangle should be defined in screen space, which:
    //
    // - Is measured in device-independent pixels
    // - Starts in the bottom-left corner
    // - Ends in the top-right corner
    void blit_to_main_window(
        const RenderTexture&,
        const Rect&,
        BlitFlags = {}
    );

    // Renders the texture to a quad via the given `Material` into a rectangular
    // subspace of the main application window.
    //
    // The rectangle should be defined in screen space, which:
    //
    // - Is measured in device-independent pixels
    // - Starts in the bottom-left corner
    // - Ends in the top-right corner
    //
    // The `Material` should:
    //
    // - Have a `sampler2D` or `samplerCube` property called "uTexture". The texture
    //   will be assigned to this property. The texture's `dimensionality()` dictates
    //   whether to use a `sampler2D` or `samplerCube` in the shader.
    void blit_to_main_window(
        const RenderTexture&,
        const Rect&,
        const Material&,
        BlitFlags = {}
    );

    // Blits the texture into a rectangular subspace of the main window.
    //
    // The rectangle should be defined in screen space, which:
    //
    // - Is measured in device-independent pixels
    // - Starts in the bottom-left corner
    // - Ends in the top-right corner
    void blit_to_main_window(
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
