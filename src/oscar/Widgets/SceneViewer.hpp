#pragma once

#include <nonstd/span.hpp>

#include <memory>

namespace osc { struct SceneDecoration; }
namespace osc { struct SceneRendererParams; }

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
        std::unique_ptr<Impl> m_Impl;
    };
}
