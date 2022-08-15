#pragma once

#include "src/OpenSimBindings/BasicModelStatePair.hpp"
#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <SDL_events.h>

namespace osc { class ParamBlock; }
namespace osc { class TabHost; }

namespace osc
{
    class PerformanceAnalyzerTab final : public Tab {
    public:
        PerformanceAnalyzerTab(TabHost*, BasicModelStatePair, ParamBlock const&);
        PerformanceAnalyzerTab(PerformanceAnalyzerTab const&) = delete;
        PerformanceAnalyzerTab(PerformanceAnalyzerTab&&) noexcept;
        PerformanceAnalyzerTab& operator=(PerformanceAnalyzerTab const&) = delete;
        PerformanceAnalyzerTab& operator=(PerformanceAnalyzerTab&&) noexcept;
        ~PerformanceAnalyzerTab() noexcept override;

    private:
        UID implGetID() const override;
        CStringView implGetName() const override;
        TabHost* implParent() const override;
        void implOnMount() override;
        void implOnUnmount() override;
        bool implOnEvent(SDL_Event const&) override;
        void implOnTick() override;
        void implOnDrawMainMenu() override;
        void implOnDraw() override;

        class Impl;
        Impl* m_Impl;
    };
}
