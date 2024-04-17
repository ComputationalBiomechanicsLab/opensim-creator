#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/RenderTextureDescriptor.h>
#include <oscar/Graphics/RenderTextureFormat.h>
#include <oscar/Graphics/RenderTextureReadWrite.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Utils/CopyOnUpdPtr.h>

#include <iosfwd>
#include <memory>

namespace osc { class RenderBuffer; }

namespace osc
{
    // render texture
    //
    // a texture that can be rendered to
    class RenderTexture final {
    public:
        RenderTexture();
        explicit RenderTexture(Vec2i dimensions);
        explicit RenderTexture(const RenderTextureDescriptor&);

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

        void reformat(const RenderTextureDescriptor&);

        std::shared_ptr<RenderBuffer> upd_color_buffer();
        std::shared_ptr<RenderBuffer> upd_depth_buffer();

        friend void swap(RenderTexture& a, RenderTexture& b) noexcept
        {
            swap(a.m_Impl, b.m_Impl);
        }

        friend bool operator==(const RenderTexture&, const RenderTexture&) = default;

        friend std::ostream& operator<<(std::ostream&, const RenderTexture&);
    private:
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };

    std::ostream& operator<<(std::ostream&, const RenderTexture&);
}
