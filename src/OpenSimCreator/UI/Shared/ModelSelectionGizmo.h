#pragma once

#include <oscar/UI/oscimgui.h>

#include <memory>

namespace osc { struct PolarPerspectiveCamera; }
namespace osc { struct Rect; }
namespace osc { class IModelStatePair; }

namespace osc
{
    class ModelSelectionGizmo final {
    public:
        explicit ModelSelectionGizmo(std::shared_ptr<IModelStatePair>);
        ModelSelectionGizmo(const ModelSelectionGizmo&);
        ModelSelectionGizmo(ModelSelectionGizmo&&) noexcept;
        ModelSelectionGizmo& operator=(const ModelSelectionGizmo&);
        ModelSelectionGizmo& operator=(ModelSelectionGizmo&&) noexcept;
        ~ModelSelectionGizmo() noexcept;

        bool isUsing() const { return m_Gizmo.is_using(); }
        bool isOver() const { return m_Gizmo.is_over(); }

        bool handleKeyboardInputs() { return m_Gizmo.handle_keyboard_inputs(); }
        void onDraw(const Rect& screenRect, const PolarPerspectiveCamera&);

        ui::GizmoOperation getOperation() const { return m_Gizmo.operation(); }
        void setOperation(ui::GizmoOperation op) { m_Gizmo.set_operation(op); }

        ui::GizmoMode getMode() { return m_Gizmo.mode(); }
        void setMode(ui::GizmoMode mode) { m_Gizmo.set_mode(mode); }

        operator ui::Gizmo& () { return m_Gizmo; }

    private:
        std::shared_ptr<IModelStatePair> m_Model;
        ui::Gizmo m_Gizmo;
    };
}
