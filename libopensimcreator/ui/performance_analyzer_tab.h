#pragma once

#include <libopensimcreator/documents/model/basic_model_state_pair_with_shared_environment.h>

#include <liboscar/ui/tabs/tab.h>

namespace osc { class ParamBlock; }
namespace osc { class Widget; }

namespace osc
{
    class PerformanceAnalyzerTab final : public Tab {
    public:
        explicit PerformanceAnalyzerTab(
            Widget* parent,
            BasicModelStatePairWithSharedEnvironment,
            const ParamBlock&
        );

    private:
        void impl_on_tick() final;
        void impl_on_draw() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
