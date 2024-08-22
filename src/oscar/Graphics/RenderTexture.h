#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/DepthStencilFormat.h>
#include <oscar/Graphics/RenderTextureFormat.h>
#include <oscar/Graphics/RenderTextureParams.h>
#include <oscar/Graphics/RenderTextureReadWrite.h>
#include <oscar/Graphics/SharedRenderBuffer.h>
#include <oscar/Graphics/TextureDimensionality.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Utils/CopyOnUpdPtr.h>

#include <iosfwd>
#include <memory>

namespace osc
{
    // render texture
    //
    // a texture that can be rendered to
    class RenderTexture final {
    public:
        RenderTexture();
        explicit RenderTexture(const RenderTextureParams&);

        Vec2i dimensions() const;
        void set_dimensions(Vec2i);

        TextureDimensionality dimensionality() const;
        void set_dimensionality(TextureDimensionality);

        RenderTextureFormat color_format() const;
        void set_color_format(RenderTextureFormat);

        AntiAliasingLevel anti_aliasing_level() const;
        void set_anti_aliasing_level(AntiAliasingLevel);

        DepthStencilFormat depth_stencil_format() const;
        void set_depth_stencil_format(DepthStencilFormat);

        RenderTextureReadWrite read_write() const;
        void set_read_write(RenderTextureReadWrite);

        void reformat(const RenderTextureParams&);

        SharedRenderBuffer upd_color_buffer();
        SharedRenderBuffer upd_depth_buffer();

        friend bool operator==(const RenderTexture&, const RenderTexture&) = default;

        friend std::ostream& operator<<(std::ostream&, const RenderTexture&);
    private:
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> impl_;
    };

    std::ostream& operator<<(std::ostream&, const RenderTexture&);
}
