#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Scene/SceneCollision.h>
#include <oscar/Maths/AABB.h>
#include <oscar/Maths/Vec2.h>

#include <memory>
#include <optional>
#include <span>

namespace osc { struct Line; }
namespace osc { struct ModelRendererParams; }
namespace osc { struct Rect; }
namespace osc { class RenderTexture; }
namespace osc { struct SceneDecoration; }
namespace osc { class SceneCache; }
namespace osc { class IConstModelStatePair; }

namespace osc
{
    class CachedModelRenderer final {
    public:
        explicit CachedModelRenderer(std::shared_ptr<SceneCache> const&);
        CachedModelRenderer(const CachedModelRenderer&) = delete;
        CachedModelRenderer(CachedModelRenderer&&) noexcept;
        CachedModelRenderer& operator=(const CachedModelRenderer&) = delete;
        CachedModelRenderer& operator=(CachedModelRenderer&&) noexcept;
        ~CachedModelRenderer() noexcept;

        void autoFocusCamera(
            const IConstModelStatePair&,
            ModelRendererParams&,
            float aspectRatio
        );

        RenderTexture& onDraw(
            const IConstModelStatePair&,
            const ModelRendererParams&,
            Vec2 dims,
            AntiAliasingLevel antiAliasingLevel
        );
        RenderTexture& updRenderTexture();

        std::span<SceneDecoration const> getDrawlist() const;
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
