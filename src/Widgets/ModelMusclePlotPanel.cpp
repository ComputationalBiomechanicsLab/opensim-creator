#include "ModelMusclePlotPanel.hpp"

#include "src/Actions/ActionFunctions.hpp"
#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/ModelStateCommit.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/Cpp20Shims.hpp"
#include "src/Utils/UID.hpp"

#include <imgui.h>
#include <implot.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>

#include <atomic>
#include <future>
#include <memory>
#include <string_view>
#include <sstream>
#include <utility>

static bool SortByComponentName(OpenSim::Component const* p1, OpenSim::Component const* p2)
{
	return p1->getName() < p2->getName();
}

namespace
{
	class MuscleOutput {
	public:
		MuscleOutput(char const* name, char const* units, double(*getter)(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)) :
			m_Name{std::move(name)},
			m_Units{std::move(units)},
			m_Getter{std::move(getter)}
		{
		}

		char const* getName() const
		{
			return m_Name.c_str();
		}

		char const* getUnits() const
		{
			return m_Units.c_str();
		}

		double operator()(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c) const
		{
			return m_Getter(st, muscle, c);
		}

	private:
		friend bool operator<(MuscleOutput const&, MuscleOutput const&);
		friend bool operator==(MuscleOutput const&, MuscleOutput const&);
		friend bool operator!=(MuscleOutput const&, MuscleOutput const&);

		osc::CStringView m_Name;
		osc::CStringView m_Units;
		double(*m_Getter)(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c);
	};

	bool operator<(MuscleOutput const& a, MuscleOutput const& b)
	{
		return a.m_Name < b.m_Name;
	}

	bool operator==(MuscleOutput const& a, MuscleOutput const& b)
	{
		return a.m_Name == b.m_Name && a.m_Units == b.m_Units && a.m_Getter == b.m_Getter;
	}

	bool operator!=(MuscleOutput const& a, MuscleOutput const& b)
	{
		return !(a == b);
	}
}

static double GetMomentArm(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return muscle.getGeometryPath().computeMomentArm(st, c);
}

static double GetFiberLength(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return muscle.getFiberLength(st);
}

static double GetTendonLength(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return muscle.getTendonLength(st);
}

static double GetPennationAngle(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return glm::degrees(muscle.getPennationAngle(st));
}

static double GetNormalizedFiberLength(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return muscle.getNormalizedFiberLength(st);
}

static double GetTendonStrain(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return muscle.getTendonStrain(st);
}

static double GetFiberPotentialEnergy(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return muscle.getFiberPotentialEnergy(st);
}

static double GetTendonPotentialEnergy(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return muscle.getTendonPotentialEnergy(st);
}

static double GetMusclePotentialEnergy(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return muscle.getMusclePotentialEnergy(st);
}

static double GetTendonForce(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return muscle.getTendonForce(st);
}

static double GetActiveFiberForce(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return muscle.getActiveFiberForce(st);
}

static double GetPassiveFiberForce(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return muscle.getPassiveFiberForce(st);
}

static double GetTotalFiberForce(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return muscle.getFiberForce(st);
}

static double GetFiberStiffness(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return muscle.getFiberStiffness(st);
}

static double GetFiberStiffnessAlongTendon(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return muscle.getFiberStiffnessAlongTendon(st);
}

static double GetTendonStiffness(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return muscle.getTendonStiffness(st);
}

static double GetMuscleStiffness(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return muscle.getMuscleStiffness(st);
}

static double GetFiberActivePower(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return muscle.getFiberActivePower(st);
}

static double GetFiberPassivePower(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return muscle.getFiberActivePower(st);
}

static double GetTendonPower(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return muscle.getTendonPower(st);
}

static double GetMusclePower(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const&)
{
	return muscle.getTendonPower(st);
}

static MuscleOutput GetDefaultMuscleOutput()
{
	return MuscleOutput{"Moment Arm", "Unitless", GetMomentArm};
}

static std::vector<MuscleOutput> InitMuscleOutputs()
{
	std::vector<MuscleOutput> rv =
	{{
		GetDefaultMuscleOutput(),
		{"Tendon Length", "m", GetTendonLength},
		{"Fiber Length", "m", GetFiberLength},
		{"Pennation Angle", "deg", GetPennationAngle},
		{"Normalized Fiber Length", "Unitless", GetNormalizedFiberLength},
		{"Tendon Strain", "Unitless", GetTendonStrain},
		{"Fiber Potential Energy", "J", GetFiberPotentialEnergy},
		{"Tendon Potential Energy", "J", GetTendonPotentialEnergy},
		{"Muscle Potential Energy", "J", GetMusclePotentialEnergy},
		{"Tendon Force", "N", GetTendonForce},
		{"Active Fiber Force", "N", GetActiveFiberForce},
		{"Passive Fiber Force", "N", GetPassiveFiberForce},
		{"Total Fiber Force", "N", GetTotalFiberForce},
		{"Fiber Stiffness", "N/m", GetFiberStiffness},
		{"Fiber Stiffness Along Tendon", "N/m", GetFiberStiffnessAlongTendon},
		{"Tendon Stiffness", "N/m", GetTendonStiffness},
		{"Muscle Stiffness", "N/m", GetMuscleStiffness},
		{"Fiber Active Power", "W", GetFiberActivePower},
		{"Fiber Passive Power", "W", GetFiberPassivePower},
		{"Tendon Power", "W", GetTendonPower},
		{"Muscle Power", "W", GetMusclePower},
	}};
	std::sort(rv.begin(), rv.end());
	return rv;
}

static std::vector<MuscleOutput> const& GetMuscleOutputs()
{
	static std::vector<MuscleOutput> g_MuscleOutputs = InitMuscleOutputs();
	return g_MuscleOutputs;
}

// background computing stuff
namespace
{
	// parameters used to create a plot
	class PlotParameters final {
	public:
		PlotParameters(osc::ModelStateCommit commit,
			           OpenSim::ComponentPath coordinatePath,
			           OpenSim::ComponentPath musclePath,
			           MuscleOutput output,
			           int requestedNumDataPoints) :
			m_Commit{std::move(commit)},
			m_CoordinatePath{std::move(coordinatePath)},
			m_MusclePath{std::move(musclePath)},
			m_Output{std::move(output)},
			m_RequestedNumDataPoints{std::move(requestedNumDataPoints)}
		{
		}

		osc::ModelStateCommit const& getCommit() const
		{
			return m_Commit;
		}

		void setCommit(osc::ModelStateCommit const& commit)
		{
			m_Commit = commit;
		}

		OpenSim::ComponentPath const& getCoordinatePath() const
		{
			return m_CoordinatePath;
		}

		void setCoordinatePath(OpenSim::ComponentPath const& cp)
		{
			m_CoordinatePath = cp;
		}

		OpenSim::ComponentPath const& getMusclePath() const
		{
			return m_MusclePath;
		}

		void setMusclePath(OpenSim::ComponentPath const& cp)
		{
			m_MusclePath = cp;
		}

		MuscleOutput const& getMuscleOutput() const
		{
			return m_Output;
		}

		void setMuscleOutput(MuscleOutput const& output)
		{
			m_Output = output;
		}

		int getNumRequestedDataPoints() const
		{
			return m_RequestedNumDataPoints;
		}

		void setNumRequestedDataPoints(int v)
		{
			m_RequestedNumDataPoints = v;
		}

	private:
		friend bool operator==(PlotParameters const&, PlotParameters const&);
		friend bool operator!=(PlotParameters const&, PlotParameters const&);

		osc::ModelStateCommit m_Commit;
		OpenSim::ComponentPath m_CoordinatePath;
		OpenSim::ComponentPath m_MusclePath;
		MuscleOutput m_Output;
		int m_RequestedNumDataPoints;
	};

	bool operator==(PlotParameters const& a, PlotParameters const& b)
	{
		return a.m_Commit == b.m_Commit &&
			a.m_CoordinatePath == b.m_CoordinatePath &&
			a.m_MusclePath == b.m_MusclePath &&
			a.m_Output == b.m_Output &&
			a.m_RequestedNumDataPoints == b.m_RequestedNumDataPoints;
	}

	bool operator!=(PlotParameters const& a, PlotParameters const& b)
	{
		return !(a == b);
	}

	// a plot, constructed according to input parameters
	class Plot final {
	public:
		explicit Plot(PlotParameters const& parameters) :
			m_Parameters{parameters}
		{
		}

		Plot(PlotParameters const& parameters,
			 std::vector<float> xValues,
			 std::vector<float> yValues) :
			m_Parameters{parameters},
			m_XValues{std::move(xValues)},
			m_YValues{std::move(yValues)}
		{
		}

		PlotParameters const& getParameters() const
		{
			return m_Parameters;
		}

		nonstd::span<float const> getXValues() const
		{
			return m_XValues;
		}

		nonstd::span<float const> getYValues() const
		{
			return m_YValues;
		}

	private:
		PlotParameters m_Parameters;
		std::vector<float> m_XValues;
		std::vector<float> m_YValues;
	};

	class ThreadsafeSharedState final {
	public:
		float getProgress() const { return m_Progress; }
		void setProgress(float v)
		{
			m_Progress = v;
			// HACK: the UI needs to redraw a progress upate
			osc::App::upd().requestRedraw();
		}

	private:
		std::atomic<float> m_Progress = 0.0f;
	};

	std::unique_ptr<Plot> ComputePlotSynchronously(osc::stop_token stopToken,
		                                           std::unique_ptr<PlotParameters> params,
		                                           std::shared_ptr<ThreadsafeSharedState> shared)
	{
		if (params->getNumRequestedDataPoints() <= 0)
		{
			return std::make_unique<Plot>(*params);  // empty plot
		}

		auto model = std::make_unique<OpenSim::Model>(*params->getCommit().getModel());

		if (stopToken.stop_requested())
		{
			return std::make_unique<Plot>(*params);  // empty plot
		}

		osc::InitializeModel(*model);

		if (stopToken.stop_requested())
		{
			return std::make_unique<Plot>(*params);  // empty plot
		}

		SimTK::State& state = osc::InitializeState(*model);

		if (stopToken.stop_requested())
		{
			return std::make_unique<Plot>(*params);  // empty plot
		}

		// lookup relevant elements in the copies

		OpenSim::Muscle const* maybeMuscle = osc::FindComponent<OpenSim::Muscle>(*model, params->getMusclePath());
		if (!maybeMuscle)
		{
			return std::make_unique<Plot>(*params);  // empty plot
		}
		OpenSim::Muscle const& muscle = *maybeMuscle;

		OpenSim::Coordinate const* maybeCoord = osc::FindComponent<OpenSim::Coordinate>(*model, params->getCoordinatePath());
		if (!maybeCoord)
		{
			return std::make_unique<Plot>(*params);  // empty plot
		}
		OpenSim::Coordinate const& coord = *maybeCoord;

		int nPoints = params->getNumRequestedDataPoints();
		double start = coord.getRangeMin();
		double end = coord.getRangeMax();
		double step = (end - start) / std::max(1, nPoints-1);
		std::vector<float> xValues;
		xValues.reserve(nPoints);
		std::vector<float> yValues;
		yValues.reserve(nPoints);

		shared->setProgress(0.0f);
		coord.setLocked(state, false);

		// HACK: ensure `computeMomentArm` is called at least once before running
		//       the computation. No idea why. See:
		//
		// https://github.com/opensim-org/opensim-core/issues/3211
		{
			model->realizeDynamics(state);
			for (OpenSim::GeometryPath const& g : model->getComponentList<OpenSim::GeometryPath>())
			{
				g.computeMomentArm(state, coord);
			}
		}

		for (int i = 0; i < nPoints; ++i)
		{
			if (stopToken.stop_requested())
			{
				return std::make_unique<Plot>(*params);  // empty plot
			}

			double xVal = start + (i * step);

			coord.setValue(state, xVal);
			model->equilibrateMuscles(state);
			model->realizeReport(state);

			double yVald = params->getMuscleOutput()(state, muscle, coord);
			float yVal = static_cast<float>(yVald);

			xValues.push_back(osc::ConvertCoordValueToDisplayValue(coord, xVal));
			yValues.push_back(yVal);

			shared->setProgress(static_cast<float>(i+1) / static_cast<float>(nPoints));
		}

		return std::make_unique<Plot>(*params, std::move(xValues), std::move(yValues));
	}
}

// state stuff
namespace
{
	// data that is shared between all states
	struct SharedStateData final {
		explicit SharedStateData(std::shared_ptr<osc::UndoableModelStatePair> uim) : Uim{std::move(uim)}
		{
			OSC_ASSERT(Uim != nullptr);
		}

		SharedStateData(std::shared_ptr<osc::UndoableModelStatePair> uim,
			            OpenSim::ComponentPath const& coordPath,
			            OpenSim::ComponentPath const& musclePath) :
			Uim{std::move(uim)},
			PlotParams{Uim->getLatestCommit(), coordPath, musclePath, GetDefaultMuscleOutput(), 180}
		{
			OSC_ASSERT(Uim != nullptr);
		}

		std::shared_ptr<osc::UndoableModelStatePair> Uim;
		PlotParameters PlotParams{Uim->getLatestCommit(), OpenSim::ComponentPath{}, OpenSim::ComponentPath{}, GetDefaultMuscleOutput(), 180 };
	};

	// base class for a single widget state
	class MusclePlotState {
	protected:
		MusclePlotState(SharedStateData* shared_) : shared{std::move(shared_)}
		{
			OSC_ASSERT(shared != nullptr);
		}
	public:
		virtual ~MusclePlotState() noexcept = default;
		virtual std::unique_ptr<MusclePlotState> draw() = 0;

	protected:
		SharedStateData* shared;
	};

	std::unique_ptr<MusclePlotState> CreateChooseCoordinateState(SharedStateData*);
	std::unique_ptr<MusclePlotState> CreateChooseMuscleState(SharedStateData*);
	std::unique_ptr<MusclePlotState> CreateLoadDataState(SharedStateData*);
	std::unique_ptr<MusclePlotState> CreateShowPlotState(SharedStateData*, std::unique_ptr<Plot>);

	// state in which the plot is being shown to the user
	class ShowingPlotState final : public MusclePlotState {
	public:
		explicit ShowingPlotState(SharedStateData* shared_, std::unique_ptr<Plot> plot) :
			MusclePlotState{std::move(shared_)},
			m_Plot{std::move(plot)}
		{
		}

		std::unique_ptr<MusclePlotState> draw() override
		{
			// ensure latest requested params reflects the latest version of the model
			shared->PlotParams.setCommit(shared->Uim->getLatestCommit());

			// if the current plot doesn't match the latest requested params, replot
			if (m_Plot->getParameters() != shared->PlotParams)
			{
				return CreateLoadDataState(shared);
			}

			if (m_Plot->getXValues().empty())
			{
				ImGui::Text("(no X values)");
				return nullptr;
			}

			if (m_Plot->getYValues().empty())
			{
				ImGui::Text("(no Y values)");
				return nullptr;
			}

			auto modelGuard = m_Plot->getParameters().getCommit().getModel();

			OpenSim::Coordinate const* maybeCoord = osc::FindComponent<OpenSim::Coordinate>(*modelGuard, m_Plot->getParameters().getCoordinatePath());
			if (!maybeCoord)
			{
				ImGui::Text("(no coordinate named %s in model)", m_Plot->getParameters().getCoordinatePath().toString().c_str());
				return nullptr;
			}
			OpenSim::Coordinate const& coord = *maybeCoord;

			glm::vec2 availSize = ImGui::GetContentRegionAvail();

			std::string title = computePlotTitle(coord);
			std::string xAxisLabel = computePlotXAxisTitle(coord);
			std::string yAxisLabel = computePlotYAxisTitle();

			double currentX = osc::ConvertCoordValueToDisplayValue(coord, coord.getValue(shared->Uim->getState()));

			bool isHovered = false;
			ImPlotPoint p = {};
			if (ImPlot::BeginPlot(title.c_str(), availSize, ImPlotFlags_AntiAliased | ImPlotFlags_NoTitle | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoChild | ImPlotFlags_NoFrame))
			{
				ImPlotAxisFlags xAxisFlags = ImPlotAxisFlags_AutoFit;
				ImPlotAxisFlags yAxisFlags = ImPlotAxisFlags_AutoFit;
				ImPlot::SetupAxes(xAxisLabel.c_str(), yAxisLabel.c_str(), xAxisFlags, yAxisFlags);
				ImPlot::PlotLine(m_Plot->getParameters().getMusclePath().getComponentName().c_str(),
					m_Plot->getXValues().data(),
					m_Plot->getYValues().data(),
					static_cast<int>(m_Plot->getXValues().size()));
				ImPlot::TagX(currentX, { 1.0f, 1.0f, 1.0f, 1.0f });
				isHovered = ImPlot::IsPlotHovered();
				p = ImPlot::GetPlotMousePos();
				if (isHovered)
				{
					ImPlot::TagX(p.x, { 1.0f, 1.0f, 1.0f, 0.6f });
				}
				ImPlot::EndPlot();
			}
			if (isHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				double storedValue = osc::ConvertCoordDisplayValueToStorageValue(coord, static_cast<float>(p.x));
				osc::ActionSetCoordinateValue(*shared->Uim, coord, storedValue);
			}
			if (isHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				double storedValue = osc::ConvertCoordDisplayValueToStorageValue(coord, static_cast<float>(p.x));
				osc::ActionSetCoordinateValueAndSave(*shared->Uim, coord, storedValue);
			}
			if (ImGui::BeginPopupContextItem((title + "_contextmenu").c_str()))
			{
				drawPlotDataTypeSelector();
				if (ImGui::InputInt("num data points", &m_NumPlotPointsEdited, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
				{
					shared->PlotParams.setNumRequestedDataPoints(m_NumPlotPointsEdited);
				}

				ImGui::EndPopup();
			}

			return nullptr;
		}

	private:

		void drawPlotDataTypeSelector()
		{
			nonstd::span<MuscleOutput const> allOutputs = GetMuscleOutputs();

			std::vector<char const*> names;
			int active = -1;
			for (int i = 0; i < static_cast<int>(allOutputs.size()); ++i)
			{
				MuscleOutput const& o = allOutputs[i];
				names.push_back(o.getName());
				if (o == m_Plot->getParameters().getMuscleOutput())
				{
					active = i;
				}
			}

			if (ImGui::Combo("data type", &active, names.data(), static_cast<int>(names.size())))
			{
				shared->PlotParams.setMuscleOutput(allOutputs[active]);
			}
		}

		std::string computePlotTitle(OpenSim::Coordinate const& c)
		{
			std::stringstream ss;
			appendYAxisName(ss);
			ss << " vs ";
			appendXAxisName(c, ss);
			return std::move(ss).str();
		}

		void appendYAxisName(std::stringstream& ss)
		{
			ss << m_Plot->getParameters().getMuscleOutput().getName();
		}

		void appendXAxisName(OpenSim::Coordinate const& c, std::stringstream& ss)
		{
			ss << c.getName();
		}

		std::string computePlotYAxisTitle()
		{
			std::stringstream ss;
			appendYAxisName(ss);
			ss << " [" << m_Plot->getParameters().getMuscleOutput().getUnits() << ']';
			return std::move(ss).str();
		}

		std::string computePlotXAxisTitle(OpenSim::Coordinate const& c)
		{
			std::stringstream ss;
			appendXAxisName(c, ss);
			ss << " value [" << osc::GetCoordDisplayValueUnitsString(c) << ']';
			return std::move(ss).str();
		}

		std::unique_ptr<Plot> m_Plot;
		int m_NumPlotPointsEdited = shared->PlotParams.getNumRequestedDataPoints();
	};

	class LoadDataState final : public MusclePlotState {
	public:
		explicit LoadDataState(SharedStateData* shared_) :
			MusclePlotState{std::move(shared_)},
			m_LoadingResult{std::async(std::launch::async, ComputePlotSynchronously, m_StopSource.get_token(), std::make_unique<PlotParameters>(shared->PlotParams), m_SharedState)}
		{
		}

		std::unique_ptr<MusclePlotState> draw() override
		{
			ImGui::Text("computing plot");
			ImGui::ProgressBar(m_SharedState->getProgress());

			if (m_LoadingResult.wait_for(std::chrono::seconds{0}) == std::future_status::ready)
			{
				return CreateShowPlotState(shared, m_LoadingResult.get());
			}
			else
			{
				return nullptr;
			}
		}

	private:
		osc::stop_source m_StopSource;
		std::shared_ptr<ThreadsafeSharedState> m_SharedState = std::make_shared<ThreadsafeSharedState>();
		std::future<std::unique_ptr<Plot>> m_LoadingResult;
	};

	// state in which a user is being prompted to select a coordinate in the model
	class PickCoordinateState final : public MusclePlotState {
	public:
		explicit PickCoordinateState(SharedStateData* shared_) : MusclePlotState{std::move(shared_)}
		{
			// this is what this state is populating
			shared->PlotParams.setCoordinatePath(osc::GetEmptyComponentPath());
		}

		std::unique_ptr<MusclePlotState> draw() override
		{
			std::unique_ptr<MusclePlotState> rv;

			std::vector<OpenSim::Coordinate const*> coordinates;
			for (OpenSim::Coordinate const& coord : shared->Uim->getModel().getComponentList<OpenSim::Coordinate>())
			{
				coordinates.push_back(&coord);
			}
			osc::Sort(coordinates, SortByComponentName);

			ImGui::Text("select coordinate:");

			ImGui::BeginChild("MomentArmPlotCoordinateSelection");
			for (OpenSim::Coordinate const* coord : coordinates)
			{
				if (ImGui::Selectable(coord->getName().c_str()))
				{
					shared->PlotParams.setCoordinatePath(coord->getAbsolutePath());
					rv = std::make_unique<LoadDataState>(shared);
				}
			}
			ImGui::EndChild();

			return rv;
		}
	};

	// state in which a user is being prompted to select a muscle in the model
	class PickMuscleState final : public MusclePlotState {
	public:
		explicit PickMuscleState(SharedStateData* shared_) : MusclePlotState{std::move(shared_)}
		{
			// this is what this state is populating
			shared->PlotParams.setMusclePath(osc::GetEmptyComponentPath());
		}

		std::unique_ptr<MusclePlotState> draw() override
		{
			std::unique_ptr<MusclePlotState> rv;

			std::vector<OpenSim::Muscle const*> muscles;
			for (OpenSim::Muscle const& musc : shared->Uim->getModel().getComponentList<OpenSim::Muscle>())
			{
				muscles.push_back(&musc);
			}
			osc::Sort(muscles, SortByComponentName);

			ImGui::Text("select muscle:");

			ImGui::BeginChild("MomentArmPlotMuscleSelection");
			for (OpenSim::Muscle const* musc : muscles)
			{
				if (ImGui::Selectable(musc->getName().c_str()))
				{
					shared->PlotParams.setMusclePath(musc->getAbsolutePath());
					rv = std::make_unique<PickCoordinateState>(shared);
				}
			}
			ImGui::EndChild();

			return rv;
		}
	};

	std::unique_ptr<MusclePlotState> CreateChooseCoordinateState(SharedStateData* shared)
	{
		return std::make_unique<PickCoordinateState>(std::move(shared));
	}

	std::unique_ptr<MusclePlotState> CreateChooseMuscleState(SharedStateData* shared)
	{
		return std::make_unique<PickMuscleState>(std::move(shared));
	}

	std::unique_ptr<MusclePlotState> CreateLoadDataState(SharedStateData* shared)
	{
		return std::make_unique<LoadDataState>(std::move(shared));
	}
	std::unique_ptr<MusclePlotState> CreateShowPlotState(SharedStateData* shared, std::unique_ptr<Plot> plot)
	{
		return std::make_unique<ShowingPlotState>(std::move(shared), std::move(plot));
	}
}

// private IMPL for the muscle plot (effectively, a state machine host)
class osc::ModelMusclePlotPanel::Impl final {
public:
	Impl(std::shared_ptr<UndoableModelStatePair> uim, std::string_view panelName) :
		m_SharedData{std::move(uim)},
		m_ActiveState{std::make_unique<PickMuscleState>(&m_SharedData)},
		m_PanelName{std::move(panelName)}		
	{
	}

	Impl(std::shared_ptr<UndoableModelStatePair> uim,
         std::string_view panelName,
		 OpenSim::ComponentPath const& coordPath,
		 OpenSim::ComponentPath const& musclePath) :
		m_SharedData{std::move(uim), coordPath, musclePath},
		m_ActiveState{std::make_unique<LoadDataState>(&m_SharedData)},
		m_PanelName{std::move(panelName)}
	{
	}

	std::string const& getName() const
	{
		return m_PanelName;
	}

	bool isOpen() const
	{
		return m_IsOpen;
	}

	void open()
	{
		m_IsOpen = true;
	}

	void close()
	{
		m_IsOpen = false;
	}

	void draw()
	{
		if (m_IsOpen)
		{
			bool isOpen = m_IsOpen;
			if (ImGui::Begin(m_PanelName.c_str(), &isOpen))
			{
				if (auto maybeNextState = m_ActiveState->draw())
				{
					m_ActiveState = std::move(maybeNextState);
				}
				m_IsOpen = isOpen;
			}
			ImGui::End();

			if (isOpen != m_IsOpen)
			{
				m_IsOpen = isOpen;
			}
		}
	}

private:
	// data that's shared between all states
	SharedStateData m_SharedData;

	// currently active state (this class controls a state machine)
	std::unique_ptr<MusclePlotState> m_ActiveState;

	// name of the panel, as shown in the UI (via ImGui::Begin)
	std::string m_PanelName;

	// if the panel is currently open or not
	bool m_IsOpen = true;
};


// public API (PIMPL)

osc::ModelMusclePlotPanel::ModelMusclePlotPanel(std::shared_ptr<UndoableModelStatePair> uim,
	                                            std::string_view panelName) :
	m_Impl{new Impl{std::move(uim), std::move(panelName)}}
{
}

osc::ModelMusclePlotPanel::ModelMusclePlotPanel(std::shared_ptr<UndoableModelStatePair> uim,
                                                std::string_view panelName,
                                                OpenSim::ComponentPath const& coordPath,
                                                OpenSim::ComponentPath const& musclePath) :
	m_Impl{new Impl{std::move(uim), std::move(panelName), coordPath, musclePath}}
{
}

osc::ModelMusclePlotPanel::ModelMusclePlotPanel(ModelMusclePlotPanel&& tmp) noexcept :
	m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ModelMusclePlotPanel& osc::ModelMusclePlotPanel::operator=(ModelMusclePlotPanel&& tmp) noexcept
{
	std::swap(m_Impl, tmp.m_Impl);
	return *this;
}

osc::ModelMusclePlotPanel::~ModelMusclePlotPanel() noexcept
{
	delete m_Impl;
}

std::string const& osc::ModelMusclePlotPanel::getName() const
{
	return m_Impl->getName();
}

bool osc::ModelMusclePlotPanel::isOpen() const
{
	return m_Impl->isOpen();
}

void osc::ModelMusclePlotPanel::open()
{
	m_Impl->open();
}

void osc::ModelMusclePlotPanel::close()
{
	m_Impl->close();
}

void osc::ModelMusclePlotPanel::draw()
{
	m_Impl->draw();
}
