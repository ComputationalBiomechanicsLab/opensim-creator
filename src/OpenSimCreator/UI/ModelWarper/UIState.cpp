#include "UIState.hpp"

#include <OpenSimCreator/Documents/ModelWarper/Detail.hpp>
#include <OpenSimCreator/Documents/ModelWarper/Document.hpp>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheck.hpp>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckConsumerResponse.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Platform/os.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <vector>

using namespace osc;
using namespace osc::mow;

namespace
{
    template<std::derived_from<OpenSim::Component> T>
    std::vector<T const*> SortedComponentPointers(OpenSim::Model const& model)
    {
        std::vector<T const*> rv;
        for (auto const& v : model.getComponentList<T>())
        {
            rv.push_back(&v);
        }
        std::sort(rv.begin(), rv.end(), IsNameLexographicallyLowerThan<T const*>);
        return rv;
    }
}

OpenSim::Model const& osc::mow::UIState::getModel() const
{
    return m_Document->getModel();
}

size_t osc::mow::UIState::getNumWarpableMeshesInModel() const
{
    return m_Document->getNumWarpableMeshesInModel();
}

void osc::mow::UIState::forEachWarpableMeshInModel(std::function<void(OpenSim::Mesh const&)> const& callback) const
{
    return m_Document->forEachWarpableMeshInModel(callback);
}

void osc::mow::UIState::forEachMeshWarpDetail(OpenSim::Mesh const& mesh, std::function<void(Detail)> const& callback) const
{
    m_Document->forEachMeshWarpDetail(mesh, callback);
}

void osc::mow::UIState::forEachMeshWarpCheck(OpenSim::Mesh const& mesh, std::function<ValidationCheckConsumerResponse(ValidationCheck)> const& callback) const
{
    m_Document->forEachMeshWarpCheck(mesh, callback);
}

ValidationCheck::State osc::mow::UIState::getMeshWarpState(OpenSim::Mesh const& mesh) const
{
    return m_Document->getMeshWarpState(mesh);
}

size_t osc::mow::UIState::getNumWarpableFramesInModel() const
{
    return m_Document->getNumWarpableFramesInModel();
}

void osc::mow::UIState::forEachWarpableFrameInModel(std::function<void(OpenSim::PhysicalOffsetFrame const&)> const& callback) const
{
    m_Document->forEachWarpableFrameInModel(callback);
}

void osc::mow::UIState::forEachFrameDefinitionCheck(
    OpenSim::PhysicalOffsetFrame const& frame,
    std::function<ValidationCheckConsumerResponse(ValidationCheck)> const& callback) const
{
    m_Document->forEachFrameDefinitionCheck(frame, callback);
}

void osc::mow::UIState::actionOpenModel(std::optional<std::filesystem::path> path)
{
    if (!path)
    {
        path = PromptUserForFile("osim");
    }

    if (path)
    {
        m_Document = std::make_shared<Document>(std::move(path).value());
    }
}
