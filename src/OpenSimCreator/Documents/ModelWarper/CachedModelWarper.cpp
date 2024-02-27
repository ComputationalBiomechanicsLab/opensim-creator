#include "CachedModelWarper.h"

#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSimCreator/Documents/Model/BasicModelStatePair.h>
#include <OpenSimCreator/Documents/ModelWarper/Document.h>

#include <memory>
#include <optional>

using namespace osc;
using namespace osc::mow;

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
        auto rv = std::make_shared<BasicModelStatePair>(document.modelstate());

        for ([[maybe_unused]] auto const& mesh : document.model().getComponentList<OpenSim::Mesh>()) {
            [[maybe_unused]] auto const* warper = document.findMeshWarp(mesh);
            // TODO
            // use the warper to warp the mesh data
            // re-export the warped mesh points as a temporary OBJ
            // mutate the returned model to point to the OBJ
        }

        return rv;
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
