#pragma once

#include "src/Tabs/TabHost.hpp"

namespace osc { class ParamBlock; }
namespace osc { class OutputExtractor; }

namespace osc
{
    // API access to shared state between main UI tabs
    //
    // this is how individual UI tabs inter-communicate (e.g. by sharing data, closing other tabs, etc.)
    class MainUIStateAPI : public TabHost {
    protected:
        MainUIStateAPI() = default;
        MainUIStateAPI(MainUIStateAPI const&) = default;
        MainUIStateAPI(MainUIStateAPI&&) noexcept = default;
        MainUIStateAPI& operator=(MainUIStateAPI const&) = default;
        MainUIStateAPI& operator=(MainUIStateAPI&&) noexcept = default;
    public:
        virtual ~MainUIStateAPI() noexcept = default;

        virtual ParamBlock const& getSimulationParams() const = 0;
        virtual ParamBlock& updSimulationParams() = 0;

        virtual int getNumUserOutputExtractors() const = 0;
        virtual OutputExtractor const& getUserOutputExtractor(int) const = 0;
        virtual void addUserOutputExtractor(OutputExtractor const&) = 0;
        virtual void removeUserOutputExtractor(int) = 0;
        virtual bool hasUserOutputExtractor(OutputExtractor const&) const = 0;
        virtual bool removeUserOutputExtractor(OutputExtractor const&) = 0;
    };
}
