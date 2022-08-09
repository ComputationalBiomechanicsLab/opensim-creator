#include "MeshHittestTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Bindings/SimTKHelpers.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Renderer.hpp"
#include "src/Maths/BVH.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/Maths/Line.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"
#include "src/Utils/UID.hpp"
#include "src/Widgets/PerfPanel.hpp"

#include <glm/vec3.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <SDL_events.h>

#include <chrono>
#include <string>
#include <utility>

namespace
{
	void DrawBVHRecursive(osc::BVH const& bvh, osc::experimental::Mesh const& cubeLinesMesh, osc::experimental::Material const& material, osc::experimental::Camera& camera, int pos)
	{
		osc::BVHNode const& n = bvh.nodes[pos];
		glm::vec3 const halfWidths = 0.5f * osc::Dimensions(n.bounds);
		glm::vec3 const midpoint = osc::Midpoint(n.bounds);

		osc::Transform t;
		t.scale *= halfWidths;
		t.position = midpoint;

		osc::experimental::Graphics::DrawMesh(cubeLinesMesh, t, material, camera);

		if (n.nlhs >= 0)
		{
			DrawBVHRecursive(bvh, cubeLinesMesh, material, camera, pos + 1);
			DrawBVHRecursive(bvh, cubeLinesMesh, material, camera, pos + n.nlhs + 1);
		}
	}

	void DrawBVH(osc::BVH const& bvh, osc::experimental::Mesh const& cubeLinesMesh, osc::experimental::Material const& material, osc::experimental::Camera& camera)
	{
		if (bvh.nodes.empty())
		{
			return;
		}
		DrawBVHRecursive(bvh, cubeLinesMesh, material, camera, 0);
	}
}

class osc::MeshHittestTab::Impl final {
public:
	Impl(TabHost* parent) : m_Parent{std::move(parent)}
	{
		m_Camera.setBackgroundColor({1.0f, 1.0f, 1.0f, 1.0f});
		BVH_BuildFromIndexedTriangles(m_BVH, m_Mesh.getVerts(), m_Mesh.getIndices());
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
		App const& app = App::get();
		UpdatePolarCameraFromImGuiUserInput(app.dims(), m_PolarCamera);

		// handle hittest
		auto raycastStart = std::chrono::high_resolution_clock::now();
		{
			Rect r = osc::GetMainViewportWorkspaceScreenRect();
			glm::vec2 d = osc::Dimensions(r);
			m_Ray = m_PolarCamera.unprojectTopLeftPosToWorldRay(glm::vec2{ ImGui::GetIO().MousePos } - r.p1, d);

			m_IsMousedOver = false;
			if (m_UseBVH)
			{
				BVHCollision collision;
				if (BVH_GetClosestRayIndexedTriangleCollision(m_BVH, m_Mesh.getVerts(), m_Mesh.getIndices(), m_Ray, &collision))
				{
					uint16_t index = m_Mesh.getIndices()[collision.primId];
					m_IsMousedOver = true;
					m_Tris[0] = m_Mesh.getVerts()[index];
					m_Tris[1] = m_Mesh.getVerts()[index+1];
					m_Tris[2] = m_Mesh.getVerts()[index+2];
				}
			}
			else
			{
				nonstd::span<glm::vec3 const> tris = m_Mesh.getVerts();
				for (size_t i = 0; i < tris.size(); i += 3)
				{
					RayCollision res = GetRayCollisionTriangle(m_Ray, tris.data() + i);
					if (res.hit)
					{
						m_HitPos = m_Ray.origin + res.distance * m_Ray.dir;
						m_IsMousedOver = true;

						m_Tris[0] = tris[i];
						m_Tris[1] = tris[i + 1];
						m_Tris[2] = tris[i + 2];

						break;
					}
				}
			}
		}
		auto raycastEnd = std::chrono::high_resolution_clock::now();
		auto raycastDt = raycastEnd - raycastStart;
		m_RaycastDuration = std::chrono::duration_cast<std::chrono::microseconds>(raycastDt);
	}

	void onDrawMainMenu()
	{
	}

	void onDraw()
	{
		// setup scene
		{
			Rect const viewportRect = osc::GetMainViewportWorkspaceScreenRect();
			glm::vec2 const viewportRectDims = osc::Dimensions(viewportRect);
			m_Camera.setPixelRect(viewportRect);

			// update real scene camera from constrained polar camera
			m_Camera.setPosition(m_PolarCamera.getPos());
			m_Camera.setNearClippingPlane(m_PolarCamera.znear);
			m_Camera.setFarClippingPlane(m_PolarCamera.zfar);
			m_Camera.setViewMatrix(m_PolarCamera.getViewMtx());
			m_Camera.setProjectionMatrix(m_PolarCamera.getProjMtx(AspectRatio(viewportRectDims)));
		}

		// draw mesh
		m_Material.setVec4("uColor", m_IsMousedOver ? glm::vec4{0.0f, 1.0f, 0.0f, 1.0f} : glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
		m_Material.setDepthTested(true);
		experimental::Graphics::DrawMesh(m_Mesh, Transform{}, m_Material, m_Camera);

		// draw hit triangle while mousing over
		if (m_IsMousedOver)
		{
			experimental::Mesh m;
			m.setVerts(m_Tris);
			uint16_t indices[] = {0, 1, 2};
			m.setIndices(indices);

			m_Material.setVec4("uColor", {0.0f, 0.0f, 0.0f, 1.0f});
			m_Material.setDepthTested(false);
			experimental::Graphics::DrawMesh(m, Transform{}, m_Material, m_Camera);
		}

		if (m_UseBVH)
		{
			// draw BVH AABBs
			m_Material.setVec4("uColor", {0.0f, 0.0f, 0.0f, 1.0f});
			m_Material.setDepthTested(true);
			DrawBVH(m_BVH, m_CubeLinesMesh, m_Material, m_Camera);
		}

		// draw scene onto viewport
		m_Camera.render();

		// auxiliary 2D UI
		// printout stats
		if (true)
		{
			ImGui::Begin("controls");
			ImGui::Checkbox("BVH", &m_UseBVH);
			ImGui::Text("%ld microseconds", static_cast<long>(m_RaycastDuration.count()));
			auto r = m_Ray;
			ImGui::Text("camerapos = (%.2f, %.2f, %.2f)", m_Camera.getPosition().x, m_Camera.getPosition().y, m_Camera.getPosition().z);
			ImGui::Text("origin = (%.2f, %.2f, %.2f), dir = (%.2f, %.2f, %.2f)", r.origin.x, r.origin.y, r.origin.z, r.dir.x, r.dir.y, r.dir.z);
			if (m_IsMousedOver)
			{
				ImGui::Text("hit = (%.2f, %.2f, %.2f)", m_HitPos.x, m_HitPos.y, m_HitPos.z);
				ImGui::Text("p1 = (%.2f, %.2f, %.2f)", m_Tris[0].x, m_Tris[0].y, m_Tris[0].z);
				ImGui::Text("p2 = (%.2f, %.2f, %.2f)", m_Tris[1].x, m_Tris[1].y, m_Tris[1].z);
				ImGui::Text("p3 = (%.2f, %.2f, %.2f)", m_Tris[2].x, m_Tris[2].y, m_Tris[2].z);

			}
			ImGui::End();
		}
		m_PerfPanel.draw();
	}

private:

	// tab state
	UID m_ID;
	std::string m_Name = ICON_FA_COOKIE " MeshHittestTab";
	TabHost* m_Parent;

	// rendering
	experimental::Camera m_Camera;
	experimental::Material m_Material
	{
		experimental::Shader
		{
			App::slurp("shaders/SolidColor.vert"),
			App::slurp("shaders/SolidColor.frag"),
		}
	};
	experimental::Mesh m_Mesh = LoadMeshViaSimTK(App::resource("geometry/hat_ribs.vtp"));
	experimental::Mesh m_SphereMesh = experimental::LoadMeshFromMeshData(GenUntexturedUVSphere(12, 12));
	experimental::Mesh m_CubeLinesMesh = experimental::LoadMeshFromMeshData(GenCubeLines());

	// other state
	BVH m_BVH;
	bool m_UseBVH = false;
	glm::vec3 m_Tris[3]{};
	std::chrono::microseconds m_RaycastDuration{0};
	PolarPerspectiveCamera m_PolarCamera;
	bool m_IsMousedOver = false;
	glm::vec3 m_HitPos = {0.0f, 0.0f, 0.0f};
	Line m_Ray{};

	PerfPanel m_PerfPanel{"perf"};
};


// public API

osc::MeshHittestTab::MeshHittestTab(TabHost* parent) :
	m_Impl{new Impl{std::move(parent)}}
{
}

osc::MeshHittestTab::MeshHittestTab(MeshHittestTab&& tmp) noexcept :
	m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::MeshHittestTab& osc::MeshHittestTab::operator=(MeshHittestTab&& tmp) noexcept
{
	std::swap(m_Impl, tmp.m_Impl);
	return *this;
}

osc::MeshHittestTab::~MeshHittestTab() noexcept
{
	delete m_Impl;
}

osc::UID osc::MeshHittestTab::implGetID() const
{
	return m_Impl->getID();
}

osc::CStringView osc::MeshHittestTab::implGetName() const
{
	return m_Impl->getName();
}

osc::TabHost* osc::MeshHittestTab::implParent() const
{
	return m_Impl->parent();
}

void osc::MeshHittestTab::implOnMount()
{
	m_Impl->onMount();
}

void osc::MeshHittestTab::implOnUnmount()
{
	m_Impl->onUnmount();
}

bool osc::MeshHittestTab::implOnEvent(SDL_Event const& e)
{
	return m_Impl->onEvent(e);
}

void osc::MeshHittestTab::implOnTick()
{
	m_Impl->onTick();
}

void osc::MeshHittestTab::implOnDrawMainMenu()
{
	m_Impl->onDrawMainMenu();
}

void osc::MeshHittestTab::implOnDraw()
{
	m_Impl->onDraw();
}
