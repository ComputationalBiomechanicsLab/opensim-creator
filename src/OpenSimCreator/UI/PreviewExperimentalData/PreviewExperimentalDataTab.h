#pragma once

#include <oscar/UI/Tabs/Tab.h>
#include <oscar/Utils/ParentPtr.h>

namespace osc { class IMainUIStateAPI; }

namespace osc
{
    class PreviewExperimentalDataTab final : public Tab {
    public:
        static CStringView id();

        explicit PreviewExperimentalDataTab(const ParentPtr<IMainUIStateAPI>&);

    private:
        void impl_on_mount() final;
        void impl_on_unmount() final;
        void impl_on_tick() final;
        void impl_on_draw_main_menu() final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
