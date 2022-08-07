#include "MeshGenTestTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Graphics/SceneDecorationFlags.hpp"
#include "src/Graphics/SceneRenderer.hpp"
#include "src/Graphics/SceneRendererParams.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/Platform/App.hpp"
#include "src/Widgets/SceneViewer.hpp"

#include <glm/gtx/transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <SDL_events.h>

#include <map>
#include <memory>
#include <string>
#include <utility>

static std::map<std::string, std::shared_ptr<osc::Mesh>> GenerateMeshLookup()
{
	return
	{
		{"sphere", osc::App::meshes().getSphereMesh()},
		{"cylinder", osc::App::meshes().getCylinderMesh()},
		{"brick", osc::App::meshes().getBrickMesh()},
		{"cone", osc::App::meshes().getConeMesh()},
		{"floor", osc::App::meshes().getFloorMesh()},
		{"100x100 grid", osc::App::meshes().get100x100GridMesh()},
		{"cube (wire)", osc::App::meshes().getCubeWireMesh()},
		{"yline", osc::App::meshes().getYLineMesh()},
		{"quad", osc::App::meshes().getTexturedQuadMesh()},
	};
}

class osc::MeshGenTestTab::Impl final {
public:
	Impl(TabHost* parent) : m_Parent{std::move(parent)}
	{
		m_Camera.radius = 5.0f;
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

	void onDrawMainMenu()
	{

	}

	void onDraw()
	{
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

		if (m_Viewer.isHovered())
		{
			osc::UpdatePolarCameraFromImGuiUserInput(osc::App::get().dims(), m_Camera);
		}

		if (ImGui::Begin("viewer"))
		{
			glm::vec2 pos = ImGui::GetCursorPos();
			for (auto const& [name, _] : m_AllMeshes)
			{
				if (ImGui::Button(name.c_str()))
				{
					m_CurrentMesh = name;
				}
				ImGui::SameLine();
			}
			ImGui::NewLine();

			glm::vec2 contentRegion = ImGui::GetContentRegionAvail();
			m_RenderParams.dimensions = contentRegion;
			m_RenderParams.samples = osc::App::get().getMSXAASamplesRecommended();

			{
				glm::vec3 p = glm::normalize(-m_Camera.focusPoint - m_Camera.getPos());
				glm::vec3 up = {0.0f, 1.0f, 0.0f};
				glm::vec3 mp = glm::rotate(glm::mat4{1.0f}, 1.25f * fpi4, up) * glm::vec4{p, 0.0f};

				m_RenderParams.lightDirection = glm::normalize(mp + -up);
				m_RenderParams.projectionMatrix = m_Camera.getProjMtx(AspectRatio(m_RenderParams.dimensions));
				m_RenderParams.viewMatrix = m_Camera.getViewMtx();
				m_RenderParams.viewPos = m_Camera.getPos();
				m_RenderParams.drawFloor = false;

				SceneDecoration d
				{
					m_AllMeshes[m_CurrentMesh],
					Transform{},
					{1.0f, 1.0f, 1.0f, 1.0f},
					"NO_ID",
					SceneDecorationFlags{}
				};

				m_Viewer.draw(nonstd::span<SceneDecoration const>{&d, 1}, m_RenderParams);
			}
		}
		ImGui::End();
	}


private:
	UID m_ID;
	std::string m_Name = ICON_FA_HAT_WIZARD " MeshGenTest";
	TabHost* m_Parent;

	std::string m_CurrentMesh = "brick";
	std::map<std::string, std::shared_ptr<osc::Mesh>> m_AllMeshes = GenerateMeshLookup();
	SceneViewer m_Viewer;
	SceneRendererParams m_RenderParams;
	PolarPerspectiveCamera m_Camera;
};


// public API

osc::MeshGenTestTab::MeshGenTestTab(TabHost* parent) :
	m_Impl{new Impl{std::move(parent)}}
{
}

osc::MeshGenTestTab::MeshGenTestTab(MeshGenTestTab&& tmp) noexcept :
	m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::MeshGenTestTab& osc::MeshGenTestTab::operator=(MeshGenTestTab&& tmp) noexcept
{
	std::swap(m_Impl, tmp.m_Impl);
	return *this;
}

osc::MeshGenTestTab::~MeshGenTestTab() noexcept
{
	delete m_Impl;
}

osc::UID osc::MeshGenTestTab::implGetID() const
{
	return m_Impl->getID();
}

osc::CStringView osc::MeshGenTestTab::implGetName() const
{
	return m_Impl->getName();
}

osc::TabHost* osc::MeshGenTestTab::implParent() const
{
	return m_Impl->parent();
}

void osc::MeshGenTestTab::implOnMount()
{
	m_Impl->onMount();
}

void osc::MeshGenTestTab::implOnUnmount()
{
	m_Impl->onUnmount();
}

bool osc::MeshGenTestTab::implOnEvent(SDL_Event const& e)
{
	return m_Impl->onEvent(e);
}

void osc::MeshGenTestTab::implOnTick()
{
	m_Impl->onTick();
}

void osc::MeshGenTestTab::implOnDrawMainMenu()
{
	m_Impl->onDrawMainMenu();
}

void osc::MeshGenTestTab::implOnDraw()
{
	m_Impl->onDraw();
}
