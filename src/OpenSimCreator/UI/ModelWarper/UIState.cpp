#include "UIState.hpp"

#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Platform/os.hpp>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <vector>

using osc::mow::MeshWarpPairing;
using osc::IsNameLexographicallyLowerThan;

namespace
{
    template<class T>
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

std::vector<OpenSim::Mesh const*> osc::mow::UIState::getWarpableMeshes() const
{
    return SortedComponentPointers<OpenSim::Mesh>(getModel());
}

std::vector<OpenSim::Frame const*> osc::mow::UIState::getWarpableFrames() const
{
    auto ptrs = SortedComponentPointers<OpenSim::Frame>(getModel());
    std::erase_if(ptrs, [](auto const* ptr) { return dynamic_cast<OpenSim::Ground const*>(ptr); });  // do not show Ground (not warpable)
    return ptrs;
}

MeshWarpPairing const* osc::mow::UIState::findMeshWarp(OpenSim::Mesh const& mesh) const
{
    return m_Document->findMeshWarp(GetAbsolutePathString(mesh));
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
