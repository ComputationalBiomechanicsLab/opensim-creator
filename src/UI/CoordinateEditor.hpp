#pragma once

#include <memory>

namespace osc
{
    class UiModel;
}

namespace osc
{
    struct CoordinateEditor final {
    public:
        CoordinateEditor();
        CoordinateEditor(CoordinateEditor const&) = delete;
        CoordinateEditor(CoordinateEditor&&) noexcept;
        CoordinateEditor& operator=(CoordinateEditor const&) = delete;
        CoordinateEditor& operator=(CoordinateEditor&&) noexcept;
        ~CoordinateEditor() noexcept;

        // returns `true` if `State` was edited by the coordinate editor
        bool draw(UiModel&);

        class Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
