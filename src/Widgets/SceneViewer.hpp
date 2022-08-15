#pragma once

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>

namespace osc { class SceneDecoration; }
namespace osc { class SceneRendererParams; }

namespace osc
{
    // pumps scenes into a `osc::BasicRenderer` and emits the output as an `ImGui::Image()`
    class SceneViewer final {
    public:
        SceneViewer();
        SceneViewer(SceneViewer const&) = delete;
        SceneViewer(SceneViewer&&) noexcept;
        SceneViewer& operator=(SceneViewer const&) = delete;
        SceneViewer& operator=(SceneViewer&&) noexcept;
        ~SceneViewer() noexcept;

        void draw(nonstd::span<SceneDecoration const>, SceneRendererParams const&);

        bool isHovered() const;
        bool isLeftClicked() const;
        bool isRightClicked() const;

    private:
        class Impl;
        Impl* m_Impl;
    };
}
