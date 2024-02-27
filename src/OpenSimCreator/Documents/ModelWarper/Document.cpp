#include "Document.h"

#include <OpenSimCreator/Documents/ModelWarper/ValidationState.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Simulation/Model/Model.h>

#include <cstddef>
#include <filesystem>
#include <iterator>
#include <memory>
#include <sstream>
#include <utility>

using namespace osc;
using namespace osc::mow;

osc::mow::Document::Document() :
    m_ModelState{make_cow<BasicModelStatePair>()},
    m_ModelWarpConfig{make_cow<ModelWarpConfiguration>()},
    m_MeshWarpLookup{make_cow<MeshWarpLookup>()},
    m_FrameWarpLookup{make_cow<FrameWarpLookup>()}
{}

osc::mow::Document::Document(std::filesystem::path const& osimFileLocation) :
    m_ModelState{make_cow<BasicModelStatePair>(osimFileLocation)},
    m_ModelWarpConfig{make_cow<ModelWarpConfiguration>(osimFileLocation, m_ModelState->getModel())},
    m_MeshWarpLookup{make_cow<MeshWarpLookup>(osimFileLocation, m_ModelState->getModel(), *m_ModelWarpConfig)},
    m_FrameWarpLookup{make_cow<FrameWarpLookup>(osimFileLocation, m_ModelState->getModel(), *m_ModelWarpConfig)}
{}

osc::mow::Document::Document(Document const&) = default;
osc::mow::Document::Document(Document&&) noexcept = default;
osc::mow::Document& osc::mow::Document::operator=(Document const&) = default;
osc::mow::Document& osc::mow::Document::operator=(Document&&) noexcept = default;
osc::mow::Document::~Document() noexcept = default;

OpenSim::Model const& osc::mow::Document::model() const
{
    return m_ModelState->getModel();
}

IConstModelStatePair const& osc::mow::Document::modelstate() const
{
    return *m_ModelState;
}

std::vector<WarpDetail> osc::mow::Document::details(OpenSim::Mesh const& mesh) const
{
    std::vector<WarpDetail> rv;
    rv.emplace_back("OpenSim::Mesh path in the OpenSim::Model", GetAbsolutePathString(mesh));

    if (IMeshWarp const* p = m_MeshWarpLookup->find(GetAbsolutePathString(mesh))) {
        auto inner = p->details();
        rv.insert(rv.end(), std::make_move_iterator(inner.begin()), std::make_move_iterator(inner.end()));
    }

    return rv;
}

std::vector<ValidationCheck> osc::mow::Document::validate(OpenSim::Mesh const& mesh) const
{
    if (IMeshWarp const* p = m_MeshWarpLookup->find(GetAbsolutePathString(mesh))) {
        return p->validate();
    }
    else {
        return {ValidationCheck{"no mesh warp pairing found: this is probably an implementation error (try reloading?)", ValidationState::Error}};
    }
}

ValidationState osc::mow::Document::state(OpenSim::Mesh const& mesh) const
{
    IMeshWarp const* p = m_MeshWarpLookup->find(GetAbsolutePathString(mesh));
    return p ? p->state() : ValidationState::Error;
}

std::vector<WarpDetail> osc::mow::Document::details(OpenSim::PhysicalOffsetFrame const& pof) const
{
    if (IFrameWarp const* p = m_FrameWarpLookup->find(GetAbsolutePathString(pof))) {
        return p->details();
    }
    return {};
}

std::vector<ValidationCheck> osc::mow::Document::validate(OpenSim::PhysicalOffsetFrame const& pof) const
{
    if (IFrameWarp const* p = m_FrameWarpLookup->find(GetAbsolutePathString(pof))) {
        return p->validate();
    }
    else {
        return {ValidationCheck{"no frame warp method found: this is probably an implementation error (try reloading?)", ValidationState::Error}};
    }
}

ValidationState osc::mow::Document::state(
    OpenSim::PhysicalOffsetFrame const& pof) const
{
    IFrameWarp const* p = m_FrameWarpLookup->find(GetAbsolutePathString(pof));
    return p ? p->state() : ValidationState::Error;
}

ValidationState osc::mow::Document::state() const
{
    ValidationState rv = ValidationState::Ok;
    for (auto const& mesh : model().getComponentList<OpenSim::Mesh>()) {
        rv = std::max(rv , state(mesh));
    }
    for (auto const& pof : model().getComponentList<OpenSim::PhysicalOffsetFrame>()) {
        rv = std::max(rv, state(pof));
    }
    return rv;
}

std::vector<ValidationCheck> osc::mow::Document::implValidate() const
{
    std::vector<ValidationCheck> rv;
    for (auto const& mesh : model().getComponentList<OpenSim::Mesh>()) {
        rv.emplace_back(mesh.getName(), state(mesh));
    }
    for (auto const& pof : model().getComponentList<OpenSim::PhysicalOffsetFrame>()) {
        rv.emplace_back(pof.getName(), state(pof));
    }
    return rv;
}
