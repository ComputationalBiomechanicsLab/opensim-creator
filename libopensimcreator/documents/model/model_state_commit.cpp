#include "model_state_commit.h"

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/synchronized_value_guard.h>
#include <liboscar/utilities/uid.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <memory>
#include <mutex>
#include <string>
#include <string_view>

using namespace osc;

class osc::ModelStateCommit::Impl final {
public:
    Impl(const opyn::ModelStatePair& msp, std::string_view message) :
        Impl{msp, message, UID::empty()}
    {}

    Impl(const opyn::ModelStatePair& msp, std::string_view message, UID parent) :
        m_MaybeParentID{parent},
        m_Model{std::make_unique<OpenSim::Model>(msp.get_model())},
        m_ModelVersion{msp.get_model_version()},
        m_FixupScaleFactor{msp.get_fixup_scale_factor()},
        m_CommitMessage{message}
    {
        opyn::initialize_model(*m_Model);
        opyn::initialize_state(*m_Model);
    }

    UID getID() const
    {
        return m_ID;
    }

    bool hasParent() const
    {
        return m_MaybeParentID != UID::empty();
    }

    UID getParentID() const
    {
        return m_MaybeParentID;
    }

    CStringView getCommitMessage() const
    {
        return m_CommitMessage;
    }

    SynchronizedValueGuard<const OpenSim::Model> getModel() const
    {
        return {m_AccessMutex, *m_Model};
    }

    UID getModelVersion() const
    {
        return m_ModelVersion;
    }

    float getFixupScaleFactor() const
    {
        return m_FixupScaleFactor;
    }

private:
    mutable std::mutex m_AccessMutex;
    UID m_ID;
    UID m_MaybeParentID;
    std::unique_ptr<OpenSim::Model> m_Model;
    UID m_ModelVersion;
    float m_FixupScaleFactor;
    std::string m_CommitMessage;
};


osc::ModelStateCommit::ModelStateCommit(const opyn::ModelStatePair& p, std::string_view message) :
    m_Impl{std::make_shared<Impl>(p, message)}
{}
osc::ModelStateCommit::ModelStateCommit(const opyn::ModelStatePair& p, std::string_view message, UID parent) :
    m_Impl{std::make_shared<Impl>(p, message, parent)}
{}

UID osc::ModelStateCommit::getID() const
{
    return m_Impl->getID();
}

bool osc::ModelStateCommit::hasParent() const
{
    return m_Impl->hasParent();
}

UID osc::ModelStateCommit::getParentID() const
{
    return m_Impl->getParentID();
}

CStringView osc::ModelStateCommit::getCommitMessage() const
{
    return m_Impl->getCommitMessage();
}

SynchronizedValueGuard<const OpenSim::Model> osc::ModelStateCommit::getModel() const
{
    return m_Impl->getModel();
}

UID osc::ModelStateCommit::getModelVersion() const
{
    return m_Impl->getModelVersion();
}

float osc::ModelStateCommit::getFixupScaleFactor() const
{
    return m_Impl->getFixupScaleFactor();
}
