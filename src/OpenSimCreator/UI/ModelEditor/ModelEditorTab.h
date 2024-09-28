#pragma once

#include <oscar/UI/Tabs/Tab.h>

namespace OpenSim { class Model; }
namespace osc { class MainUIScreen; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelEditorTab final : public Tab {
    public:
        explicit ModelEditorTab(
            MainUIScreen&
        );
        explicit ModelEditorTab(
            MainUIScreen&,
            const OpenSim::Model&
        );
        explicit ModelEditorTab(
            MainUIScreen&,
            std::unique_ptr<OpenSim::Model>,
            float fixupScaleFactor = 1.0f
        );
        explicit ModelEditorTab(
            MainUIScreen&,
            std::unique_ptr<UndoableModelStatePair>
        );

    private:
        bool impl_is_unsaved() const final;
        bool impl_try_save() final;
        void impl_on_mount() final;
        void impl_on_unmount() final;
        bool impl_on_event(Event&) final;
        void impl_on_tick() final;
        void impl_on_draw_main_menu() final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
