#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITabHost; }

namespace osc
{
    class ImPlotDemoTab final : public ITab {
    public:
        static CStringView id();

        explicit ImPlotDemoTab(ParentPtr<ITabHost> const&);
        ImPlotDemoTab(ImPlotDemoTab const&) = delete;
        ImPlotDemoTab(ImPlotDemoTab&&) noexcept;
        ImPlotDemoTab& operator=(ImPlotDemoTab const&) = delete;
        ImPlotDemoTab& operator=(ImPlotDemoTab&&) noexcept;
        ~ImPlotDemoTab() noexcept override;

    private:
        UID implGetID() const final;
        CStringView implGetName() const final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
