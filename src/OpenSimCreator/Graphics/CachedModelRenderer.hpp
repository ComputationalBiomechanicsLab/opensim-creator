#pragma once

#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Graphics/AntiAliasingLevel.hpp>
#include <oscar/Scene/SceneCollision.hpp>

#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace osc { class AppConfig; }
namespace osc { struct Line; }
namespace osc { struct ModelRendererParams; }
namespace osc { struct Rect; }
namespace osc { class RenderTexture; }
namespace osc { struct SceneDecoration; }
namespace osc { class SceneCache; }
namespace osc { class ShaderCache; }
namespace osc { class IConstModelStatePair; }

namespace osc
{
    class CachedModelRenderer final {
    public:
        CachedModelRenderer(
            AppConfig const&,
            std::shared_ptr<SceneCache> const&,
            ShaderCache&
        );
        CachedModelRenderer(CachedModelRenderer const&) = delete;
        CachedModelRenderer(CachedModelRenderer&&) noexcept;
        CachedModelRenderer& operator=(CachedModelRenderer const&) = delete;
        CachedModelRenderer& operator=(CachedModelRenderer&&) noexcept;
        ~CachedModelRenderer() noexcept;

        void autoFocusCamera(
            IConstModelStatePair const&,
            ModelRendererParams&,
            float aspectRatio
        );

        RenderTexture& onDraw(
            IConstModelStatePair const&,
            ModelRendererParams const&,
            Vec2 dims,
            AntiAliasingLevel antiAliasingLevel
        );
        RenderTexture& updRenderTexture();

        std::span<SceneDecoration const> getDrawlist() const;
        std::optional<AABB> getRootAABB() const;
        std::optional<SceneCollision> getClosestCollision(
            ModelRendererParams const&,
            Vec2 mouseScreenPos,
            Rect const& viewportScreenRect
        ) const;

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
