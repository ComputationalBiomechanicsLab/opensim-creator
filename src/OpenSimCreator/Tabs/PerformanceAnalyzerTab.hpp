#pragma once

#include "OpenSimCreator/BasicModelStatePair.hpp"

#include <oscar/Tabs/Tab.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <memory>

namespace osc { class ParamBlock; }
namespace osc { class TabHost; }

namespace osc
{
    class PerformanceAnalyzerTab final : public Tab {
    public:
        PerformanceAnalyzerTab(
            std::weak_ptr<TabHost>,
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
