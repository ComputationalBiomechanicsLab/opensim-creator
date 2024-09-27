#pragma once

#include <oscar/UI/Tabs/Tab.h>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class Screenshot; }
namespace osc { class ITabHost; }

namespace osc
{
    class ScreenshotTab final : public Tab {
    public:
        ScreenshotTab(const ParentPtr<ITabHost>&, Screenshot&&);

    private:
        void impl_on_draw_main_menu() final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
