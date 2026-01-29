#pragma once

#include <liboscar/graphics/anti_aliasing_level.h>
#include <liboscar/graphics/scene/scene_collision.h>
#include <liboscar/maths/aabb.h>
#include <liboscar/maths/vector2.h>

#include <memory>
#include <optional>
#include <span>

namespace opyn { struct ModelRendererParams; }
namespace opyn { class ModelStatePair; }
namespace osc { class Rect; }
namespace osc { class RenderTexture; }
namespace osc { class SceneCache; }
namespace osc { struct Ray; }
namespace osc { struct SceneDecoration; }

namespace opyn
{
    class CachedModelRenderer final {
    public:
        explicit CachedModelRenderer(const std::shared_ptr<osc::SceneCache>&);
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

        osc::RenderTexture& onDraw(
            const ModelStatePair&,
            const ModelRendererParams&,
            osc::Vector2 dims,
            float devicePixelRatio,
            osc::AntiAliasingLevel antiAliasingLevel
        );
        osc::RenderTexture& updRenderTexture();

        std::span<const osc::SceneDecoration> getDrawlist() const;

        // Returns an `AABB` that tightly bounds all geometry in the scene, or `std::nullopt`
        // if the scene contains no geometry.
        //
        // This includes hidden/invisible elements that exist for hittesting/rim-highlighting
        // purposes.
        std::optional<osc::AABB> bounds() const;

        // Returns an `AABB` that tightly bounds all visible geometry in the scene, or `std::nullopt`
        // if the scene contains no visible geometry.
        //
        // This is useful if (e.g.) you want to ensure a scene camera only tries to scope the visible
        // parts of a scene (#1029).
        std::optional<osc::AABB> visibleBounds() const;

        std::optional<osc::SceneCollision> getClosestCollision(
            const ModelRendererParams&,
            osc::Vector2 mouseScreenPosition,
            const osc::Rect& viewportScreenRect
        ) const;

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
