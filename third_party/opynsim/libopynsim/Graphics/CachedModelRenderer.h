#pragma once

#include <liboscar/graphics/anti_aliasing_level.h>
#include <liboscar/graphics/scene/scene_collision.h>
#include <liboscar/maths/aabb.h>
#include <liboscar/maths/vector2.h>

#include <memory>
#include <optional>
#include <span>

namespace osc { class ModelStatePair; }
namespace osc { class Rect; }
namespace osc { class RenderTexture; }
namespace osc { class SceneCache; }
namespace osc { struct Ray; }
namespace osc { struct ModelRendererParams; }
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
            const ModelStatePair&,
            ModelRendererParams&,
            float aspectRatio
        );

        RenderTexture& onDraw(
            const ModelStatePair&,
            const ModelRendererParams&,
            Vector2 dims,
            float devicePixelRatio,
            AntiAliasingLevel antiAliasingLevel
        );
        RenderTexture& updRenderTexture();

        std::span<const SceneDecoration> getDrawlist() const;

        // Returns an `AABB` that tightly bounds all geometry in the scene, or `std::nullopt`
        // if the scene contains no geometry.
        //
        // This includes hidden/invisible elements that exist for hittesting/rim-highlighting
        // purposes.
        std::optional<AABB> bounds() const;

        // Returns an `AABB` that tightly bounds all visible geometry in the scene, or `std::nullopt`
        // if the scene contains no visible geometry.
        //
        // This is useful if (e.g.) you want to ensure a scene camera only tries to scope the visible
        // parts of a scene (#1029).
        std::optional<AABB> visibleBounds() const;

        std::optional<SceneCollision> getClosestCollision(
            const ModelRendererParams&,
            Vector2 mouseScreenPosition,
            const Rect& viewportScreenRect
        ) const;

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
