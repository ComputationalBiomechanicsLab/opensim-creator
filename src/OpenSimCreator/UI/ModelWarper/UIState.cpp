#include "UIState.h"

#include <OpenSimCreator/Documents/ModelWarper/Document.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheck.h>
#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckConsumerResponse.h>
#include <OpenSimCreator/Documents/ModelWarper/WarpDetail.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Platform/os.h>

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
        for (auto const& v : model.getComponentList<T>()) {
            rv.push_back(&v);
        }
        std::sort(rv.begin(), rv.end(), IsNameLexographicallyLowerThan<T const*>);
        return rv;
    }
}

OpenSim::Model const& osc::mow::UIState::model() const
{
    return m_Document->model();
}

void osc::mow::UIState::forEachWarpDetail(OpenSim::Mesh const& mesh, std::function<void(WarpDetail)> const& callback) const
{
    m_Document->forEachWarpDetail(mesh, callback);
}

void osc::mow::UIState::forEachWarpCheck(OpenSim::Mesh const& mesh, std::function<ValidationCheckConsumerResponse(ValidationCheck)> const& callback) const
{
    m_Document->forEachWarpCheck(mesh, callback);
}

ValidationCheck::State osc::mow::UIState::warpState(OpenSim::Mesh const& mesh) const
{
    return m_Document->warpState(mesh);
}

void osc::mow::UIState::forEachWarpCheck(
    OpenSim::PhysicalOffsetFrame const& frame,
    std::function<ValidationCheckConsumerResponse(ValidationCheck)> const& callback) const
{
    m_Document->forEachWarpCheck(frame, callback);
}

void osc::mow::UIState::actionOpenOsimOrPromptUser(std::optional<std::filesystem::path> path)
{
    if (!path) {
        path = PromptUserForFile("osim");
    }

    if (path) {
        m_Document = std::make_shared<Document>(std::move(path).value());
    }
}
