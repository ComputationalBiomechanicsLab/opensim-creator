#pragma once

#include <OpenSimCreator/UI/IMainUIStateAPI.h>

#include <oscar/UI/Tabs/Tab.h>
#include <oscar/Utils/ParentPtr.h>

namespace osc::mow
{
    class ModelWarperTab final : public Tab {
    public:
        static CStringView id();

        explicit ModelWarperTab(const ParentPtr<IMainUIStateAPI>&);

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
