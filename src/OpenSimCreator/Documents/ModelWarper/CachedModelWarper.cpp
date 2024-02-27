#include "CachedModelWarper.h"

#include <OpenSimCreator/Documents/ModelWarper/Document.h>

#include <OpenSim/Simulation/Model/Model.h>

#include <memory>
#include <optional>

using namespace osc::mow;

class osc::mow::CachedModelWarper::Impl final {
public:
    std::shared_ptr<OpenSim::Model const> warp(Document const& document)
    {
        if (document != *m_PreviousDocument) {
            m_PreviousResult = createWarpedModel(document);
            m_PreviousDocument = document;
        }
        return m_PreviousResult;
    }

    std::shared_ptr<OpenSim::Model const> createWarpedModel(Document const& document)
    {
        return std::make_unique<OpenSim::Model>(document.model());  // TODO: actually warp it
    }
private:
    std::optional<Document> m_PreviousDocument;
    std::shared_ptr<OpenSim::Model const> m_PreviousResult;
};

osc::mow::CachedModelWarper::CachedModelWarper() :
    m_Impl{std::make_unique<Impl>()}
{}
osc::mow::CachedModelWarper::CachedModelWarper(CachedModelWarper&&) noexcept = default;
CachedModelWarper& osc::mow::CachedModelWarper::operator=(CachedModelWarper&&) noexcept = default;
osc::mow::CachedModelWarper::~CachedModelWarper() noexcept = default;

std::shared_ptr<OpenSim::Model const> osc::mow::CachedModelWarper::warp(Document const& document)
{
    return m_Impl->warp(document);
}
