#pragma once

#include <libopensimcreator/Documents/Model/BasicModelStatePair.h>

#include <liboscar/UI/Tabs/Tab.h>

namespace osc { class ParamBlock; }
namespace osc { class Widget; }

namespace osc
{
    class PerformanceAnalyzerTab final : public Tab {
    public:
        explicit PerformanceAnalyzerTab(
            Widget* parent,
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
