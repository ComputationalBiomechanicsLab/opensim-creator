#pragma once

#include <SDL_events.h>
#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

namespace osc { class IMainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelEditorTab final : public ITab {
    public:
        ModelEditorTab(
            ParentPtr<IMainUIStateAPI> const&,
            std::unique_ptr<UndoableModelStatePair>
        );
        ModelEditorTab(ModelEditorTab const&) = delete;
        ModelEditorTab(ModelEditorTab&&) noexcept;
        ModelEditorTab& operator=(ModelEditorTab const&) = delete;
        ModelEditorTab& operator=(ModelEditorTab&&) noexcept;
        ~ModelEditorTab() noexcept override;

    private:
        UID impl_get_id() const final;
        CStringView impl_get_name() const final;
        bool impl_is_unsaved() const final;
        bool impl_try_save() final;
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(SDL_Event const&) final;
        void impl_on_tick() final;
        void impl_on_draw_main_menu() final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
