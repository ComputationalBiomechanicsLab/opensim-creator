#pragma once

#include <libOpenSimCreator/Documents/Model/BasicModelStatePair.h>

#include <liboscar/UI/Tabs/Tab.h>

namespace osc { class ParamBlock; }
namespace osc { class Widget; }

namespace osc
{
    class PerformanceAnalyzerTab final : public Tab {
    public:
        PerformanceAnalyzerTab(
            Widget&,
            BasicModelStatePair,
            const ParamBlock&
        );

    private:
        void impl_on_tick() final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
