#include "ModelWarpDocument.h"

#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckState.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Utils/Algorithms.h>

#include <cstddef>
#include <filesystem>
#include <iterator>
#include <memory>
#include <sstream>
#include <utility>

using namespace osc;
using namespace osc::mow;

osc::mow::ModelWarpDocument::ModelWarpDocument() :
    m_ModelState{make_cow<BasicModelStatePair>()},
    m_ModelWarpConfig{make_cow<ModelWarpConfiguration>()},
    m_MeshWarpLookup{make_cow<PointWarperFactories>()},
    m_FrameWarpLookup{make_cow<FrameWarpLookup>()}
{}

osc::mow::ModelWarpDocument::ModelWarpDocument(std::filesystem::path const& osimFileLocation) :
    m_ModelState{make_cow<BasicModelStatePair>(osimFileLocation)},
    m_ModelWarpConfig{make_cow<ModelWarpConfiguration>(osimFileLocation, m_ModelState->getModel())},
    m_MeshWarpLookup{make_cow<PointWarperFactories>(osimFileLocation, m_ModelState->getModel(), *m_ModelWarpConfig)},
    m_FrameWarpLookup{make_cow<FrameWarpLookup>(osimFileLocation, m_ModelState->getModel(), *m_ModelWarpConfig)}
{}

osc::mow::ModelWarpDocument::ModelWarpDocument(ModelWarpDocument const&) = default;
osc::mow::ModelWarpDocument::ModelWarpDocument(ModelWarpDocument&&) noexcept = default;
osc::mow::ModelWarpDocument& osc::mow::ModelWarpDocument::operator=(ModelWarpDocument const&) = default;
osc::mow::ModelWarpDocument& osc::mow::ModelWarpDocument::operator=(ModelWarpDocument&&) noexcept = default;
osc::mow::ModelWarpDocument::~ModelWarpDocument() noexcept = default;

OpenSim::Model const& osc::mow::ModelWarpDocument::model() const
{
    return m_ModelState->getModel();
}

IConstModelStatePair const& osc::mow::ModelWarpDocument::modelstate() const
{
    return *m_ModelState;
}

std::vector<WarpDetail> osc::mow::ModelWarpDocument::details(OpenSim::Mesh const& mesh) const
{
    std::vector<WarpDetail> rv;
    rv.emplace_back("OpenSim::Mesh path in the OpenSim::Model", GetAbsolutePathString(mesh));

    if (IPointWarperFactory const* p = m_MeshWarpLookup->find(GetAbsolutePathString(mesh))) {
        auto inner = p->details();
        rv.insert(rv.end(), std::make_move_iterator(inner.begin()), std::make_move_iterator(inner.end()));
    }

    return rv;
}

std::vector<ValidationCheckResult> osc::mow::ModelWarpDocument::validate(OpenSim::Mesh const& mesh) const
{
    if (IPointWarperFactory const* p = m_MeshWarpLookup->find(GetAbsolutePathString(mesh))) {
        return p->validate();
    }
    else {
        return {ValidationCheckResult{"no mesh warp pairing found: this is probably an implementation error (try reloading?)", ValidationCheckState::Error}};
    }
}

ValidationCheckState osc::mow::ModelWarpDocument::state(OpenSim::Mesh const& mesh) const
{
    IPointWarperFactory const* p = m_MeshWarpLookup->find(GetAbsolutePathString(mesh));
    return p ? p->state() : ValidationCheckState::Error;
}

IPointWarperFactory const* osc::mow::ModelWarpDocument::findMeshWarp(OpenSim::Mesh const& mesh) const
{
    return m_MeshWarpLookup->find(GetAbsolutePathString(mesh));
}

std::vector<WarpDetail> osc::mow::ModelWarpDocument::details(OpenSim::PhysicalOffsetFrame const& pof) const
{
    if (IFrameWarperFactory const* p = m_FrameWarpLookup->find(GetAbsolutePathString(pof))) {
        return p->details();
    }
    return {};
}

std::vector<ValidationCheckResult> osc::mow::ModelWarpDocument::validate(OpenSim::PhysicalOffsetFrame const& pof) const
{
    if (IFrameWarperFactory const* p = m_FrameWarpLookup->find(GetAbsolutePathString(pof))) {
        return p->validate();
    }
    else {
        return {ValidationCheckResult{"no frame warp method found: this is probably an implementation error (try reloading?)", ValidationCheckState::Error}};
    }
}

ValidationCheckState osc::mow::ModelWarpDocument::state(
    OpenSim::PhysicalOffsetFrame const& pof) const
{
    IFrameWarperFactory const* p = m_FrameWarpLookup->find(GetAbsolutePathString(pof));
    return p ? p->state() : ValidationCheckState::Error;
}

ValidationCheckState osc::mow::ModelWarpDocument::state() const
{
    ValidationCheckState rv = ValidationCheckState::Ok;
    for (auto const& mesh : model().getComponentList<OpenSim::Mesh>()) {
        rv = max(rv , state(mesh));
    }
    for (auto const& pof : model().getComponentList<OpenSim::PhysicalOffsetFrame>()) {
        rv = max(rv, state(pof));
    }
    return rv;
}

float osc::mow::ModelWarpDocument::getWarpBlendingFactor() const
{
    return m_ModelWarpConfig->getWarpBlendingFactor();
}

void osc::mow::ModelWarpDocument::setWarpBlendingFactor(float v)
{
    m_ModelWarpConfig.upd()->setWarpBlendingFactor(v);
}

std::vector<ValidationCheckResult> osc::mow::ModelWarpDocument::implValidate() const
{
    std::vector<ValidationCheckResult> rv;
    for (auto const& mesh : model().getComponentList<OpenSim::Mesh>()) {
        rv.emplace_back(mesh.getName(), state(mesh));
    }
    for (auto const& pof : model().getComponentList<OpenSim::PhysicalOffsetFrame>()) {
        rv.emplace_back(pof.getName(), state(pof));
    }
    return rv;
}
