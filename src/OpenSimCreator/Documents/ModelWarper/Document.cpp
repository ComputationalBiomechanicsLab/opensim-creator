#include "Document.hpp"

#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <OpenSim/Simulation/Model/Model.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

using osc::mow::ValidationCheck;

osc::mow::Document::Document() :
    m_Model{std::make_unique<OpenSim::Model>()}
{
}

osc::mow::Document::Document(std::filesystem::path const& osimPath) :
    m_Model{std::make_unique<OpenSim::Model>(osimPath.string())},
    m_TopLevelWarpConfig{osimPath},
    m_MeshWarpPairingLookup{osimPath, *m_Model},
    m_FrameDefinitionLookup{osimPath, *m_Model}
{
}

osc::mow::Document::Document(Document const&) = default;
osc::mow::Document::Document(Document&&) noexcept = default;
osc::mow::Document& osc::mow::Document::operator=(Document const&) = default;
osc::mow::Document& osc::mow::Document::operator=(Document&&) noexcept = default;
osc::mow::Document::~Document() noexcept = default;

size_t osc::mow::Document::getNumWarpableMeshesInModel() const
{
    return GetNumChildren<OpenSim::Mesh>(*m_Model);
}

void osc::mow::Document::forEachWarpableMeshInModel(
    std::function<void(OpenSim::Mesh const&)> const& callback) const
{
    for (auto const& mesh : m_Model->getComponentList<OpenSim::Mesh>())
    {
        callback(mesh);
    }
}

void osc::mow::Document::forEachMeshWarpDetail(
    OpenSim::Mesh const& mesh,
    std::function<void(Detail)> const& callback) const
{
    callback({ "OpenSim::Mesh path in the OpenSim::Model", GetAbsolutePathString(mesh) });

    if (MeshWarpPairing const* p = m_MeshWarpPairingLookup.lookup(GetAbsolutePathString(mesh)))
    {
        p->forEachDetail(callback);
    }
}

void osc::mow::Document::forEachMeshWarpCheck(
    OpenSim::Mesh const& mesh,
    std::function<ValidationCheckConsumerResponse(ValidationCheck)> const& callback) const
{
    if (MeshWarpPairing const* p = m_MeshWarpPairingLookup.lookup(GetAbsolutePathString(mesh)))
    {
        p->forEachCheck(callback);
    }
    else
    {
        callback({ "no mesh warp pairing found: this is probably an implementation error (maybe reload?)", ValidationCheck::State::Error });
    }
}

ValidationCheck::State osc::mow::Document::getMeshWarpState(OpenSim::Mesh const& mesh) const
{
    MeshWarpPairing const* p = m_MeshWarpPairingLookup.lookup(GetAbsolutePathString(mesh));
    return p ? p->state() : ValidationCheck::State::Error;
}

size_t osc::mow::Document::getNumWarpableFramesInModel() const
{
    return GetNumChildren<OpenSim::PhysicalOffsetFrame>(*m_Model);
}

void osc::mow::Document::forEachWarpableFrameInModel(
    std::function<void(OpenSim::PhysicalOffsetFrame const&)> const& callback) const
{
    for (auto const& frame : m_Model->getComponentList<OpenSim::PhysicalOffsetFrame>())
    {
        callback(frame);
    }
}

void osc::mow::Document::forEachFrameDefinitionCheck(
    OpenSim::PhysicalOffsetFrame const&,
    std::function<ValidationCheckConsumerResponse(ValidationCheck)> const& callback) const
{
    // check for frames file
    {
        std::stringstream ss;
        ss << "has a frame definition file at " << recommendedFrameDefinitionFilepath().string();
        if (callback({std::move(ss).str(), hasFrameDefinitionFile()}) == ValidationCheckConsumerResponse::Stop)
        {
            return;
        }
    }

    // check for top-level parse errors in frames file
    {
        std::stringstream ss;
        ValidationCheck::State state = ValidationCheck::State::Ok;
        if (!hasFrameDefinitionFile())
        {
            ss << "frame definition file error: the file does not exist";
            state = ValidationCheck::State::Error;
        }
        else if (auto err = getFramesFileLoadError())
        {
            ss << "frame definition file error: " << *err;
            state = ValidationCheck::State::Error;
        }
        else
        {
            ss << "frame definition file contains no errors";
        }

        if (callback({std::move(ss).str(), state}) == ValidationCheckConsumerResponse::Stop)
        {
            return;
        }
    }

    // TODO
    // - check associated mesh is found
    // - check origin location landmark
    // - check axis edge begin landmark
    // - check axis edge end landmark
    // - check nonparallel edge begin landmark
    // - check nonparallel edge end landmark
}
