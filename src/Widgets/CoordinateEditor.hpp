#pragma once

#include <memory>

namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class CoordinateEditor final {
    public:
        explicit CoordinateEditor(std::shared_ptr<UndoableModelStatePair>);
        CoordinateEditor(CoordinateEditor const&) = delete;
        CoordinateEditor(CoordinateEditor&&) noexcept;
        CoordinateEditor& operator=(CoordinateEditor const&) = delete;
        CoordinateEditor& operator=(CoordinateEditor&&) noexcept;
        ~CoordinateEditor() noexcept;

        void draw();

    private:
        class Impl;
        Impl* m_Impl;
    };
}
