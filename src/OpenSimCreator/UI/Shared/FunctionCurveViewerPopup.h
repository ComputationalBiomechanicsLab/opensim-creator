#pragma once

#include <oscar/UI/Panels/IPanel.h>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class Function; }
namespace osc { class IModelStatePair; }

namespace osc
{

    class FunctionCurveViewerPanel final : public IPanel {
    public:
        FunctionCurveViewerPanel(
            std::string_view panelName,
            std::shared_ptr<const IModelStatePair> targetModel,
            std::function<const OpenSim::Function*()> functionGetter
        );
        FunctionCurveViewerPanel(const FunctionCurveViewerPanel&) = delete;
        FunctionCurveViewerPanel(FunctionCurveViewerPanel&&) noexcept;
        FunctionCurveViewerPanel& operator=(const FunctionCurveViewerPanel&) = delete;
        FunctionCurveViewerPanel& operator=(FunctionCurveViewerPanel&&) noexcept;
        ~FunctionCurveViewerPanel() noexcept;

    private:
        CStringView impl_get_name() const final;
        bool impl_is_open() const final;
        void impl_open() final;
        void impl_close() final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
