#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

namespace OpenSim { class Model; }
namespace osc { class IMainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelEditorTab final : public ITab {
    public:
        explicit ModelEditorTab(
            const ParentPtr<IMainUIStateAPI>&
        );
        explicit ModelEditorTab(
            const ParentPtr<IMainUIStateAPI>&,
            const OpenSim::Model&
        );
        explicit ModelEditorTab(
            const ParentPtr<IMainUIStateAPI>&,
            std::unique_ptr<OpenSim::Model>,
            float fixupScaleFactor = 1.0f
        );
        explicit ModelEditorTab(
            const ParentPtr<IMainUIStateAPI>&,
            std::unique_ptr<UndoableModelStatePair>
        );
        ModelEditorTab(const ModelEditorTab&) = delete;
        ModelEditorTab(ModelEditorTab&&) noexcept;
        ModelEditorTab& operator=(const ModelEditorTab&) = delete;
        ModelEditorTab& operator=(ModelEditorTab&&) noexcept;
        ~ModelEditorTab() noexcept override;

    private:
        UID impl_get_id() const final;
        CStringView impl_get_name() const final;
        bool impl_is_unsaved() const final;
        bool impl_try_save() final;
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(Event&) final;
        void impl_on_tick() final;
        void impl_on_draw_main_menu() final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
