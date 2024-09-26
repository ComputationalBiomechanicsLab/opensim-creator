#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Scene/SceneCollision.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Vec2.h>

#include <memory>
#include <optional>
#include <span>

namespace osc { class IModelStatePair; }
namespace osc { class RenderTexture; }
namespace osc { class SceneCache; }
namespace osc { struct Line; }
namespace osc { struct ModelRendererParams; }
namespace osc { struct Rect; }
namespace osc { struct SceneDecoration; }

namespace osc
{
    class CachedModelRenderer final {
    public:
        explicit CachedModelRenderer(const std::shared_ptr<SceneCache>&);
        CachedModelRenderer(const CachedModelRenderer&) = delete;
        CachedModelRenderer(CachedModelRenderer&&) noexcept;
        CachedModelRenderer& operator=(const CachedModelRenderer&) = delete;
        CachedModelRenderer& operator=(CachedModelRenderer&&) noexcept;
        ~CachedModelRenderer() noexcept;

        void autoFocusCamera(
            const IModelStatePair&,
            ModelRendererParams&,
            float aspectRatio
        );

        RenderTexture& onDraw(
            const IModelStatePair&,
            const ModelRendererParams&,
            Vec2 dims,
            AntiAliasingLevel antiAliasingLevel
        );
        RenderTexture& updRenderTexture();

        std::span<const SceneDecoration> getDrawlist() const;
        std::optional<AABB> bounds() const;
        std::optional<SceneCollision> getClosestCollision(
            const ModelRendererParams&,
            Vec2 mouseScreenPos,
            const Rect& viewportScreenRect
        ) const;

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
