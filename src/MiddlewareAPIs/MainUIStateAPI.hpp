#pragma once

#include "src/Tabs/TabHost.hpp"

namespace osc
{
	class ParamBlock;
	class OutputExtractor;
}

namespace osc
{
	// API access to shared state between main UI tabs
	//
	// this is how individual UI tabs inter-communicate (e.g. by sharing data, closing other tabs, etc.)
	class MainUIStateAPI : public TabHost {
	public:
        MainUIStateAPI() = default;
        MainUIStateAPI(MainUIStateAPI const&) = delete;
        MainUIStateAPI(MainUIStateAPI&&) noexcept = delete;
        MainUIStateAPI& operator=(MainUIStateAPI const&) = delete;
        MainUIStateAPI& operator=(MainUIStateAPI&&) noexcept = delete;
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
