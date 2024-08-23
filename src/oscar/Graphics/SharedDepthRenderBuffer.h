#pragma once

#include <oscar/Graphics/TextureDimensionality.h>

#include <memory>

namespace osc { struct DepthRenderBufferParams; }

namespace osc
{
    // A render buffer that's shareable between `Material`s, `RenderTarget`s,
    // and `RenderTexture`s.
    //
    // Unlike most of `osc`'s graphics classes, this one _doesn't_ have value
    // semantics. Instead, it should be thought of as a `std::shared_ptr<RenderBuffer>`. The
    // reason for this change of style is to support typical use-cases, such as:
    //
    // - Calling code allocates a `RenderTexture` (i.e. color + depth `RenderBuffer`s)
    //   (owners: `RenderTexture`)
    //
    // - Calling code wants to render to only the color part of the `RenderTexture`, so it
    //   creates a `RenderTarget` that points to the `RenderTexture`'s color `RenderBuffer`
    //   (owners: `RenderTexture`, `RenderTarget`)
    //
    // - After a render pass to the `RenderTarget`, calling code then wants to sample from
    //   the `RenderBuffer` in a downstream pass (e.g. for deferred shading, or similar), so
    //   the code sets the `RenderBuffer` in a `Material` or `MaterialPropertyBlock`
    //   (owners: `RenderTexture`, `RenderTarget`, `Material`)
    //
    // You might (rightfully) be thinking that the last step should be solved by explicitly
    // blitting the `RenderBuffer` to (e.g.) a `Texture2D`, followed by assigning that `Texture2D`
    // to the `Material`. However, in typical multi-frame rendering operations that potentially
    // allocates an additional `Texture2D`, because the calling code would have to:
    //
    // - First blit the `RenderBuffer` to a new `Texture2D`
    // - Then assign the new `Texture2D` over the old one present in the `Material` from the
    //   last frame
    //
    // The second step is problematic, because it implies there exists a point in time where
    // there's two instances of `Texture2D` hanging around. Callers would have to explicitly
    // remember to `unset` the `Texture2D` after sampling it in the downstream `Material`, which
    // is a pain in the ass to remember. Whereas `SharedRenderBuffer`s just keep everything as
    // shared references and it's assumed that the calling code knows when/where to share it.
    // This also makes it easier for graph traversal algorithms to figure out the dependency
    // chains between render passes.
    class SharedDepthRenderBuffer final {
    public:
        explicit SharedDepthRenderBuffer();
        explicit SharedDepthRenderBuffer(const DepthRenderBufferParams&);

        friend bool operator==(const SharedDepthRenderBuffer&, const SharedDepthRenderBuffer&) = default;

        SharedDepthRenderBuffer clone() const;

        TextureDimensionality dimensionality() const;

    private:
        friend class GraphicsBackend;
        friend class RenderTexture;
        class DepthRenderBuffer;

        SharedDepthRenderBuffer(const DepthRenderBuffer&);

        std::shared_ptr<DepthRenderBuffer> impl_;
    };
}
