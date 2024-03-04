#include "CachedModelWarper.h"

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
        OpenSim::Mesh const&,
        IMeshWarp const&)
    {
        return std::make_unique<InMemoryMesh>();
    }

    void OverwriteGeometry(
        OpenSim::Geometry&,
        std::unique_ptr<OpenSim::Geometry>)
    {
        // TODO: update sockets, names, etc. delete the old one add in the
        // old one
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
                auto warpedMesh = WarpMesh(mesh, *meshWarper);
                OpenSim::Mesh* targetMesh = FindComponentMut<OpenSim::Mesh>(warpedModel, mesh.getAbsolutePath());
                OSC_ASSERT_ALWAYS(targetMesh && "cannot find target mesh in output model: this should never happen");
                OverwriteGeometry(*targetMesh, std::move(warpedMesh));
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
