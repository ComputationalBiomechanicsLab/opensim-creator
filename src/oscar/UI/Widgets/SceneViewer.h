#pragma once

#include <memory>
#include <span>

namespace osc { struct SceneDecoration; }
namespace osc { struct SceneRendererParams; }

namespace osc
{
    // pumps scenes into a `BasicRenderer` and emits the output as a `ui::Image()`
    class SceneViewer final {
    public:
        SceneViewer();
        SceneViewer(const SceneViewer&) = delete;
        SceneViewer(SceneViewer&&) noexcept;
        SceneViewer& operator=(const SceneViewer&) = delete;
        SceneViewer& operator=(SceneViewer&&) noexcept;
        ~SceneViewer() noexcept;

        void onDraw(std::span<const SceneDecoration>, const SceneRendererParams&);

        bool isHovered() const;
        bool isLeftClicked() const;
        bool isRightClicked() const;

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
