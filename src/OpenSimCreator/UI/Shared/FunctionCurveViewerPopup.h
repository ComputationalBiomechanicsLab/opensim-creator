#pragma once

#include <oscar/UI/Panels/Panel.h>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class Function; }
namespace osc { class IModelStatePair; }

namespace osc
{

    class FunctionCurveViewerPanel final : public Panel {
    public:
        explicit FunctionCurveViewerPanel(
            std::string_view panelName,
            std::shared_ptr<const IModelStatePair> targetModel,
            std::function<const OpenSim::Function*()> functionGetter
        );

    private:
        void impl_draw_content() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}
