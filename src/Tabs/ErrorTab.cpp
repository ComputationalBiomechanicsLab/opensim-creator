#include "ErrorTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Widgets/LogViewer.hpp"

#include <glm/vec2.hpp>
#include <imgui.h>
#include <SDL_events.h>
#include <IconsFontAwesome5.h>

#include <string>
#include <utility>

class osc::ErrorTab::Impl final {
public:
	Impl(TabHost* parent, std::exception const& ex) :
		m_Parent{std::move(parent)},
		m_ErrorMessage{ex.what()}
	{
	}

	UID getID() const
	{
        return m_ID;
	}

	CStringView getName() const
	{
		return m_Name;
	}

	TabHost* parent()
	{
		return m_Parent;
	}

	void onMount()
	{

	}

	void onUnmount()
	{

	}

	bool onEvent(SDL_Event const&)
	{
		return false;
	}

	void onTick()
	{

	}

	void drawMainMenu()
	{
	}

	void onDraw()
	{
		constexpr float width = 800.0f;
		constexpr float padding = 10.0f;

		Rect tabRect = osc::GetMainViewportWorkspaceScreenRect();
		glm::vec2 tabDims = osc::Dimensions(tabRect);

		// error message panel
		{
			glm::vec2 pos{tabRect.p1.x + tabDims.x/2.0f, tabRect.p1.y + padding};
			ImGui::SetNextWindowPos(pos, ImGuiCond_Once, {0.5f, 0.0f});
			ImGui::SetNextWindowSize({width, 0.0f});

			if (ImGui::Begin("fatal error"))
			{
				ImGui::TextWrapped("The application threw an exception with the following message:");
				ImGui::Dummy({2.0f, 10.0f});
				ImGui::SameLine();
				ImGui::TextWrapped("%s", m_ErrorMessage.c_str());
				ImGui::Dummy({0.0f, 10.0f});
			}
			ImGui::End();
		}

		// log message panel
		{
			glm::vec2 pos{tabRect.p1.x + tabDims.x/2.0f, tabRect.p2.y - padding};
			ImGui::SetNextWindowPos(pos, ImGuiCond_Once, ImVec2(0.5f, 1.0f));
			ImGui::SetNextWindowSize(ImVec2(width, 0.0f));

			if (ImGui::Begin("Error Log", nullptr, ImGuiWindowFlags_MenuBar))
			{
				m_LogViewer.draw();
			}
			ImGui::End();
		}
	}

private:
	UID m_ID;
	TabHost* m_Parent;
	std::string m_Name = ICON_FA_SPIDER " Error";
	std::string m_ErrorMessage;
	osc::LogViewer m_LogViewer;
};


// public API

osc::ErrorTab::ErrorTab(TabHost* parent, std::exception const& ex) :
	m_Impl{new Impl{std::move(parent), ex}}
{
}

osc::ErrorTab::ErrorTab(ErrorTab&& tmp) noexcept :
	m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ErrorTab& osc::ErrorTab::operator=(ErrorTab&& tmp) noexcept
{
	std::swap(m_Impl, tmp.m_Impl);
	return *this;
}

osc::ErrorTab::~ErrorTab() noexcept
{
	delete m_Impl;
}

osc::UID osc::ErrorTab::implGetID() const
{
	return m_Impl->getID();
}

osc::CStringView osc::ErrorTab::implGetName() const
{
	return m_Impl->getName();
}

osc::TabHost* osc::ErrorTab::implParent() const
{
	return m_Impl->parent();
}

void osc::ErrorTab::implOnMount()
{
	m_Impl->onMount();
}

void osc::ErrorTab::implOnUnmount()
{
	m_Impl->onUnmount();
}

bool osc::ErrorTab::implOnEvent(SDL_Event const& e)
{
	return m_Impl->onEvent(e);
}

void osc::ErrorTab::implOnTick()
{
	m_Impl->onTick();
}

void osc::ErrorTab::implOnDrawMainMenu()
{
	m_Impl->drawMainMenu();
}

void osc::ErrorTab::implOnDraw()
{
	m_Impl->onDraw();
}
