#include "HittestTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Renderer.hpp"
#include "src/Maths/Disc.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Line.hpp"
#include "src/Maths/Sphere.hpp"
#include "src/Platform/App.hpp"

#include <glm/vec3.hpp>
#include <SDL_events.h>
#include <IconsFontAwesome5.h>

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace
{
	std::array<glm::vec3, 4> const g_CrosshairVerts =
	{{
		// -X to +X
		{-0.05f, 0.0f, 0.0f},
		{+0.05f, 0.0f, 0.0f},

		// -Y to +Y
		{0.0f, -0.05f, 0.0f},
		{0.0f, +0.05f, 0.0f},
	}};

	std::array<uint16_t, 4> const g_CrosshairIndices = {0, 1, 2, 3};

	std::array<glm::vec3, 3> const g_TriangleVerts =
	{{
		{-10.0f, -10.0f, 0.0f},
		{+0.0f, +10.0f, 0.0f},
		{+10.0f, -10.0f, 0.0f},
	}};

	std::array<uint16_t, 3> const g_TriangleIndices = {0, 1, 2};

	struct SceneSphere final {
		glm::vec3 pos;
		bool isHovered = false;

		SceneSphere(glm::vec3 pos_) : pos{pos_} {}
	};

	std::vector<SceneSphere> GenerateSceneSpheres()
	{
		constexpr int min = -30;
		constexpr int max = 30;
		constexpr int step = 6;

		std::vector<SceneSphere> rv;
		for (int x = min; x <= max; x += step)
		{
			for (int y = min; y <= max; y += step)
			{
				for (int z = min; z <= max; z += step)
				{
					rv.emplace_back(glm::vec3{x, 50.0f + 2.0f*y, z});
				}
			}
		}
		return rv;
	}

	osc::experimental::Mesh GenerateCrosshairMesh()
	{
		osc::experimental::Mesh rv;
		rv.setTopography(osc::experimental::MeshTopography::Lines);
		rv.setVerts(g_CrosshairVerts);
		rv.setIndices(g_CrosshairIndices);
		return rv;
	}

	osc::experimental::Mesh GenerateTriangleMesh()
	{
		osc::experimental::Mesh rv;
		rv.setVerts(g_TriangleVerts);
		rv.setIndices(g_TriangleIndices);
		return rv;
	}

	osc::MaterialPropertyBlock GeneratePropertyBlock(glm::vec4 const& v)
	{
		osc::MaterialPropertyBlock p;
		p.setVec4("uColor", v);
		return p;
	}

	osc::Line GetCameraRay(osc::experimental::Camera const& camera)
	{
		return
		{
			camera.getPosition(),
			camera.getDirection(),
		};
	}
}

class osc::HittestTab::Impl final {
public:
	Impl(TabHost* parent) : m_Parent{std::move(parent)}
	{
		m_Camera.setBackgroundColor({1.0f, 1.0f, 1.0f, 0.0f});
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
		App::upd().makeMainEventLoopPolling();
		m_IsMouseCaptured = true;
	}

	void onUnmount()
	{
		m_IsMouseCaptured = false;
		App::upd().makeMainEventLoopWaiting();
		App::upd().setShowCursor(true);
	}

	bool onEvent(SDL_Event const& e)
	{
		if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
		{
			m_IsMouseCaptured = false;
			return true;
		}
		else if (e.type == SDL_MOUSEBUTTONDOWN && osc::IsMouseInMainViewportWorkspaceScreenRect())
		{
			m_IsMouseCaptured = true;
			return true;
		}
		return false;
	}

	void onTick()
	{
		// hittest spheres

		Line ray = GetCameraRay(m_Camera);
		float closestEl = std::numeric_limits<float>::max();
		SceneSphere* closestSceneSphere = nullptr;

		for (SceneSphere& ss : m_SceneSpheres)
		{
			ss.isHovered = false;

			Sphere s
			{
				ss.pos,
				m_SceneSphereBoundingSphere.radius
			};

			RayCollision res = GetRayCollisionSphere(ray, s);
			if (res.hit && res.distance >= 0.0f && res.distance < closestEl)
			{
				closestEl = res.distance;
				closestSceneSphere = &ss;
			}
		}

		if (closestSceneSphere)
		{
			closestSceneSphere->isHovered = true;
		}
	}

	void onDrawMainMenu()
	{
	}

	void onDraw()
	{
		// handle mouse capturing
		if (m_IsMouseCaptured)
		{
			UpdateEulerCameraFromImGuiUserInput(m_Camera, m_CameraEulers);
			ImGui::SetMouseCursor(ImGuiMouseCursor_None);
			App::upd().setShowCursor(false);
		}
		else
		{
			ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
			App::upd().setShowCursor(true);
		}

		// set render dimensions
		m_Camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());

		// render spheres
		for (SceneSphere const& sphere : m_SceneSpheres)
		{
			Transform t;
			t.position = sphere.pos;

			experimental::Graphics::DrawMesh(
				m_SphereMesh,
				t,
				m_Material,
				m_Camera,
				sphere.isHovered ? m_BlueColorMaterialProps : m_RedColorMaterialProps
			);

			// draw sphere AABBs
			if (m_IsShowingAABBs)
			{
				Transform aabbTransform;
				aabbTransform.position = sphere.pos;
				aabbTransform.scale = 0.5f * Dimensions(m_SceneSphereAABB);

				experimental::Graphics::DrawMesh(
					m_WireframeCubeMesh,
					aabbTransform,
					m_Material,
					m_Camera,
					m_BlackColorMaterialProps
				);
			}
		}

		// hittest + draw disc
		{
			Line ray = GetCameraRay(m_Camera);

			Disc sceneDisc;
			sceneDisc.origin = {0.0f, 0.0f, 0.0f};
			sceneDisc.normal = {0.0f, 1.0f, 0.0f};
			sceneDisc.radius = {10.0f};

			RayCollision collision = GetRayCollisionDisc(ray, sceneDisc);

			Disc meshDisc;
			meshDisc.origin = {0.0f, 0.0f, 0.0f};
			meshDisc.normal = {0.0f, 0.0f, 1.0f};
			meshDisc.radius = 1.0f;

			experimental::Graphics::DrawMesh(
				m_CircleMesh,
				DiscToDiscMat4(meshDisc, sceneDisc),
				m_Material,
				m_Camera,
				collision ? m_BlueColorMaterialProps : m_RedColorMaterialProps
			);
		}

		// hittest + draw triangle
		{
			Line ray = GetCameraRay(m_Camera);
			RayCollision collision = GetRayCollisionTriangle(ray, g_TriangleVerts.data());

			experimental::Graphics::DrawMesh(
				m_TriangleMesh,
				Transform{},
				m_Material,
				m_Camera,
				collision ? m_BlueColorMaterialProps : m_RedColorMaterialProps
			);
		}

		// draw crosshair overlay
		experimental::Graphics::DrawMesh(
			m_CrosshairMesh,
			m_Camera.getInverseViewProjectionMatrix(),
			m_Material,
			m_Camera,
			m_BlackColorMaterialProps
		);

		// draw scene to screen
		m_Camera.render();
	}


private:
	// tab state
	UID m_ID;
	std::string m_Name = ICON_FA_COOKIE " HittestTab";
	TabHost* m_Parent;

	// rendering
	experimental::Camera m_Camera;
	Material m_Material
	{
		Shader
		{
			App::slurp("shaders/SolidColor.vert"),
			App::slurp("shaders/SolidColor.frag"),
		}
	};
	experimental::Mesh m_SphereMesh = GenUntexturedUVSphere(12, 12);
	experimental::Mesh m_WireframeCubeMesh = GenCubeLines();
	experimental::Mesh m_CircleMesh = GenCircle(36);
	experimental::Mesh m_CrosshairMesh = GenerateCrosshairMesh();
	experimental::Mesh m_TriangleMesh = GenerateTriangleMesh();
	MaterialPropertyBlock m_BlackColorMaterialProps = GeneratePropertyBlock({0.0f, 0.0f, 0.0f, 1.0f});
	MaterialPropertyBlock m_BlueColorMaterialProps = GeneratePropertyBlock({0.0f, 0.0f, 1.0f, 1.0f});
	MaterialPropertyBlock m_RedColorMaterialProps = GeneratePropertyBlock({1.0f, 0.0f, 0.0f, 1.0f});

	// scene state
	std::vector<SceneSphere> m_SceneSpheres = GenerateSceneSpheres();
	AABB m_SceneSphereAABB = AABBFromVerts(m_SphereMesh.getVerts().data(), m_SphereMesh.getVerts().size());
	Sphere m_SceneSphereBoundingSphere = BoundingSphereOf(m_SphereMesh.getVerts().data(), m_SphereMesh.getVerts().size());
	bool m_IsMouseCaptured = false;
	glm::vec3 m_CameraEulers = {0.0f, 0.0f, 0.0f};
	bool m_IsShowingAABBs = true;
};


// public API

osc::HittestTab::HittestTab(TabHost* parent) :
	m_Impl{new Impl{std::move(parent)}}
{
}

osc::HittestTab::HittestTab(HittestTab&& tmp) noexcept :
	m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::HittestTab& osc::HittestTab::operator=(HittestTab&& tmp) noexcept
{
	std::swap(m_Impl, tmp.m_Impl);
	return *this;
}

osc::HittestTab::~HittestTab() noexcept
{
	delete m_Impl;
}

osc::UID osc::HittestTab::implGetID() const
{
	return m_Impl->getID();
}

osc::CStringView osc::HittestTab::implGetName() const
{
	return m_Impl->getName();
}

osc::TabHost* osc::HittestTab::implParent() const
{
	return m_Impl->parent();
}

void osc::HittestTab::implOnMount()
{
	m_Impl->onMount();
}

void osc::HittestTab::implOnUnmount()
{
	m_Impl->onUnmount();
}

bool osc::HittestTab::implOnEvent(SDL_Event const& e)
{
	return m_Impl->onEvent(e);
}

void osc::HittestTab::implOnTick()
{
	m_Impl->onTick();
}

void osc::HittestTab::implOnDrawMainMenu()
{
	m_Impl->onDrawMainMenu();
}

void osc::HittestTab::implOnDraw()
{
	m_Impl->onDraw();
}
