#pragma once

#include <oscar/UI/Widgets/IPopup.h>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class Function; }
namespace osc { class IConstModelStatePair; }

namespace osc
{

    class FunctionCurveViewerPopup final : public IPopup {
    public:
        FunctionCurveViewerPopup(
            std::string_view popupName,
            std::shared_ptr<const IConstModelStatePair> targetModel,
            std::function<const OpenSim::Function*()> functionGetter
        );
        FunctionCurveViewerPopup(const FunctionCurveViewerPopup&) = delete;
        FunctionCurveViewerPopup(FunctionCurveViewerPopup&&) noexcept;
        FunctionCurveViewerPopup& operator=(const FunctionCurveViewerPopup&) = delete;
        FunctionCurveViewerPopup& operator=(FunctionCurveViewerPopup&&) noexcept;
        ~FunctionCurveViewerPopup() noexcept;

    private:
        bool impl_is_open() const final;
        void impl_open() final;
        void impl_close() final;
        bool impl_begin_popup() final;
        void impl_on_draw() final;
        void impl_end_popup() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
