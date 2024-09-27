#pragma once

#include <oscar/UI/Tabs/Tab.h>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class ImGuiDemoTab final : public Tab {
    public:
        static CStringView id();

        explicit ImGuiDemoTab(const ParentPtr<ITabHost>&);

    private:
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
