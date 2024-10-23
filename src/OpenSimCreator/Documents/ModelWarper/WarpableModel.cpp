#include "WarpableModel.h"

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

osc::mow::WarpableModel::WarpableModel() :
    m_ModelState{make_cow<BasicModelStatePair>()},
    m_ModelWarpConfig{make_cow<ModelWarpConfiguration>()},
    m_MeshWarpLookup{make_cow<PointWarperFactories>()},
    m_FrameWarpLookup{make_cow<FrameWarperFactories>()}
{}

osc::mow::WarpableModel::WarpableModel(const std::filesystem::path& osimFileLocation) :
    m_ModelState{make_cow<BasicModelStatePair>(osimFileLocation)},
    m_ModelWarpConfig{make_cow<ModelWarpConfiguration>(osimFileLocation, m_ModelState->getModel())},
    m_MeshWarpLookup{make_cow<PointWarperFactories>(osimFileLocation, m_ModelState->getModel(), *m_ModelWarpConfig)},
    m_FrameWarpLookup{make_cow<FrameWarperFactories>(osimFileLocation, m_ModelState->getModel(), *m_ModelWarpConfig)}
{}

osc::mow::WarpableModel::WarpableModel(const WarpableModel&) = default;
osc::mow::WarpableModel::WarpableModel(WarpableModel&&) noexcept = default;
osc::mow::WarpableModel& osc::mow::WarpableModel::operator=(const WarpableModel&) = default;
osc::mow::WarpableModel& osc::mow::WarpableModel::operator=(WarpableModel&&) noexcept = default;
osc::mow::WarpableModel::~WarpableModel() noexcept = default;

std::vector<WarpDetail> osc::mow::WarpableModel::details(const OpenSim::Mesh& mesh) const
{
    std::vector<WarpDetail> rv;
    rv.emplace_back("OpenSim::Mesh path in the OpenSim::Model", GetAbsolutePathString(mesh));

    if (const IPointWarperFactory* p = m_MeshWarpLookup->find(GetAbsolutePathString(mesh))) {
        auto inner = p->details();
        rv.insert(rv.end(), std::make_move_iterator(inner.begin()), std::make_move_iterator(inner.end()));
    }

    return rv;
}

std::vector<ValidationCheckResult> osc::mow::WarpableModel::validate(const OpenSim::Mesh& mesh) const
{
    if (const IPointWarperFactory* p = m_MeshWarpLookup->find(GetAbsolutePathString(mesh))) {
        return p->validate(*this);
    }
    else {
        return {ValidationCheckResult{"no mesh warp pairing found: this is probably an implementation error (try reloading?)", ValidationCheckState::Error}};
    }
}

ValidationCheckState osc::mow::WarpableModel::state(const OpenSim::Mesh& mesh) const
{
    const IPointWarperFactory* p = m_MeshWarpLookup->find(GetAbsolutePathString(mesh));
    return p ? p->state(*this) : ValidationCheckState::Error;
}

const IPointWarperFactory* osc::mow::WarpableModel::findMeshWarp(const OpenSim::Mesh& mesh) const
{
    return m_MeshWarpLookup->find(GetAbsolutePathString(mesh));
}

std::vector<WarpDetail> osc::mow::WarpableModel::details(const OpenSim::PhysicalOffsetFrame& pof) const
{
    if (const IFrameWarperFactory* p = m_FrameWarpLookup->find(GetAbsolutePathString(pof))) {
        return p->details();
    }
    return {};
}

std::vector<ValidationCheckResult> osc::mow::WarpableModel::validate(const OpenSim::PhysicalOffsetFrame& pof) const
{
    if (const IFrameWarperFactory* p = m_FrameWarpLookup->find(GetAbsolutePathString(pof))) {
        return p->validate(*this);
    }
    else {
        return {ValidationCheckResult{"no frame warp method found: this is probably an implementation error (try reloading?)", ValidationCheckState::Error}};
    }
}

ValidationCheckState osc::mow::WarpableModel::state(
    const OpenSim::PhysicalOffsetFrame& pof) const
{
    const IFrameWarperFactory* p = m_FrameWarpLookup->find(GetAbsolutePathString(pof));
    return p ? p->state(*this) : ValidationCheckState::Error;
}

ValidationCheckState osc::mow::WarpableModel::state() const
{
    ValidationCheckState rv = ValidationCheckState::Ok;
    for (const auto& mesh : getModel().getComponentList<OpenSim::Mesh>()) {
        rv = max(rv , state(mesh));
    }
    for (const auto& pof : getModel().getComponentList<OpenSim::PhysicalOffsetFrame>()) {
        rv = max(rv, state(pof));
    }
    return rv;
}

float osc::mow::WarpableModel::getWarpBlendingFactor() const
{
    return m_ModelWarpConfig->getWarpBlendingFactor();
}

void osc::mow::WarpableModel::setWarpBlendingFactor(float v)
{
    m_ModelWarpConfig.upd()->setWarpBlendingFactor(v);
}

bool osc::mow::WarpableModel::getShouldWriteWarpedMeshesToDisk() const
{
    return m_ModelWarpConfig->getShouldWriteWarpedMeshesToDisk();
}

void osc::mow::WarpableModel::setShouldWriteWarpedMeshesToDisk(bool v)
{
    m_ModelWarpConfig.upd()->setShouldWriteWarpedMeshesToDisk(v);
}

std::optional<std::filesystem::path> osc::mow::WarpableModel::getWarpedMeshesOutputDirectory() const
{
    const auto osimFileLocation = getOsimFileLocation();
    if (not osimFileLocation) {
        return std::nullopt;
    }
    return std::filesystem::weakly_canonical(osimFileLocation->parent_path() / m_ModelWarpConfig->getWarpedMeshesOutputDirectory());
}

std::optional<std::filesystem::path> osc::mow::WarpableModel::getOsimFileLocation() const
{
    return TryFindInputFile(m_ModelState->getModel());
}

std::vector<ValidationCheckResult> osc::mow::WarpableModel::implValidate(const WarpableModel&) const
{
    std::vector<ValidationCheckResult> rv;
    for (const auto& mesh : getModel().getComponentList<OpenSim::Mesh>()) {
        rv.emplace_back(mesh.getName(), state(mesh));
    }
    for (const auto& pof : getModel().getComponentList<OpenSim::PhysicalOffsetFrame>()) {
        rv.emplace_back(pof.getName(), state(pof));
    }
    return rv;
}

const OpenSim::Model& osc::mow::WarpableModel::implGetModel() const
{
    return m_ModelState->getModel();
}

const SimTK::State& osc::mow::WarpableModel::implGetState() const
{
    return m_ModelState->getState();
}

float osc::mow::WarpableModel::implGetFixupScaleFactor() const
{
    return m_ModelState->getFixupScaleFactor();
}

void osc::mow::WarpableModel::implSetFixupScaleFactor(float sf)
{
    m_ModelState.upd()->setFixupScaleFactor(sf);
}

std::shared_ptr<Environment> osc::mow::WarpableModel::implUpdAssociatedEnvironment() const
{
    return m_ModelState->tryUpdEnvironment();
}
