#pragma once

#include <OpenSimCreator/Documents/Model/BasicModelStatePair.h>

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

namespace osc { class ParamBlock; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class PerformanceAnalyzerTab final : public ITab {
    public:
        PerformanceAnalyzerTab(
            ParentPtr<ITabHost> const&,
            BasicModelStatePair,
            ParamBlock const&
        );
        PerformanceAnalyzerTab(PerformanceAnalyzerTab const&) = delete;
        PerformanceAnalyzerTab(PerformanceAnalyzerTab&&) noexcept;
        PerformanceAnalyzerTab& operator=(PerformanceAnalyzerTab const&) = delete;
        PerformanceAnalyzerTab& operator=(PerformanceAnalyzerTab&&) noexcept;
        ~PerformanceAnalyzerTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnTick() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
