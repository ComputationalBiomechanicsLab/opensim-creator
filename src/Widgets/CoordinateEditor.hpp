#pragma once

#include <memory>

namespace osc
{
    class UndoableUiModel;
}

namespace osc
{
    struct CoordinateEditor final {
    public:
        explicit CoordinateEditor(std::shared_ptr<UndoableUiModel>);
        CoordinateEditor(CoordinateEditor const&) = delete;
        CoordinateEditor(CoordinateEditor&&) noexcept;
        CoordinateEditor& operator=(CoordinateEditor const&) = delete;
        CoordinateEditor& operator=(CoordinateEditor&&) noexcept;
        ~CoordinateEditor() noexcept;
        
        bool draw();  // returns true if an edit was made

        class Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
