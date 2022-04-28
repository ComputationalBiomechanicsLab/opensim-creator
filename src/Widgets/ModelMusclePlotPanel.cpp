#include "ModelMusclePlotPanel.hpp"

#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableUiModel.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/CStringView.hpp"

#include <imgui.h>
#include <implot.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>

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

		char const* getUnits()
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

static double GetFiberLength(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return muscle.getFiberLength(st);
}

static double GetTendonLength(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return muscle.getTendonLength(st);
}

static double GetPennationAngle(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return glm::degrees(muscle.getPennationAngle(st));
}

static double GetNormalizedFiberLength(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return muscle.getNormalizedFiberLength(st);
}

static double GetTendonStrain(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return muscle.getTendonStrain(st);
}

static double GetFiberPotentialEnergy(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return muscle.getFiberPotentialEnergy(st);
}

static double GetTendonPotentialEnergy(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return muscle.getTendonPotentialEnergy(st);
}

static double GetMusclePotentialEnergy(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return muscle.getMusclePotentialEnergy(st);
}

static double GetTendonForce(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return muscle.getTendonForce(st);
}

static double GetActiveFiberForce(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return muscle.getActiveFiberForce(st);
}

static double GetPassiveFiberForce(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return muscle.getPassiveFiberForce(st);
}

static double GetTotalFiberForce(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return muscle.getFiberForce(st);
}

static double GetFiberStiffness(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return muscle.getFiberStiffness(st);
}

static double GetFiberStiffnessAlongTendon(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return muscle.getFiberStiffnessAlongTendon(st);
}

static double GetTendonStiffness(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return muscle.getTendonStiffness(st);
}

static double GetMuscleStiffness(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return muscle.getMuscleStiffness(st);
}

static double GetFiberActivePower(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return muscle.getFiberActivePower(st);
}

static double GetFiberPassivePower(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return muscle.getFiberActivePower(st);
}

static double GetTendonPower(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
{
	return muscle.getTendonPower(st);
}

static double GetMusclePower(SimTK::State const& st, OpenSim::Muscle const& muscle, OpenSim::Coordinate const& c)
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

class osc::ModelMusclePlotPanel::Impl final {
public:
	Impl(std::shared_ptr<UndoableUiModel> uim, std::string_view panelName) :
		m_Uim{std::move(uim)},
		m_PanelName{std::move(panelName)}
	{
	}

	void open()
	{
	}

	void close()
	{
	}

	void draw()
	{
		bool isOpen = m_IsOpen;

		int stylesPushed = 0;
		if (m_IsShowingPlot)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
			++stylesPushed;
		}

		if (ImGui::Begin(m_PanelName.c_str(), &isOpen))
		{
			ImGui::PopStyleVar(stylesPushed);
			drawPanelContent();
			m_IsOpen = isOpen;
		}
		else
		{
			ImGui::PopStyleVar(stylesPushed);
			m_IsOpen = isOpen;
		}
		ImGui::End();
	}

private:
	void drawPanelContent()
	{
		if (m_IsChoosingMuscle)
		{
			drawPickMuscleState();
		}
		else if (m_IsChoosingCoordinate)
		{
			drawPickCoordinateState();
		}
		else if (m_IsShowingPlot)
		{
			drawShowPlotState();
		}
	}

	void drawPickMuscleState()
	{
		std::vector<OpenSim::Muscle const*> muscles;
		for (OpenSim::Muscle const& musc : m_Uim->getModel().getComponentList<OpenSim::Muscle>())
		{
			muscles.push_back(&musc);
		}
		Sort(muscles, SortByComponentName);

		ImGui::Text("select muscle:");

		ImGui::BeginChild("MomentArmPlotMuscleSelection");
		for (OpenSim::Muscle const* musc : muscles)
		{
			if (ImGui::Selectable(musc->getName().c_str()))
			{
				m_MuscleComponentPath = musc->getAbsolutePath();
				m_IsChoosingMuscle = false;
				m_IsChoosingCoordinate = true;
				m_IsShowingPlot = false;
			}
		}
		ImGui::EndChild();
	}

	void drawPickCoordinateState()
	{
		std::vector<OpenSim::Coordinate const*> coordinates;
		for (OpenSim::Coordinate const& coord : m_Uim->getModel().getComponentList<OpenSim::Coordinate>())
		{
			coordinates.push_back(&coord);
		}
		Sort(coordinates, SortByComponentName);

		ImGui::Text("select coordinate:");

		ImGui::BeginChild("MomentArmPlotCoordinateSelection");
		for (OpenSim::Coordinate const* coord : coordinates)
		{
			if (ImGui::Selectable(coord->getName().c_str()))
			{
				m_IsChoosingCoordinate = false;
				m_CoordinateComponentPath = coord->getAbsolutePath();
				m_IsShowingPlot = true;
			}
		}
		ImGui::EndChild();
	}

	void drawShowPlotState()
	{
		OpenSim::Model const& model = m_Uim->getModel();
		OpenSim::Coordinate const* coord = FindComponent<OpenSim::Coordinate>(model, m_CoordinateComponentPath);

		if (!coord)
		{
			return;
		}

		drawPlotDataTypeSelector();

		if (m_Uim->getModelVersion() != m_LastPlotModelVersion || m_ChosenMuscleOutput != m_ActiveMuscleOutput)
		{
			recomputePlotData(coord);
		}

		if (m_YValues.empty())
		{
			ImGui::Text("(no Y values)");
			return;
		}

		glm::vec2 availSize = ImGui::GetContentRegionAvail();

		std::string title = computePlotTitle(*coord);
		std::string xAxisLabel = computePlotXAxisTitle(*coord);
		std::string yAxisLabel = computePlotYAxisTitle();

		if (ImPlot::BeginPlot(title.c_str(), availSize))
		{
			ImPlot::SetupAxes(xAxisLabel.c_str(), yAxisLabel.c_str());
			ImPlot::PlotLine(m_MuscleComponentPath.getComponentName().c_str(), m_XValues.data(), m_YValues.data(), static_cast<int>(m_XValues.size()));
			ImPlot::EndPlot();
		}
	}

	void drawPlotDataTypeSelector()
	{
		nonstd::span<MuscleOutput const> allOutputs = GetMuscleOutputs();

		std::vector<char const*> names;
		int active = -1;
		for (int i = 0; i < static_cast<int>(allOutputs.size()); ++i)
		{
			MuscleOutput const& o = allOutputs[i];
			names.push_back(o.getName());
			if (o == m_ActiveMuscleOutput)
			{
				active = i;
			}
		}

		if (ImGui::Combo("data type", &active, names.data(), static_cast<int>(names.size())))
		{
			m_ChosenMuscleOutput = allOutputs[active];
		}
	}

	void recomputePlotData(OpenSim::Coordinate const* coord)
	{
		OpenSim::Model const& model = m_Uim->getModel();

		OpenSim::Muscle const* muscle = FindComponent<OpenSim::Muscle>(model, m_MuscleComponentPath);

		if (!muscle)
		{
			return;
		}

		// input state
		SimTK::State stateCopy = m_Uim->getState();
		model.realizeReport(stateCopy);

		int numSteps = 50;

		if (numSteps <= 0)
		{
			return;
		}

		bool prev_locked = coord->getLocked(stateCopy);
		double prev_val = coord->getValue(stateCopy);

		coord->setLocked(stateCopy, false);

		double start = coord->getRangeMin();
		double end = coord->getRangeMax();
		double step = (end - start) / numSteps;

		m_XBegin = static_cast<float>(start);
		m_XEnd = static_cast<float>(end);
		m_XValues.clear();
		m_XValues.reserve(numSteps);
		m_YValues.clear();
		m_YValues.reserve(numSteps);
		m_YMin = std::numeric_limits<float>::max();
		m_YMax = std::numeric_limits<float>::min();

		for (int i = 0; i < numSteps; ++i)
		{
			double xVal = start + (i * step);
			coord->setValue(stateCopy, xVal);
			model.realizeReport(stateCopy);

			double yVald = m_ChosenMuscleOutput(stateCopy, *muscle, *coord);
			float yVal = static_cast<float>(yVald);

			m_XValues.push_back(ConvertCoordValueToDisplayValue(*coord, xVal));
			m_YValues.push_back(yVal);
			m_YMin = std::min(yVal, m_YMin);
			m_YMax = std::max(yVal, m_YMax);
		}

		m_LastPlotModelVersion = m_Uim->getModelVersion();
		m_ActiveMuscleOutput = m_ChosenMuscleOutput;
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
		ss << m_ActiveMuscleOutput.getName();
	}

	void appendXAxisName(OpenSim::Coordinate const& c, std::stringstream& ss)
	{
		ss << c.getName();
	}

	std::string computePlotYAxisTitle()
	{
		std::stringstream ss;
		appendYAxisName(ss);
		ss << " [" << m_ActiveMuscleOutput.getUnits() << ']';
		return std::move(ss).str();
	}

	std::string computePlotXAxisTitle(OpenSim::Coordinate const& c)
	{
		std::stringstream ss;
		appendXAxisName(c, ss);
		ss << " value [" << GetCoordDisplayValueUnitsString(c) << ']';
		return std::move(ss).str();
	}

	// overall panel state
	std::shared_ptr<UndoableUiModel> m_Uim;
	std::string m_PanelName;
	bool m_IsOpen = true;

	// data type
	MuscleOutput m_ChosenMuscleOutput = GetDefaultMuscleOutput();
	MuscleOutput m_ActiveMuscleOutput = m_ChosenMuscleOutput;

	// muscle picking state
	bool m_IsChoosingMuscle = true;
	OpenSim::ComponentPath m_MuscleComponentPath;

	// coordinate picking state
	bool m_IsChoosingCoordinate = false;
	OpenSim::ComponentPath m_CoordinateComponentPath;

	// plot state
	bool m_IsShowingPlot = false;
	UID m_LastPlotModelVersion;
	float m_XBegin = -1.0f;
	float m_XEnd = -1.0f;
	std::vector<float> m_XValues;
	std::vector<float> m_YValues;
	float m_YMin = -1.0f;
	float m_YMax = -1.0f;
};


// public API

osc::ModelMusclePlotPanel::ModelMusclePlotPanel(std::shared_ptr<UndoableUiModel> uim,
	                                            std::string_view panelName) :
	m_Impl{new Impl{std::move(uim), std::move(panelName)}}
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