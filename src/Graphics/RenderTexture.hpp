#pragma once

#include "src/Graphics/RenderTextureDescriptor.hpp"
#include "src/Graphics/RenderTextureFormat.hpp"
#include "src/Graphics/RenderTextureReadWrite.hpp"
#include "src/Utils/CopyOnUpdPtr.hpp"

#include <glm/vec2.hpp>

#include <iosfwd>
#include <optional>

namespace osc { class RenderBuffer; }
namespace osc { class RenderTexture; }
namespace osc { void DrawTextureAsImGuiImage(RenderTexture const&, glm::vec2); }

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // render texture
    //
    // a texture that can be rendered to
    class RenderTexture final {
    public:
        RenderTexture();
        explicit RenderTexture(glm::ivec2 dimensions);
        explicit RenderTexture(RenderTextureDescriptor const&);
        RenderTexture(RenderTexture const&);
        RenderTexture(RenderTexture&&) noexcept;
        RenderTexture& operator=(RenderTexture const&);
        RenderTexture& operator=(RenderTexture&&) noexcept;
        ~RenderTexture() noexcept;

        glm::ivec2 getDimensions() const;
        void setDimensions(glm::ivec2);

        RenderTextureFormat getColorFormat() const;
        void setColorFormat(RenderTextureFormat);

        int32_t getAntialiasingLevel() const;
        void setAntialiasingLevel(int32_t);

        DepthStencilFormat getDepthStencilFormat() const;
        void setDepthStencilFormat(DepthStencilFormat);

        RenderTextureReadWrite getReadWrite() const;
        void setReadWrite(RenderTextureReadWrite);

        void reformat(RenderTextureDescriptor const& d);

        std::shared_ptr<RenderBuffer> updColorBuffer();
        std::shared_ptr<RenderBuffer> updDepthBuffer();

        friend void swap(RenderTexture& a, RenderTexture& b) noexcept
        {
            swap(a.m_Impl, b.m_Impl);
        }

    private:
        friend void osc::DrawTextureAsImGuiImage(RenderTexture const&, glm::vec2);
        void* getTextureHandleHACK() const;  // used by ImGui... for now

        friend class GraphicsBackend;
        friend bool operator==(RenderTexture const&, RenderTexture const&) noexcept;
        friend bool operator!=(RenderTexture const&, RenderTexture const&) noexcept;
        friend std::ostream& operator<<(std::ostream&, RenderTexture const&);

        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };

    inline bool operator==(RenderTexture const& a, RenderTexture const& b) noexcept
    {
        return a.m_Impl == b.m_Impl;
    }

    inline bool operator!=(RenderTexture const& a, RenderTexture const& b) noexcept
    {
        return a.m_Impl != b.m_Impl;
    }

    std::ostream& operator<<(std::ostream&, RenderTexture const&);
}