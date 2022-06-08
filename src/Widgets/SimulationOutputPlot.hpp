#pragma once

#include "src/Maths/Rect.hpp"
#include "src/OpenSimBindings/OutputExtractor.hpp"

#include <nonstd/span.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>

namespace osc
{
	class SimulatorUIAPI;
}

namespace osc
{
	class SimulationOutputPlot final {
	public:
		SimulationOutputPlot(SimulatorUIAPI*, OutputExtractor, float height);
		SimulationOutputPlot(SimulationOutputPlot const&) = delete;
		SimulationOutputPlot(SimulationOutputPlot&&) noexcept;
		SimulationOutputPlot& operator=(SimulationOutputPlot const&) = delete;
		SimulationOutputPlot& operator=(SimulationOutputPlot&&) noexcept;
		~SimulationOutputPlot() noexcept;

		void draw();

	private:
		friend std::filesystem::path TryPromptAndSaveAllAsCSV(nonstd::span<SimulationOutputPlot>);

		class Impl;
		Impl* m_Impl;
	};

	// returns empty path if not saved
	std::filesystem::path TryPromptAndSaveAllUserDesiredOutputsAsCSV(SimulatorUIAPI&);
}