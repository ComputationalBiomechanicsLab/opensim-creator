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

using namespace osc::mow;

osc::mow::Document::Document() :
    m_Model{std::make_unique<OpenSim::Model>()}
{}

osc::mow::Document::Document(std::filesystem::path const& osimFileLocation) :
    m_Model{std::make_unique<OpenSim::Model>(osimFileLocation.string())},
    m_ModelWarpConfig{osimFileLocation, *m_Model},
    m_MeshWarpLookup{osimFileLocation, *m_Model, m_ModelWarpConfig},
    m_FrameWarpLookup{osimFileLocation, *m_Model, m_ModelWarpConfig}
{}

osc::mow::Document::Document(Document const&) = default;
osc::mow::Document::Document(Document&&) noexcept = default;
osc::mow::Document& osc::mow::Document::operator=(Document const&) = default;
osc::mow::Document& osc::mow::Document::operator=(Document&&) noexcept = default;
osc::mow::Document::~Document() noexcept = default;

std::vector<WarpDetail> osc::mow::Document::details(OpenSim::Mesh const& mesh) const
{
    std::vector<WarpDetail> rv;
    rv.emplace_back("OpenSim::Mesh path in the OpenSim::Model", GetAbsolutePathString(mesh));

    if (IMeshWarp const* p = m_MeshWarpLookup.find(GetAbsolutePathString(mesh))) {
        auto inner = p->details();
        rv.insert(rv.end(), std::make_move_iterator(inner.begin()), std::make_move_iterator(inner.end()));
    }

    return rv;
}

std::vector<ValidationCheck> osc::mow::Document::validate(OpenSim::Mesh const& mesh) const
{
    if (IMeshWarp const* p = m_MeshWarpLookup.find(GetAbsolutePathString(mesh))) {
        return p->validate();
    }
    else {
        return {ValidationCheck{"no mesh warp pairing found: this is probably an implementation error (maybe reload?)", ValidationState::Error}};
    }
}

ValidationState osc::mow::Document::state(OpenSim::Mesh const& mesh) const
{
    IMeshWarp const* p = m_MeshWarpLookup.find(GetAbsolutePathString(mesh));
    return p ? p->state() : ValidationState::Error;
}

std::vector<WarpDetail> osc::mow::Document::details(OpenSim::PhysicalOffsetFrame const&) const
{
    return {};  // TODO
}

std::vector<ValidationCheck> osc::mow::Document::validate(OpenSim::PhysicalOffsetFrame const&) const
{
    // TODO
    // - check associated mesh is found
    // - check origin location landmark
    // - check axis edge begin landmark
    // - check axis edge end landmark
    // - check nonparallel edge begin landmark
    // - check nonparallel edge end landmark
    return {};
}

ValidationState osc::mow::Document::state(
    OpenSim::PhysicalOffsetFrame const&) const
{
    return ValidationState::Ok;
}
