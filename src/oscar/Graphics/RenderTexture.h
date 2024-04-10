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

        Vec2i getDimensions() const;
        void setDimensions(Vec2i);

        TextureDimensionality getDimensionality() const;
        void setDimensionality(TextureDimensionality);

        RenderTextureFormat getColorFormat() const;
        void setColorFormat(RenderTextureFormat);

        AntiAliasingLevel getAntialiasingLevel() const;
        void setAntialiasingLevel(AntiAliasingLevel);

        DepthStencilFormat getDepthStencilFormat() const;
        void setDepthStencilFormat(DepthStencilFormat);

        RenderTextureReadWrite getReadWrite() const;
        void setReadWrite(RenderTextureReadWrite);

        void reformat(const RenderTextureDescriptor&);

        std::shared_ptr<RenderBuffer> updColorBuffer();
        std::shared_ptr<RenderBuffer> updDepthBuffer();

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
