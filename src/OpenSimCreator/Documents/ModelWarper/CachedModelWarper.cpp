#include "CachedModelWarper.h"

#include <OpenSimCreator/Graphics/OpenSimDecorationGenerator.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimCreator/Documents/Model/BasicModelStatePair.h>
#include <OpenSimCreator/Documents/ModelWarper/InMemoryMesh.h>
#include <OpenSimCreator/Documents/ModelWarper/ModelWarpDocument.h>
#include <OpenSimCreator/Utils/SimTKHelpers.h>
#include <oscar/Platform/Log.h>
#include <oscar/Utils/Assertions.h>

#include <map>
#include <memory>
#include <optional>

using namespace osc;
using namespace osc::mow;

namespace
{
    std::unique_ptr<InMemoryMesh> WarpMesh(
        ModelWarpDocument const& document,
        OpenSim::Model const& model,
        SimTK::State const& state,
        OpenSim::Mesh const& inputMesh,
        IPointWarperFactory const& warper)
    {
        // TODO: this ignores scale factors
        Mesh mesh = ToOscMesh(model, state, inputMesh);
        auto verts = mesh.getVerts();
        auto compiled = warper.tryCreatePointWarper(document);
        compiled->warpInPlace(verts);
        mesh.setVerts(verts);
        mesh.recalculateNormals();
        return std::make_unique<InMemoryMesh>(mesh);
    }

    void OverwriteGeometry(
        OpenSim::Model& model,
        OpenSim::Geometry& oldGeometry,
        std::unique_ptr<OpenSim::Geometry> newGeometry)
    {
        newGeometry->set_scale_factors(oldGeometry.get_scale_factors());
        newGeometry->set_Appearance(oldGeometry.get_Appearance());
        newGeometry->connectSocket_frame(oldGeometry.getConnectee("frame"));
        newGeometry->setName(oldGeometry.getName());
        OpenSim::Component* owner = UpdOwner(model, oldGeometry);
        OSC_ASSERT_ALWAYS(owner && "the mesh being replaced has no owner? cannot overwrite a root component");
        OSC_ASSERT_ALWAYS(TryDeleteComponentFromModel(model, oldGeometry) && "cannot delete old mesh from model during warping");
        InitializeModel(model);
        InitializeState(model);
        owner->addComponent(newGeometry.release());
        FinalizeConnections(model);
    }
}

class osc::mow::CachedModelWarper::Impl final {
public:
    std::shared_ptr<IConstModelStatePair const> warp(ModelWarpDocument const& document)
    {
        if (document != m_PreviousDocument) {
            m_PreviousResult = createWarpedModel(document);
            m_PreviousDocument = document;
        }
        return m_PreviousResult;
    }

    std::shared_ptr<IConstModelStatePair const> createWarpedModel(ModelWarpDocument const& document)
    {
        // copy the model into an editable "warped" version
        OpenSim::Model warpedModel{document.model()};
        InitializeModel(warpedModel);
        InitializeState(warpedModel);

        // iterate over each mesh in the model and warp it in-memory
        //
        // additionally, collect a base-frame-to-mesh lookup while doing this
        std::map<OpenSim::ComponentPath, std::vector<OpenSim::ComponentPath>> baseFrame2meshes;
        for (auto const& mesh : document.model().getComponentList<OpenSim::Mesh>()) {
            // try to warp+overwrite
            if (auto const* meshWarper = document.findMeshWarp(mesh)) {
                auto warpedMesh = WarpMesh(document, document.model(), document.modelstate().getState(), mesh, *meshWarper);
                auto* targetMesh = FindComponentMut<OpenSim::Mesh>(warpedModel, mesh.getAbsolutePath());
                OSC_ASSERT_ALWAYS(targetMesh && "cannot find target mesh in output model: this should never happen");
                OverwriteGeometry(warpedModel, *targetMesh, std::move(warpedMesh));
            }
            else {
                return nullptr;  // no warper for the mesh (not even an identity warp): halt
            }

            // update base-frame-to-mesh lookup
            auto const& [it, inserted] = baseFrame2meshes.try_emplace(mesh.getFrame().findBaseFrame().getAbsolutePath());
            it->second.push_back(mesh.getAbsolutePath());
        }
        InitializeModel(warpedModel);
        InitializeState(warpedModel);

        // iterate over each `PathPoint` in the model (incl. muscle points) and warp them by
        // figuring out how each relates to a mesh in the model
        //
        // TODO: the `osc::mow::ModelWarpDocument` should handle figuring out each point's warper, because
        // there are situations where there isn't a 1:1 relationship between meshes and bodies
        for (auto& station : warpedModel.updComponentList<OpenSim::PathPoint>()) {
            auto baseFramePath = station.getParentFrame().findBaseFrame().getAbsolutePath();
            if (auto it = baseFrame2meshes.find(baseFramePath); it != baseFrame2meshes.end()) {
                if (it->second.size() == 1) {
                    if (auto const* mesh = FindComponent<OpenSim::Mesh>(document.model(), it->second.front())) {
                        if (auto const meshWarper = document.findMeshWarp(*mesh)) {
                            // redefine the station's position in the mesh's coordinate system
                            auto posInMeshFrame = station.getParentFrame().expressVectorInAnotherFrame(warpedModel.getWorkingState(), station.get_location(), mesh->getFrame());
                            auto warpedInMeshFrame = ToSimTKVec3(meshWarper->tryCreatePointWarper(document)->warp(ToVec3(posInMeshFrame)));
                            auto warpedInParentFrame = mesh->getFrame().expressVectorInAnotherFrame(warpedModel.getWorkingState(), warpedInMeshFrame, station.getParentFrame());
                            station.set_location(warpedInParentFrame);
                        }
                        else {
                            log_warn("no warper available for %s", it->second.front().toString().c_str());
                        }
                    }
                    else {
                        log_error("cannot find %s in the model: this shouldn't happen", it->second.front().toString().c_str());
                    }
                }
                else {
                    log_warn("cannot warp %s: there are multiple meshes attached to the same base frame, so it's ambiguous how to warp this point", station.getName().c_str());
                }
            }
            else {
                log_warn("cannot warp %s: there don't appear to be any meshes attached to the same base frame?", station.getName().c_str());
            }
        }
        InitializeModel(warpedModel);
        InitializeState(warpedModel);

        return std::make_shared<BasicModelStatePair>(
            warpedModel.getModel(),
            warpedModel.getWorkingState()
        );
    }
private:
    std::optional<ModelWarpDocument> m_PreviousDocument;
    std::shared_ptr<IConstModelStatePair const> m_PreviousResult;
};

osc::mow::CachedModelWarper::CachedModelWarper() :
    m_Impl{std::make_unique<Impl>()}
{}
osc::mow::CachedModelWarper::CachedModelWarper(CachedModelWarper&&) noexcept = default;
CachedModelWarper& osc::mow::CachedModelWarper::operator=(CachedModelWarper&&) noexcept = default;
osc::mow::CachedModelWarper::~CachedModelWarper() noexcept = default;

std::shared_ptr<IConstModelStatePair const> osc::mow::CachedModelWarper::warp(ModelWarpDocument const& document)
{
    return m_Impl->warp(document);
}
