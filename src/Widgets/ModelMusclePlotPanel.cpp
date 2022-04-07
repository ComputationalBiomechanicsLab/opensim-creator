#include "ModelMusclePlotPanel.hpp"

#include "src/OpenSimBindings/UndoableUiModel.hpp"
#include "src/Utils/ScopeGuard.hpp"

#include <imgui.h>

#include <memory>
#include <string_view>
#include <utility>

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
		bool open = ImGui::Begin(m_PanelName.c_str(), &m_IsOpen);
		OSC_SCOPE_GUARD({ ImGui::End(); });

		if (!open)
		{
			return;
		}

		ImGui::Text("hello, world");
	}

private:
	std::shared_ptr<UndoableUiModel> m_Uim;
	std::string m_PanelName;
	bool m_IsOpen = true;
};

// public API

osc::ModelMusclePlotPanel::ModelMusclePlotPanel(std::shared_ptr<UndoableUiModel> uim,
	                                            std::string_view panelName) :
	m_Impl{new Impl{std::move(uim), std::move(panelName)}}
{
}

osc::ModelMusclePlotPanel::ModelMusclePlotPanel(ModelMusclePlotPanel&&) noexcept = default;
osc::ModelMusclePlotPanel& osc::ModelMusclePlotPanel::operator=(ModelMusclePlotPanel&&) noexcept = default;
osc::ModelMusclePlotPanel::~ModelMusclePlotPanel() noexcept = default;

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