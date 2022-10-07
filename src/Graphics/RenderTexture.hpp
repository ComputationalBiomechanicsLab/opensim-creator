#pragma once

#include "src/Graphics/RenderTextureDescriptor.hpp"
#include "src/Graphics/RenderTextureFormat.hpp"

#include <glm/vec2.hpp>

#include <iosfwd>
#include <memory>
#include <optional>

namespace osc { class RenderTexture; }
namespace osc { void DrawTextureAsImGuiImage(RenderTexture&, glm::vec2); }

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // render texture
    //
    // a texture that can be rendered to
    class RenderTexture final {
    public:
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

        int getAntialiasingLevel() const;
        void setAntialiasingLevel(int);

        DepthStencilFormat getDepthStencilFormat() const;
        void setDepthStencilFormat(DepthStencilFormat);

        void reformat(RenderTextureDescriptor const& d);

    private:
        friend void osc::DrawTextureAsImGuiImage(RenderTexture&, glm::vec2);
        void* updTextureHandleHACK();  // used by ImGui... for now

        friend class GraphicsBackend;
        friend bool operator==(RenderTexture const&, RenderTexture const&);
        friend bool operator!=(RenderTexture const&, RenderTexture const&);
        friend bool operator<(RenderTexture const&, RenderTexture const&);
        friend std::ostream& operator<<(std::ostream&, RenderTexture const&);

        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(RenderTexture const&, RenderTexture const&);
    bool operator!=(RenderTexture const&, RenderTexture const&);
    bool operator<(RenderTexture const&, RenderTexture const&);
    std::ostream& operator<<(std::ostream&, RenderTexture const&);

    void EmplaceOrReformat(std::optional<RenderTexture>& t, RenderTextureDescriptor const& desc);
}