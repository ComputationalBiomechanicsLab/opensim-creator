#pragma once

#include <memory>

namespace osc { class IModelStatePair; }
namespace osc { class PanelManager; }
namespace osc { class Widget; }

namespace osc
{
    class ModelEditorMainMenu final {
    public:
        ModelEditorMainMenu(
            Widget&,
            std::shared_ptr<PanelManager>,
            std::shared_ptr<IModelStatePair>
        );
        ModelEditorMainMenu(const ModelEditorMainMenu&) = delete;
        ModelEditorMainMenu(ModelEditorMainMenu&&) noexcept;
        ModelEditorMainMenu& operator=(const ModelEditorMainMenu&) = delete;
        ModelEditorMainMenu& operator=(ModelEditorMainMenu&&) noexcept;
        ~ModelEditorMainMenu() noexcept;

        void onDraw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
