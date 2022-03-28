#pragma once

#include "src/OpenSimBindings/SimulationClock.hpp"
#include "src/OpenSimBindings/SimulationReport.hpp"
#include "src/OpenSimBindings/SimulationStatus.hpp"
#include "src/OpenSimBindings/VirtualSimulation.hpp"

#include <nonstd/span.hpp>

#include <filesystem>
#include <memory>
#include <vector>

namespace OpenSim
{
	class Model;
}

namespace osc
{
    class ParamBlock;
}

namespace osc
{
	class StoFileSimulation final : public VirtualSimulation {
	public:
		StoFileSimulation(std::unique_ptr<OpenSim::Model>, std::filesystem::path stoFilePath);
		StoFileSimulation(StoFileSimulation const&) = delete;
		StoFileSimulation(StoFileSimulation&&) noexcept;
		StoFileSimulation& operator=(StoFileSimulation const&) = delete;
		StoFileSimulation& operator=(StoFileSimulation&&) noexcept;
		~StoFileSimulation() noexcept;

        SynchronizedValueGuard<OpenSim::Model const> getModel() const override;

		int getNumReports() const override;
		SimulationReport getSimulationReport(int reportIndex) const override;
		std::vector<SimulationReport> getAllSimulationReports() const override;

		SimulationStatus getStatus() const override;
		SimulationClock::time_point getCurTime() const override;
		SimulationClock::time_point getStartTime() const override;
		SimulationClock::time_point getEndTime() const override;
		float getProgress() const override;
		ParamBlock const& getParams() const override;
		nonstd::span<OutputExtractor const> getOutputExtractors() const override;

		void requestStop() override;
		void stop() override;

		class Impl;
	private:
		std::unique_ptr<Impl> m_Impl;
	};
}
