#include "CachedModelWarper.h"

#include <OpenSimCreator/Graphics/OpenSimDecorationGenerator.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimCreator/Documents/Model/BasicModelStatePair.h>
#include <OpenSimCreator/Documents/ModelWarper/Document.h>
#include <OpenSimCreator/Documents/ModelWarper/InMemoryMesh.h>
#include <oscar/Utils/Assertions.h>

#include <memory>
#include <optional>

using namespace osc;
using namespace osc::mow;

namespace
{
    std::unique_ptr<InMemoryMesh> WarpMesh(
        Document const& document,
        OpenSim::Model const& model,
        SimTK::State const& state,
        OpenSim::Mesh const& inputMesh,
        IMeshWarp const& warper)
    {
        // TODO: this ignores scale factors
        Mesh mesh = ToOscMesh(model, state, inputMesh);
        auto verts = mesh.getVerts();
        auto compiled = warper.compileWarper(document);
        compiled->warpInPlace(verts);
        return std::make_unique<InMemoryMesh>(verts, mesh.getIndices());
    }

    void OverwriteGeometry(
        OpenSim::Model& model,
        OpenSim::Geometry& oldGeometry,
        std::unique_ptr<OpenSim::Geometry> newGeometry)
    {
        newGeometry->set_scale_factors(oldGeometry.get_scale_factors());
        newGeometry->set_Appearance(oldGeometry.get_Appearance());
        newGeometry->connectSocket_frame(oldGeometry.getConnectee("frame"));
        OpenSim::Component& owner = const_cast<OpenSim::Component&>(oldGeometry.getOwner());  // TODO: use mutable lookup
        OSC_ASSERT_ALWAYS(TryDeleteComponentFromModel(model, oldGeometry) && "cannot delete old mesh from model during warping");
        InitializeModel(model);
        InitializeState(model);
        owner.addComponent(newGeometry.release());
        FinalizeConnections(model);
    }
}

class osc::mow::CachedModelWarper::Impl final {
public:
    std::shared_ptr<IConstModelStatePair const> warp(Document const& document)
    {
        if (document != m_PreviousDocument) {
            m_PreviousResult = createWarpedModel(document);
            m_PreviousDocument = document;
        }
        return m_PreviousResult;
    }

    std::shared_ptr<IConstModelStatePair const> createWarpedModel(Document const& document)
    {
        OpenSim::Model warpedModel{document.model()};
        InitializeModel(warpedModel);
        InitializeState(warpedModel);

        // iterate over component in the source model and write warped versions into
        // the warped model where necessary
        for (auto const& mesh : document.model().getComponentList<OpenSim::Mesh>()) {
            if (auto const* meshWarper = document.findMeshWarp(mesh)) {
                auto warpedMesh = WarpMesh(document, document.model(), document.modelstate().getState(), mesh, *meshWarper);
                OpenSim::Mesh* targetMesh = FindComponentMut<OpenSim::Mesh>(warpedModel, mesh.getAbsolutePath());
                OSC_ASSERT_ALWAYS(targetMesh && "cannot find target mesh in output model: this should never happen");
                OverwriteGeometry(warpedModel, *targetMesh, std::move(warpedMesh));
            }
            else {
                return nullptr;  // no warper for the mesh (not even an identity warp): halt
            }
        }

        return std::make_shared<BasicModelStatePair>(
            warpedModel.getModel(),
            warpedModel.getWorkingState()
        );
    }
private:
    std::optional<Document> m_PreviousDocument;
    std::shared_ptr<IConstModelStatePair const> m_PreviousResult;
};

osc::mow::CachedModelWarper::CachedModelWarper() :
    m_Impl{std::make_unique<Impl>()}
{}
osc::mow::CachedModelWarper::CachedModelWarper(CachedModelWarper&&) noexcept = default;
CachedModelWarper& osc::mow::CachedModelWarper::operator=(CachedModelWarper&&) noexcept = default;
osc::mow::CachedModelWarper::~CachedModelWarper() noexcept = default;

std::shared_ptr<IConstModelStatePair const> osc::mow::CachedModelWarper::warp(Document const& document)
{
    return m_Impl->warp(document);
}
