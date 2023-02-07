#pragma once

#include <imgui.h>
#include <ImGuizmo.h>  // care: must be included after imgui

#include <memory>

namespace osc { class PolarPerspectiveCamera; }
namespace osc { struct Rect; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelSelectionGizmo final {
    public:
        explicit ModelSelectionGizmo(std::shared_ptr<UndoableModelStatePair>);
        ModelSelectionGizmo(ModelSelectionGizmo const&);
        ModelSelectionGizmo(ModelSelectionGizmo&&) noexcept;
        ModelSelectionGizmo& operator=(ModelSelectionGizmo const&);
        ModelSelectionGizmo& operator=(ModelSelectionGizmo&&) noexcept;
        ~ModelSelectionGizmo() noexcept;

        bool isUsing() const;

        void draw(Rect const& screenRect, PolarPerspectiveCamera const&);

        ImGuizmo::OPERATION getOperation() const
        {
            return m_GizmoOperation;
        }
        void setOperation(ImGuizmo::OPERATION newOperation)
        {
            m_GizmoOperation = newOperation;
        }

        ImGuizmo::MODE getMode() const
        {
            return m_GizmoMode;
        }
        void setMode(ImGuizmo::MODE newMode)
        {
            m_GizmoMode = newMode;
        }

    private:
        std::shared_ptr<osc::UndoableModelStatePair> m_Model;
        ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE m_GizmoMode = ImGuizmo::WORLD;
        bool m_WasUsingGizmoLastFrame = false;
    };
}