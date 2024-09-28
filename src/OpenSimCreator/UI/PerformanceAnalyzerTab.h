#pragma once

#include <OpenSimCreator/Documents/Model/BasicModelStatePair.h>
#include <OpenSimCreator/UI/MainUIScreen.h>

#include <oscar/UI/Tabs/Tab.h>
#include <oscar/Utils/ParentPtr.h>

namespace osc { class ParamBlock; }

namespace osc
{
    class PerformanceAnalyzerTab final : public Tab {
    public:
        PerformanceAnalyzerTab(
            const ParentPtr<MainUIScreen>&,
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
