#pragma once

#include <liboscar/ui/tabs/tab.h>
#include <liboscar/ui/tabs/tab_save_result.h>
#include <liboscar/utilities/c_string_view.h>

#include <future>

namespace OpenSim
{
    class Model;
}

namespace osc
{
    class UndoableModelStatePair;
    class Widget;

    class ModelEditorTab final : public Tab {
    public:
        static CStringView id() { return "OpenSim/ModelEditor"; }

        explicit ModelEditorTab(Widget*);
        explicit ModelEditorTab(Widget*, const OpenSim::Model&);
        explicit ModelEditorTab(
            Widget*, std::unique_ptr<OpenSim::Model>, float fixup_scale_factor = 1.0f);
        explicit ModelEditorTab(Widget*, std::unique_ptr<UndoableModelStatePair>);

    private:
        bool impl_is_unsaved() const override;
        std::future<TabSaveResult> impl_try_save() override;
        void impl_on_mount() override;
        void impl_on_unmount() override;
        bool impl_on_event(Event&) override;
        void impl_on_tick() override;
        void impl_on_draw_main_menu() override;
        void impl_on_draw() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
