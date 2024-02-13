#include "Document.h"

#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Simulation/Model/Model.h>

#include <cstddef>
#include <filesystem>
#include <functional>
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

void osc::mow::Document::forEachWarpDetail(
    OpenSim::Mesh const& mesh,
    std::function<void(WarpDetail)> const& callback) const
{
    callback({ "OpenSim::Mesh path in the OpenSim::Model", GetAbsolutePathString(mesh) });

    if (IMeshWarp const* p = m_MeshWarpLookup.find(GetAbsolutePathString(mesh)))
    {
        p->forEachDetail(callback);
    }
}

void osc::mow::Document::forEachWarpCheck(
    OpenSim::Mesh const& mesh,
    std::function<ValidationCheckConsumerResponse(ValidationCheck)> const& callback) const
{
    if (IMeshWarp const* p = m_MeshWarpLookup.find(GetAbsolutePathString(mesh)))
    {
        p->forEachCheck(callback);
    }
    else
    {
        callback({ "no mesh warp pairing found: this is probably an implementation error (maybe reload?)", ValidationCheck::State::Error });
    }
}

ValidationCheck::State osc::mow::Document::warpState(OpenSim::Mesh const& mesh) const
{
    IMeshWarp const* p = m_MeshWarpLookup.find(GetAbsolutePathString(mesh));
    return p ? p->state() : ValidationCheck::State::Error;
}

void osc::mow::Document::forEachWarpCheck(
    OpenSim::PhysicalOffsetFrame const&,
    std::function<ValidationCheckConsumerResponse(ValidationCheck)> const&) const
{
    // TODO
    // - check associated mesh is found
    // - check origin location landmark
    // - check axis edge begin landmark
    // - check axis edge end landmark
    // - check nonparallel edge begin landmark
    // - check nonparallel edge end landmark
}

ValidationCheck::State osc::mow::Document::warpState(
    OpenSim::PhysicalOffsetFrame const&) const
{
    return ValidationCheck::State::Ok;
}
