#pragma once

#include <liboscar/graphics/BlitFlags.h>
#include <liboscar/graphics/CubemapFace.h>
#include <liboscar/graphics/MaterialPropertyBlock.h>
#include <liboscar/maths/Matrix4x4.h>
#include <liboscar/maths/Rect.h>

#include <cstddef>
#include <optional>

namespace osc { class Camera; }
namespace osc { class Cubemap; }
namespace osc { class Material; }
namespace osc { class Mesh; }
namespace osc { class Rect; }
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
    void draw(const Mesh&, const Transform&, const Material&, Camera&);
    void draw(const Mesh&, const Transform&, const Material&, Camera&, const MaterialPropertyBlock&);
    void draw(const Mesh&, const Transform&, const Material&, Camera&, size_t submesh_index);
    void draw(const Mesh&, const Transform&, const Material&, Camera&, const MaterialPropertyBlock&, size_t submesh_index);

    // Queues the given `Mesh` + `Matrix4x4` + `Material` + extras against
    // the `Camera`.
    //
    // Once everything is queued against the `Camera`, the caller should call
    // `Camera::render()` or `Camera::render_to()` to flush the queue.
    void draw(const Mesh&, const Matrix4x4&, const Material&, Camera&);
    void draw(const Mesh&, const Matrix4x4&, const Material&, Camera&, const MaterialPropertyBlock&);
    void draw(const Mesh&, const Matrix4x4&, const Material&, Camera&, size_t submesh_index);
    void draw(const Mesh&, const Matrix4x4&, const Material&, Camera&, const MaterialPropertyBlock&, size_t submesh_index);

    // Blits the `Texture` to the `RenderTexture`.
    void blit(
        const Texture2D&,
        RenderTexture&
    );

    // Blits `render_texture` into a rectangular region of the main window.
    //
    // If provided, `destination_screen_rect` should be defined in screen space
    // and device-independent pixels. Screen space starts in the bottom-left
    // corner and ends in the top-right corner. If it is not provided, the
    // destination region will be the entire contents of the main window.
    void blit_to_main_window(
        const RenderTexture& render_texture,
        std::optional<Rect> destination_screen_rect = std::nullopt,
        BlitFlags = {}
    );

    // Renders `render_texture` as a quad using `material` into a rectangular region
    // of the main window.
    //
    // `material` should have a `sampler2D` or `samplerCube` property called
    // "uTexture". `texture` will be assigned to this property. `texture`'s
    // `dimensionality()` dictates whether a `sampler2D` or `samplerCube` is
    // required in the shader.
    //
    // If provided, `destination_screen_rect` should be defined in screen space
    // and device-independent pixels. Screen space starts in the bottom-left
    // corner and ends in the top-right corner. If it is not provided, the
    // destination region will be the entire contents of the main window.
    void blit_to_main_window(
        const RenderTexture& render_texture,
        const Material& material,
        std::optional<Rect> destination_screen_rect = std::nullopt,
        BlitFlags = {}
    );

    // Blits the texture into a rectangular region in the main window.
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
        size_t mipmap_level
    );
}
