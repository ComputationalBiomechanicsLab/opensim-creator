#pragma once

#include <liboscar/ui/oscimgui.h>

#include <memory>

namespace opyn
{
    class ModelStatePair;
}

namespace osc
{
    class Camera;
    class Rect;

    class ModelSelectionGizmo final {
    public:
        explicit ModelSelectionGizmo(std::shared_ptr<opyn::ModelStatePair>);
        ModelSelectionGizmo(const ModelSelectionGizmo&);
        ModelSelectionGizmo(ModelSelectionGizmo&&) noexcept;
        ModelSelectionGizmo& operator=(const ModelSelectionGizmo&);
        ModelSelectionGizmo& operator=(ModelSelectionGizmo&&) noexcept;
        ~ModelSelectionGizmo() noexcept;

        bool is_using() const { return gizmo_.is_using(); }
        bool is_over() const { return gizmo_.is_over(); }

        bool handle_keyboard_inputs() { return gizmo_.handle_keyboard_inputs(); }
        void on_draw(const Rect& screen_rect, const Camera&);

        ui::GizmoOperation get_operation() const { return gizmo_.operation(); }
        void set_operation(ui::GizmoOperation op) { gizmo_.set_operation(op); }

        ui::GizmoMode get_mode() { return gizmo_.mode(); }
        void set_mode(ui::GizmoMode mode) { gizmo_.set_mode(mode); }

        operator ui::Gizmo&() { return gizmo_; }

    private:
        std::shared_ptr<opyn::ModelStatePair> model_;
        ui::Gizmo gizmo_;
    };
}
