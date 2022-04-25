#include "ModelMusclePlotPanel.hpp"

#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableUiModel.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Algorithms.hpp"

#include <imgui.h>
#include <implot.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/Muscle.h>

#include <memory>
#include <string_view>
#include <utility>

static bool SortByComponentName(OpenSim::Component const* p1, OpenSim::Component const* p2)
{
	return p1->getName() < p2->getName();
}

class osc::ModelMusclePlotPanel::Impl final {
public:
	Impl(std::shared_ptr<UndoableUiModel> uim, std::string_view panelName) :
		m_Uim{std::move(uim)},
		m_PanelName{std::move(panelName)}
	{
		// TODO
	}

	void open()
	{
		// TODO
	}

	void close()
	{
		// TODO
	}

	void draw()
	{
		bool isOpen = m_IsOpen;
		if (ImGui::Begin(m_PanelName.c_str(), &isOpen))
		{
			drawPanelContent();
			m_IsOpen = isOpen;
		}
		else
		{
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
		if (m_Uim->getModelVersion() != m_LastPlotModelVersion)
		{
			recomputePlotData();
		}

		if (m_YValues.empty())
		{
			ImGui::Text("no values");
			return;
		}

		if (ImPlot::BeginPlot("Moment Arm vs Coordinate Value"))
		{
			ImPlot::SetupAxes(m_CoordinateComponentPath.getComponentName().c_str(), "Moment Arm");
			ImPlot::PlotLine(m_MuscleComponentPath.getComponentName().c_str(), m_XValues.data(), m_YValues.data(), static_cast<int>(m_XValues.size()));
			ImPlot::EndPlot();
		}
	}

	void recomputePlotData()
	{
		osc::log::info("recompute");

		OpenSim::Model const& model = m_Uim->getModel();

		OpenSim::Muscle const* muscle = FindComponent<OpenSim::Muscle>(model, m_MuscleComponentPath);

		if (!muscle)
		{
			return;
		}

		OpenSim::Coordinate const* coord = FindComponent<OpenSim::Coordinate>(model, m_CoordinateComponentPath);

		if (!coord)
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

			double yVald = muscle->getGeometryPath().computeMomentArm(stateCopy, *coord);
			float yVal = static_cast<float>(yVald);

			m_XValues.push_back(static_cast<float>(xVal));
			m_YValues.push_back(yVal);
			m_YMin = std::min(yVal, m_YMin);
			m_YMax = std::max(yVal, m_YMax);
		}

		m_LastPlotModelVersion = m_Uim->getModelVersion();
	}

	// overall panel state
	std::shared_ptr<UndoableUiModel> m_Uim;
	std::string m_PanelName;
	bool m_IsOpen = true;

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