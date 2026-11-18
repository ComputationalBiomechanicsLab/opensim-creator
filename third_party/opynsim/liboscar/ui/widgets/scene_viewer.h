#pragma once

#include <memory>
#include <span>

namespace osc
{
    class SceneCache;
    struct SceneDecoration;
    struct SceneRendererParams;

    // pumps scenes into a `BasicRenderer` and emits the output as a `ui::Image()`
    class SceneViewer final {
    public:
        explicit SceneViewer(SceneCache&);
        SceneViewer(const SceneViewer&) = delete;
        SceneViewer(SceneViewer&&) noexcept;
        SceneViewer& operator=(const SceneViewer&) = delete;
        SceneViewer& operator=(SceneViewer&&) noexcept;
        ~SceneViewer() noexcept;

        void on_draw(std::span<const SceneDecoration>, const SceneRendererParams&);

        bool is_hovered() const;
        bool is_left_clicked() const;
        bool is_right_clicked() const;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
