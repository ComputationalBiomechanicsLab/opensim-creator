#include "SubMeshTab.hpp"

#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Graphics/SubMeshDescriptor.hpp>
#include <oscar/Maths/Transform.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/UI/Tabs/StandardTabImpl.hpp>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <array>
#include <cstdint>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using osc::CStringView;
using osc::Mesh;
using osc::SubMeshDescriptor;
using osc::Vec3;

namespace
{
    constexpr CStringView c_TabStringID = "Demos/SubMeshes";

    template<std::ranges::range T, std::ranges::range U>
    void Append(T& out, U els)
        requires std::same_as<typename T::value_type, typename U::value_type>
    {
        out.insert(out.end(), els.begin(), els.end());
    }

    Mesh GenerateMeshWithSubMeshes()
    {
        auto const meshes = std::to_array(
        {
            osc::GenCube(),
            osc::GenSphere(16, 16),
            osc::GenCircle(32),
        });

        std::vector<Vec3> allVerts;
        std::vector<Vec3> allNormals;
        std::vector<uint32_t> allIndices;
        std::vector<SubMeshDescriptor> allDescriptors;

        for (auto const& mesh : meshes)
        {
            Append(allVerts, mesh.getVerts());
            Append(allNormals, mesh.getNormals());

            size_t firstIndex = allIndices.size();
            for (auto index : mesh.getIndices())
            {
                allIndices.push_back(static_cast<uint32_t>(firstIndex + index));
            }
            size_t nIndices = allIndices.size() - firstIndex;

            allDescriptors.emplace_back(firstIndex, nIndices, mesh.getTopology());
        }

        Mesh rv;
        rv.setVerts(allVerts);
        rv.setNormals(allNormals);
        rv.setIndices(allIndices);
        for (auto const& desc : allDescriptors)
        {
            rv.pushSubMeshDescriptor(desc);
        }
        return rv;
    }
}

class osc::SubMeshTab::Impl final : public osc::StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_TabStringID}
    {
        m_Camera.setBackgroundColor(Color::white());
        m_Camera.setNearClippingPlane(0.1f);
        m_Camera.setFarClippingPlane(5.0f);
        m_Camera.setPosition({0.0f, 0.0f, -2.5f});
        m_Camera.setDirection({0.0f, 0.0f, 1.0f});

        m_Material.setColor("uColor", Color::red());
        m_Material.setWireframeMode(true);
    }

private:
    void implOnMount() final {}
    void implOnUnmount() final {}

    bool implOnEvent(SDL_Event const&) final
    {
        return false;
    }

    void implOnTick() final {}

    void implOnDrawMainMenu() final {}

    void implOnDraw() final
    {
        for (size_t subMeshIndex = 0; subMeshIndex < m_MeshWithSubmeshes.getSubMeshCount(); ++subMeshIndex)
        {

            Graphics::DrawMesh(
                m_MeshWithSubmeshes,
                Identity<Transform>(),
                m_Material,
                m_Camera,
                std::nullopt,
                subMeshIndex
            );
        }
        m_Camera.setPixelRect(GetMainViewportWorkspaceScreenRect());
        m_Camera.renderToScreen();
    }

    Camera m_Camera;
    Material m_Material
    {
        Shader
        {
            App::slurp("oscar_demos/shaders/SolidColor.vert"),
            App::slurp("oscar_demos/shaders/SolidColor.frag"),
        },
    };
    Mesh m_MeshWithSubmeshes = GenerateMeshWithSubMeshes();
};


// public API

CStringView osc::SubMeshTab::id()
{
    return c_TabStringID;
}

osc::SubMeshTab::SubMeshTab(ParentPtr<ITabHost> const&) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::SubMeshTab::SubMeshTab(SubMeshTab&&) noexcept = default;
osc::SubMeshTab& osc::SubMeshTab::operator=(SubMeshTab&&) noexcept = default;
osc::SubMeshTab::~SubMeshTab() noexcept = default;

osc::UID osc::SubMeshTab::implGetID() const
{
    return m_Impl->getID();
}

CStringView osc::SubMeshTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::SubMeshTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::SubMeshTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::SubMeshTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::SubMeshTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::SubMeshTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::SubMeshTab::implOnDraw()
{
    m_Impl->onDraw();
}
